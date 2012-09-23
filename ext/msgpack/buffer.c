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
#include "rmem.h"

#ifdef RUBY_VM
#define HAVE_RB_STR_REPLACE
#endif

#ifndef HAVE_RB_STR_REPLACE
static ID s_replace;
#endif

#ifndef DISABLE_RMEM
static msgpack_rmem_t s_rmem;
#endif

void msgpack_buffer_static_init()
{
#ifndef DISABLE_RMEM
    msgpack_rmem_init(&s_rmem);
#endif
#ifndef HAVE_RB_STR_REPLACE
    s_replace = rb_intern("replace");
#endif
}

void msgpack_buffer_static_destroy()
{
#ifndef DISABLE_RMEM
    msgpack_rmem_destroy(&s_rmem);
#endif
}

void msgpack_buffer_init(msgpack_buffer_t* b)
{
    memset(b, 0, sizeof(msgpack_buffer_t));

    b->head = &b->tail;
    b->append_reference_threshold = MSGPACK_BUFFER_STRING_APPEND_REFERENCE_DEFAULT;
}

static void _msgpack_buffer_chunk_destroy(msgpack_buffer_chunk_t* c)
{
    if(c->mem != NULL) {
#ifndef DISABLE_RMEM
        if(!msgpack_rmem_free(&s_rmem, c->mem)) {
            free(c->mem);
        }
        /* no needs to update rmem_owner because chunks will not be
         * free()ed and thus *rmem_owner = NULL is always valid. */
#else
        free(c->mem);
#endif
    }
    c->first = NULL;
    c->last = NULL;
    c->mem = NULL;
}

void msgpack_buffer_destroy(msgpack_buffer_t* b)
{
    /* head is always available */
    msgpack_buffer_chunk_t* c = b->head;
    while(c != &b->tail) {
        msgpack_buffer_chunk_t* n = c->next;
        _msgpack_buffer_chunk_destroy(c);
        free(c);
        c = n;
    }
    _msgpack_buffer_chunk_destroy(c);
    //while(c != &b->tail) {
    //    _msgpack_buffer_chunk_destroy(c);
    //    msgpack_buffer_chunk_t* n = c->next;
    //    free(c);
    //    c = n;
    //}
    //if(c->first != NULL) {
    //    /* tail may not be initialized */
    //    _msgpack_buffer_chunk_destroy(c);
    //}

    c = b->free_list;
    while(c != NULL) {
        msgpack_buffer_chunk_t* n = c->next;
        free(c);
        c = n;
    }
}

void msgpack_buffer_mark(msgpack_buffer_t* b)
{
    /* head is always available */
    msgpack_buffer_chunk_t* c = b->head;
    while(c != &b->tail) {
        rb_gc_mark(c->mapped_string);
        c = c->next;
    }
    rb_gc_mark(c->mapped_string);

    rb_gc_mark(b->owner);
}

bool _msgpack_buffer_shift_chunk(msgpack_buffer_t* b)
{
    _msgpack_buffer_chunk_destroy(b->head);

    if(b->head == &b->tail) {
        /* list becomes empty. don't add head to free_list
         * because head should be always available */
        b->tail_buffer_end = NULL;
        b->read_buffer = NULL;
        return false;
    }

    /* add head to free_list */
    msgpack_buffer_chunk_t* next_head = b->head->next;
    b->head->next = b->free_list;
    b->free_list = b->head;

    b->head = next_head;
    b->read_buffer = next_head->first;

    return true;
}

void msgpack_buffer_clear(msgpack_buffer_t* b)
{
    while(_msgpack_buffer_shift_chunk(b)) {
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
}

bool msgpack_buffer_read_all(msgpack_buffer_t* b, char* buffer, size_t length)
{
    /* buffer == NULL means skip */
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
        _msgpack_buffer_shift_chunk(b);
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

        if(!_msgpack_buffer_shift_chunk(b)) {
            return length_orig - length;
        }
    }
}

static inline msgpack_buffer_chunk_t* _msgpack_buffer_alloc_new_chunk(msgpack_buffer_t* b)
{
    msgpack_buffer_chunk_t* reuse = b->free_list;
    if(reuse == NULL) {
        return malloc(sizeof(msgpack_buffer_chunk_t));
    }
    b->free_list = b->free_list->next;
    return reuse;
}

static inline void _msgpack_buffer_add_new_chunk(msgpack_buffer_t* b)
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

