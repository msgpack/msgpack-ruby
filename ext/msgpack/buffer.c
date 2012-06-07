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

void msgpack_buffer_static_init()
{
    msgpack_pool_static_init_default();
}

void msgpack_buffer_static_destroy()
{
    msgpack_pool_static_destroy();
}

void msgpack_buffer_init(msgpack_buffer_t* b)
{
    memset(b, 0, sizeof(msgpack_buffer_t));

    b->head = &b->tail;
}

static void _msgpack_buffer_chunk_destroy(msgpack_buffer_chunk_t* c)
{
    if(c->mapped_string == NO_MAPPED_STRING) {
        msgpack_pool_static_free(c->first, c->last - c->first);
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

static VALUE _msgpack_buffer_chunk_as_string(msgpack_buffer_chunk_t* c, size_t read_offset)
{
    size_t sz = c->last - c->first - read_offset;

    if(c->mapped_string != NO_MAPPED_STRING) {
        if(read_offset > 0) {
            return rb_str_substr(c->mapped_string, read_offset, sz);
        } else {
            return rb_str_dup(c->mapped_string);
        }
    }

    if(sz == 0) {
        return rb_str_buf_new(0);
    }

#ifndef DISABLE_STR_NEW_MOVE
    if(msgpack_pool_static_try_move_to_string(
                c->first, c->last - c->first, &c->mapped_string)) {
        if(read_offset > 0) {
            return rb_str_substr(c->mapped_string, read_offset, sz);
        } else {
            return rb_str_dup(c->mapped_string);
        }
    }
#endif

    return rb_str_new(c->first + read_offset, sz);
}

VALUE msgpack_buffer_all_as_string(msgpack_buffer_t* b)
{
    if(b->head == &b->tail) {
        size_t read_offset = b->read_buffer - b->head->first;
        return _msgpack_buffer_chunk_as_string(&b->tail, read_offset);
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

bool msgpack_buffer_try_refer_string(msgpack_buffer_t* b, size_t length, VALUE* dest)
{
    if(length >= MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD
            && msgpack_buffer_top_readable_size(b) >= length
#ifdef DISABLE_STR_NEW_MOVE
            && b->head->mapped_string != NO_MAPPED_STRING
#endif
            ) {

        size_t read_offset = b->read_buffer - b->head->first;
        *dest = _msgpack_buffer_chunk_as_string(b->head, read_offset);

        _msgpack_buffer_consumed(b, length);
        return true;
    }

    return false;
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
        VALUE s = _msgpack_buffer_chunk_as_string(b->head, read_offset);
        if(RSTRING_LEN(s) > length) {
            s = rb_str_substr(s, 0, length);
        }
        rb_str_replace(string, s);
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
    VALUE s;

    size_t read_offset = b->read_buffer - b->head->first;
    s = _msgpack_buffer_chunk_as_string(b->head, read_offset);
    rb_ary_push(ary, s);

    msgpack_buffer_chunk_t* c = b->head->next;

    while(true) {
        s = _msgpack_buffer_chunk_as_string(c, 0);
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

    size_t read_offset = b->read_buffer - b->head->first;
    VALUE s = _msgpack_buffer_chunk_as_string(&b->tail, read_offset);
    rb_funcall(io, write_method, 1, s);

    while(_msgpack_buffer_pop_chunk(b)) {
        VALUE s = _msgpack_buffer_chunk_as_string(b->head, 0);
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
        char* mem = msgpack_pool_static_realloc(b->tail.first, tail_filled+length, &capacity);
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
        char* mem = msgpack_pool_static_alloc(length, &capacity);

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

