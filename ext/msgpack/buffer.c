/*
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2012 FURUHASHI Sadayuki
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "buffer.h"
#include "rb_ext.h"

#ifdef RUBY_VM
#define HAVE_RB_STR_REPLACE
#endif

#ifndef HAVE_RB_STR_REPLACE
static ID s_replace;
#endif

void msgpack_buffer_static_init()
{
#ifndef HAVE_RB_STR_REPLACE
    s_replace = rb_intern("replace");
#endif
}

void msgpack_buffer_static_destroy()
{ }

void msgpack_buffer_init(msgpack_buffer_t* b)
{
    memset(b, 0, sizeof(msgpack_buffer_t));

    b->head = &b->tail;
    b->append_reference_threshold = MSGPACK_BUFFER_STRING_APPEND_REFERENCE_DEFAULT;
}

static void _msgpack_buffer_chunk_destroy(msgpack_buffer_chunk_t* c)
{
    if(c->mapped_string == NO_MAPPED_STRING) {
        free(c->first);
    } else {
        c->mapped_string = NO_MAPPED_STRING;
    }
    c->first = NULL;
    c->last = NULL;
}

void msgpack_buffer_destroy(msgpack_buffer_t* b)
{
    msgpack_buffer_chunk_t* c = b->head;
    msgpack_buffer_chunk_t* n;
    while(c != &b->tail) {
        _msgpack_buffer_chunk_destroy(c);
        n = c->next;
        free(c);
        c = n;
    }
    if(c->first != NULL) {
        /* tail may not be initialized */
        _msgpack_buffer_chunk_destroy(c);
    }

    c = b->free_list;
    while(c != NULL) {
        n = c->next;
        free(c);
        c = n;
    }
}

void msgpack_buffer_mark(msgpack_buffer_t* b)
{
    msgpack_buffer_chunk_t* c = b->head;
    while(c != &b->tail) {
        rb_gc_mark(c->mapped_string);
        c = c->next;
    }
    rb_gc_mark(c->mapped_string);

    rb_gc_mark(b->owner);
}

static inline void _msgpack_buffer_pop_chunk_no_check(msgpack_buffer_t* b)
{
    _msgpack_buffer_chunk_destroy(b->head);

    msgpack_buffer_chunk_t* next_head = b->head->next;
    b->head->next = b->free_list;
    b->free_list = b->head;
    b->head = next_head;

    b->read_buffer = b->head->first;
}

bool _msgpack_buffer_pop_chunk(msgpack_buffer_t* b)
{
    _msgpack_buffer_chunk_destroy(b->head);

    if(b->head == &b->tail) {
        /* list becomes empty */
        b->tail_buffer_end = NULL;
        b->read_buffer = NULL;
        return false;
    }

    msgpack_buffer_chunk_t* next_head = b->head->next;
    b->head->next = b->free_list;
    b->free_list = b->head;
    b->head = next_head;

    b->read_buffer = b->head->first;

    return true;
}

void msgpack_buffer_clear(msgpack_buffer_t* b)
{
    while(_msgpack_buffer_pop_chunk(b)) {
        ;
    }
}

size_t msgpack_buffer_all_readable_size(const msgpack_buffer_t* b)
{
    size_t sz = msgpack_buffer_top_readable_size(b);

    if(b->head == &b->tail) {
        return sz;
    }

    msgpack_buffer_chunk_t* c = b->head->next;

    while(true) {
        sz += c->last - c->first;
        if(c == &b->tail) {
            return sz;
        }
        c = c->next;
    }

    return sz;
}

union msgpack_buffer_cast_block_t* _msgpack_buffer_build_cast_block(msgpack_buffer_t* b, size_t n);

bool msgpack_buffer_skip_all(msgpack_buffer_t* b, size_t length)
{
    return msgpack_buffer_read_all(b, NULL, length);
}