#ifndef DISABLE_RMEM
#ifndef DISABLE_RMEM_REUSE_INTERNAL_FRAGMENT
        if(b->rmem_owner == &b->tail.mem) {
            /* reuse unused rmem */
            size_t unused = b->tail_buffer_end - b->tail.last;
            b->rmem_last -= unused;
        }
#endif
#endif

        /* rebuild tail */
        *nc = b->tail;
        before_tail->next = nc;
        nc->next = &b->tail;
    }
}

void _msgpack_buffer_append_reference(msgpack_buffer_t* b, const char* data, size_t length, VALUE mapped_string)
{
    _msgpack_buffer_add_new_chunk(b);

    b->tail.first = (char*) data;
    b->tail.last = (char*) data + length;
    b->tail.mapped_string = mapped_string;
    b->tail.mem = NULL;

    /* msgpack_buffer_writable_size should return 0 for mapped chunk */
    b->tail_buffer_end = b->tail.last;

    /* consider read_buffer */
    if(b->head == &b->tail) {
        b->read_buffer = b->tail.first;
    }
}

static inline void* _msgpack_buffer_chunk_malloc(
        msgpack_buffer_t* b, msgpack_buffer_chunk_t* c,
        size_t required_size, size_t* allocated_size)
{
#ifndef DISABLE_RMEM
    if(required_size <= MSGPACK_RMEM_PAGE_SIZE) {
#ifndef DISABLE_RMEM_REUSE_INTERNAL_FRAGMENT
        if((size_t)(b->rmem_end - b->rmem_last) < required_size) {
#endif
            /* alloc new rmem page */
            *allocated_size = MSGPACK_RMEM_PAGE_SIZE;
            char* buffer = msgpack_rmem_alloc(&s_rmem);
            c->mem = buffer;

            /* update rmem owner */
            b->rmem_owner = &c->mem;
            b->rmem_last = b->rmem_end = buffer + MSGPACK_RMEM_PAGE_SIZE;

            return buffer;

#ifndef DISABLE_RMEM_REUSE_INTERNAL_FRAGMENT
        } else {
            /* reuse unused rmem */
            *allocated_size = (size_t)(b->rmem_end - b->rmem_last);
            char* buffer = b->rmem_last;
            b->rmem_last = b->rmem_end;

            /* update rmem owner */
            c->mem = *b->rmem_owner;
            *b->rmem_owner = NULL;
            b->rmem_owner = &c->mem;

            return buffer;
        }
#endif
    }
#else
    if(required_size < 72) {
        required_size = 72;
    }
#endif

    // TODO alignment?
    *allocated_size = required_size;
    void* mem = malloc(required_size);
    c->mem = mem;
    return mem;
}

static inline void* _msgpack_buffer_chunk_realloc(
        msgpack_buffer_t* b, msgpack_buffer_chunk_t* c,
        void* mem, size_t required_size, size_t* current_size)
{
    if(mem == NULL) {
        return _msgpack_buffer_chunk_malloc(b, c, required_size, current_size);
    }

    size_t next_size = *current_size * 2;
    while(next_size < required_size) {
        next_size *= 2;
    }
    *current_size = next_size;
    mem = realloc(mem, next_size);

    c->mem = mem;
    return mem;
}

void _msgpack_buffer_append2(msgpack_buffer_t* b, const char* data, size_t length)
{
    /* data == NULL means ensure_sequential */
    if(data != NULL) {
        size_t tail_avail = b->tail_buffer_end - b->tail.last;
        memcpy(b->tail.last, data, tail_avail);
        b->tail.last += tail_avail;
        data += tail_avail;
        length -= tail_avail;
    }

    size_t capacity = b->tail_buffer_end - b->tail.first;

    /* can't realloc mapped chunk or rmem page */
    if(b->tail.mapped_string != NO_MAPPED_STRING
#ifndef DISABLE_RMEM
            || capacity <= MSGPACK_RMEM_PAGE_SIZE
#endif
            ) {
        /* allocate new chunk */
        _msgpack_buffer_add_new_chunk(b);

        char* mem = _msgpack_buffer_chunk_malloc(b, &b->tail, length, &capacity);
        //printf("tail malloc()ed capacity %lu\n", capacity);

        char* last = mem;
        if(data != NULL) {
            memcpy(mem, data, length);
            last += length;
        }

        /* rebuild tail chunk */
        b->tail.first = mem;
        b->tail.last = last;
        b->tail.mapped_string = NO_MAPPED_STRING;
        b->tail_buffer_end = mem + capacity;

        /* consider read_buffer */
        if(b->head == &b->tail) {
            b->read_buffer = b->tail.first;
        }

    } else {
        /* realloc malloc()ed chunk or NULL */
        size_t tail_filled = b->tail.last - b->tail.first;
        char* mem = _msgpack_buffer_chunk_realloc(b, &b->tail,
                b->tail.first, tail_filled+length, &capacity);
        //printf("tail realloc()ed capacity %lu filled=%lu\n", capacity, tail_filled);

        char* last = mem + tail_filled;
        if(data != NULL) {
            memcpy(last, data, length);
            last += length;
        }

        /* consider read_buffer */
        if(b->head == &b->tail) {
            size_t read_offset = b->read_buffer - b->head->first;
            //printf("relink read_buffer read_offset %lu\n", read_offset);
            b->read_buffer = mem + read_offset;
        }

        /* rebuild tail chunk */
        b->tail.first = mem;
        b->tail.last = last;
        b->tail_buffer_end = mem + capacity;
        //printf("alloced chunk size: %lu\n", msgpack_buffer_top_readable_size(b));
    }
}