bool msgpack_buffer_read_all(msgpack_buffer_t* b, char* buffer, size_t length)
{
    if(length == 0) {
        return true;
    }

    size_t chunk_size = msgpack_buffer_top_readable_size(b);
    if(length <= chunk_size) {
        if(buffer != NULL) {
            memcpy(buffer, b->read_buffer, length);
        }
        _msgpack_buffer_consumed(b, length);
        return true;

    } else if(b->head == &b->tail) {
        return false;
    }

    if(buffer != NULL) {
        memcpy(buffer, b->read_buffer, chunk_size);
        buffer += chunk_size;
    }
    length -= chunk_size;

    size_t consumed_chunk = 1;
    msgpack_buffer_chunk_t* c = b->head->next;

    while(true) {
        chunk_size = c->last - c->first;
        if(length <= chunk_size) {
            if(buffer != NULL) {
                memcpy(buffer, c->first, length);
            }
            break;

        } else if(c == &b->tail) {
            return false;
        }

        if(buffer != NULL) {
            memcpy(buffer, c->first, chunk_size);
            buffer += chunk_size;
        }
        length -= chunk_size;
        consumed_chunk++;

        c = c->next;
    }

    size_t i;
    for(i = 0; i < consumed_chunk; i++) {
        _msgpack_buffer_pop_chunk_no_check(b);
    }

    _msgpack_buffer_consumed(b, length);
    return true;
}

size_t msgpack_buffer_read(msgpack_buffer_t* b, char* buffer, size_t length)
{
    size_t const length_orig = length;

    while(true) {
        size_t chunk_size = msgpack_buffer_top_readable_size(b);

        if(length <= chunk_size) {
            memcpy(buffer, b->read_buffer, length);

            _msgpack_buffer_consumed(b, length);
            return length;
        }

        memcpy(buffer, b->read_buffer, chunk_size);
        buffer += chunk_size;
        length -= chunk_size;

        if(!_msgpack_buffer_pop_chunk(b)) {
            return length_orig - length;
        }
    }
}

#ifndef DISABLE_STR_NEW_MOVE
static inline void _msgpack_buffer_chunk_buffer_to_mapping_string(
        msgpack_buffer_t* b, msgpack_buffer_chunk_t* c)
{
    UNUSED(b);
    c->mapped_string = rb_str_new_move(c->first, c->last - c->first);
}
#endif

static VALUE _msgpack_buffer_chunk_as_string(
        msgpack_buffer_t* b, msgpack_buffer_chunk_t* c)
{
    size_t chunk_size = c->last - c->first;
    if(chunk_size == 0) {
        return rb_str_buf_new(0);
    }

    if(c->mapped_string != NO_MAPPED_STRING) {
        return rb_str_dup(c->mapped_string);
    }

#ifndef DISABLE_STR_NEW_MOVE
    _msgpack_buffer_chunk_buffer_to_mapping_string(b, c);
    return rb_str_dup(c->mapped_string);
#else
    return rb_str_new(c->first, chunk_size);
#endif
}

static inline void* _msgpack_buffer_chunk_malloc(
        size_t required_size, size_t* allocated_size)
{
    *allocated_size = required_size;
#ifndef DISABLE_STR_NEW_MOVE
    /* +1 for rb_str_new_move */
    required_size += 1;
#endif
    return malloc(required_size);
}

static inline void* _msgpack_buffer_chunk_realloc(void* ptr,
        size_t required_size, size_t* current_size)
{
    if(*current_size <= 0) {
        return _msgpack_buffer_chunk_malloc(required_size, current_size);
    }

#ifndef DISABLE_STR_NEW_MOVE
    /* +1 for rb_str_new_move */
    required_size += 1;
#endif

    size_t next_size = *current_size * 2;
    while(next_size < required_size) {
        next_size *= 2;
    }

#ifndef DISABLE_STR_NEW_MOVE
    *current_size = next_size - 1;
#else
    *current_size = next_size;
#endif
    return realloc(ptr, next_size);
}

static VALUE _msgpack_buffer_chunk_as_string_substr(
        msgpack_buffer_t* b, msgpack_buffer_chunk_t* c,
        size_t offset, size_t length)
{
    if(length == 0) {
        return rb_str_buf_new(0);
    }

    if(c->mapped_string != NO_MAPPED_STRING) {
        return rb_str_substr(c->mapped_string, offset, length);
    }

#ifndef DISABLE_STR_NEW_MOVE
    _msgpack_buffer_chunk_buffer_to_mapping_string(b, c);
    return rb_str_substr(c->mapped_string, offset, length);
#else
    return rb_str_new(c->first + offset, length);
#endif
}

static inline VALUE _msgpack_buffer_head_chunk_as_string(msgpack_buffer_t* b)
{
    size_t offset = b->read_buffer - b->head->first;
    size_t length = b->head->last - b->head->first - offset;
    return _msgpack_buffer_chunk_as_string_substr(b, &b->tail, offset, length);
}

VALUE msgpack_buffer_all_as_string(msgpack_buffer_t* b)
{
    if(b->head == &b->tail) {
        return _msgpack_buffer_head_chunk_as_string(b);
    }

    size_t sz = msgpack_buffer_all_readable_size(b);
    VALUE string = rb_str_buf_new(sz);

    if(sz == 0) {
        return string;
    }

    msgpack_buffer_read_to_string(b, string, sz);

    _msgpack_buffer_append_reference(b, RSTRING_PTR(string), RSTRING_LEN(string), string);

    return rb_str_dup(string);
}

VALUE msgpack_buffer_read_top_as_string(msgpack_buffer_t* b, size_t length, bool suppress_reference)
{
    VALUE result;

    if(!suppress_reference &&
            length >= MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD
#ifdef DISABLE_STR_NEW_MOVE
            && b->head->mapped_string != NO_MAPPED_STRING
#endif
            ) {
        result = _msgpack_buffer_head_chunk_as_string(b);
    } else {
        result = rb_str_new(b->read_buffer, length);
    }

    _msgpack_buffer_consumed(b, length);

    return result;
}

size_t msgpack_buffer_read_to_string(msgpack_buffer_t* b, VALUE string, size_t length)
{
    size_t chunk_size = msgpack_buffer_top_readable_size(b);
    if(chunk_size == 0) {
        return 0;
    }

#ifndef DISABLE_BUFFER_READ_REFERENCE_OPTIMIZE
    /* optimize */
    if(length >= MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD &&
            RSTRING_LEN(string) == 0 &&
            length <= chunk_size
#ifdef DISABLE_STR_NEW_MOVE
            && b->head->mapped_string != NO_MAPPED_STRING
#endif
            ) {
        size_t read_offset = b->read_buffer - b->head->first;
        VALUE s = _msgpack_buffer_chunk_as_string_substr(b, b->head, read_offset, length);
#ifndef HAVE_RB_STR_REPLACE
        /* TODO MRI 1.8 */
        rb_funcall(string, s_replace, 1, s);
#else
        rb_str_replace(string, s);
#endif
        _msgpack_buffer_consumed(b, length);
        return length;
    }
#endif

    size_t const length_orig = length;

    while(true) {
        if(length <= chunk_size) {
            rb_str_buf_cat(string, b->read_buffer, length);

            _msgpack_buffer_consumed(b, length);
            return length;
        }

        rb_str_buf_cat(string, b->read_buffer, chunk_size);
        length -= chunk_size;

        if(!_msgpack_buffer_pop_chunk(b)) {
            return length_orig - length;
        }

        chunk_size = msgpack_buffer_top_readable_size(b);
    }
}

VALUE msgpack_buffer_all_as_string_array(msgpack_buffer_t* b)
{
    if(b->head == &b->tail) {
        VALUE s = msgpack_buffer_all_as_string(b);
        VALUE ary = rb_ary_new();  /* TODO capacity = 1 */
        rb_ary_push(ary, s);
        return ary;
    }

    VALUE ary = rb_ary_new();

    VALUE s = _msgpack_buffer_head_chunk_as_string(b);
    rb_ary_push(ary, s);

    msgpack_buffer_chunk_t* c = b->head->next;

    while(true) {
        s = _msgpack_buffer_chunk_as_string(b, c);
        rb_ary_push(ary, s);
        if(c == &b->tail) {
            return ary;
        }
        c = c->next;
    }

    return ary;
}