static inline VALUE _msgpack_buffer_head_chunk_as_string(msgpack_buffer_t* b)
{
    size_t length = b->head->last - b->read_buffer;
    if(length == 0) {
        return rb_str_buf_new(0);
    }

    if(b->head->mapped_string != NO_MAPPED_STRING) {
        return _msgpack_buffer_refer_head_mapped_string(b, length);
    }

    return rb_str_new(b->read_buffer, length);
}

static inline VALUE _msgpack_buffer_chunk_as_string(msgpack_buffer_chunk_t* c)
{
    size_t chunk_size = c->last - c->first;
    if(chunk_size == 0) {
        return rb_str_buf_new(0);
    }

    if(c->mapped_string != NO_MAPPED_STRING) {
        return rb_str_dup(c->mapped_string);
    }

    return rb_str_new(c->first, chunk_size);
}

size_t msgpack_buffer_read_to_string(msgpack_buffer_t* b, VALUE string, size_t length)
{
    size_t chunk_size = msgpack_buffer_top_readable_size(b);
    if(chunk_size == 0) {
        return 0;
    }

#ifndef DISABLE_BUFFER_READ_REFERENCE_OPTIMIZE
    /* optimize */
    if(length <= chunk_size &&
            b->head->mapped_string != NO_MAPPED_STRING &&
            length >= MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD &&
            RSTRING_LEN(string) == 0) {
        VALUE s = _msgpack_buffer_refer_head_mapped_string(b, length);
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
//printf("chunk size: %lu\n", chunk_size);
//printf("length: %lu\n", length);
        if(length <= chunk_size) {
//printf("read_buffer: %p\n", b->read_buffer);
            rb_str_buf_cat(string, b->read_buffer, length);

            _msgpack_buffer_consumed(b, length);
            return length;
        }

        rb_str_buf_cat(string, b->read_buffer, chunk_size);
        length -= chunk_size;

        if(!_msgpack_buffer_shift_chunk(b)) {
            return length_orig - length;
        }

        chunk_size = msgpack_buffer_top_readable_size(b);
    }
}

VALUE msgpack_buffer_all_as_string(msgpack_buffer_t* b)
{
    if(b->head == &b->tail) {
        return _msgpack_buffer_head_chunk_as_string(b);
    }

    size_t length = msgpack_buffer_all_readable_size(b);
    VALUE string = rb_str_new(NULL, length);
    char* buffer = RSTRING_PTR(string);

    size_t chunk_size = msgpack_buffer_top_readable_size(b);
    memcpy(buffer, b->read_buffer, chunk_size);
    buffer += chunk_size;
    length -= chunk_size;

    msgpack_buffer_chunk_t* c = b->head->next;

    while(true) {
        chunk_size = c->last - c->first;
        memcpy(buffer, c->first, chunk_size);

        if(length <= chunk_size) {
            return string;
        }
        buffer += chunk_size;
        length -= chunk_size;

        c = c->next;
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

    /* TODO optimize ary construction */
    VALUE ary = rb_ary_new();

    VALUE s = _msgpack_buffer_head_chunk_as_string(b);
    rb_ary_push(ary, s);

    msgpack_buffer_chunk_t* c = b->head->next;

    while(true) {
        s = _msgpack_buffer_chunk_as_string(c);
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

    while(_msgpack_buffer_shift_chunk(b)) {
        VALUE s = _msgpack_buffer_chunk_as_string(b->head);
        rb_funcall(io, write_method, 1, s);
    }
}