void msgpack_buffer_flush_to_io(msgpack_buffer_t* b, VALUE io, ID write_method)
{
    if(msgpack_buffer_top_readable_size(b) == 0) {
        return;
    }

    VALUE s = _msgpack_buffer_head_chunk_as_string(b);
    rb_funcall(io, write_method, 1, s);

    while(_msgpack_buffer_pop_chunk(b)) {
        VALUE s = _msgpack_buffer_chunk_as_string(b, b->head);
        rb_funcall(io, write_method, 1, s);
    }
}


static inline msgpack_buffer_chunk_t* _msgpack_buffer_alloc_new_chunk(msgpack_buffer_t* b)
{
    msgpack_buffer_chunk_t* candidate = b->free_list;
    if(candidate == NULL) {
        return malloc(sizeof(msgpack_buffer_chunk_t));
    }
    b->free_list = b->free_list->next;
    return candidate;
}

void _msgpack_buffer_add_new_chunk(msgpack_buffer_t* b)
{
    if(b->head == &b->tail) {
        if(b->tail.first == NULL) {
            /* empty buffer */
            return;
        }

        msgpack_buffer_chunk_t* nc = _msgpack_buffer_alloc_new_chunk(b);

        *nc = b->tail;
        b->head = nc;
        nc->next = &b->tail;

    } else {
        /* search node before tail */
        msgpack_buffer_chunk_t* before_tail = b->head;
        while(before_tail->next != &b->tail) {
            before_tail = before_tail->next;
        }

        msgpack_buffer_chunk_t* nc = _msgpack_buffer_alloc_new_chunk(b);

        *nc = b->tail;
        before_tail->next = nc;
        nc->next = &b->tail;
    }

    /*
    b->tail.next = NULL;
    b->tail.mapped_string = NO_MAPPED_STRING;
    */
}

void _msgpack_buffer_append2(msgpack_buffer_t* b, const char* data, size_t length)
{
    size_t tail_filled = b->tail.last - b->tail.first;

    if(b->tail.mapped_string == NO_MAPPED_STRING) {
        /* realloc */
        size_t capacity = b->tail_buffer_end - b->tail.first;
        char* mem = _msgpack_buffer_chunk_realloc(b->tail.first, tail_filled+length, &capacity);
//printf("realloc %lu tail filled=%lu\n", capacity, tail_filled);

        char* last = mem + tail_filled;
        if(data != NULL) {
            memcpy(last, data, length);
            last += length;
        }

        /* consider read_buffer */
        if(b->head == &b->tail) {
            size_t read_offset = b->read_buffer - b->head->first;
            b->read_buffer = mem + read_offset;
        }

        /* rebuild tail chunk */
        b->tail.first = mem;
        b->tail.last = last;
        b->tail_buffer_end = mem + capacity;

    } else {
        /* allocate new chunk */
        size_t capacity;
        char* mem = _msgpack_buffer_chunk_malloc(length, &capacity);

        char* last = mem;
        if(data != NULL) {
            memcpy(mem, data, length);
            last += length;
        }

        _msgpack_buffer_add_new_chunk(b);

        /* rebuild tail chunk */
        b->tail.first = mem;
        b->tail.last = last;
        b->tail.mapped_string = NO_MAPPED_STRING;
        b->tail_buffer_end = mem + capacity;

        /* consider read_buffer */
        if(b->head == &b->tail) {
            b->read_buffer = b->tail.first;
        }
    }
}

void _msgpack_buffer_append_reference(msgpack_buffer_t* b, const char* data, size_t length, VALUE mapped_string)
{
    _msgpack_buffer_add_new_chunk(b);

    b->tail.first = (char*) data;
    b->tail.last = (char*) data + length;
    b->tail.mapped_string = mapped_string;
    b->tail_buffer_end = b->tail.last;

    /* consider read_buffer */
    if(b->head == &b->tail) {
        b->read_buffer = b->tail.first;
    }
}

