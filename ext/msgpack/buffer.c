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

void msgpack_buffer_init(msgpack_buffer_t* b)
{
    memset(b, 0, sizeof(msgpack_buffer_t));

    b->head = &b->tail;
}

static void _msgpack_buffer_chunk_destroy(msgpack_buffer_chunk_t* c)
{
    if(c->mapped_string == NO_MAPPED_STRING) {
        //msgpack_pool_free(c->first);
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
    while(c != &b->tail) {
        _msgpack_buffer_chunk_destroy(c);
        c = c->next;
    }
    _msgpack_buffer_chunk_destroy(c);
}

void msgpack_buffer_mark(msgpack_buffer_t* b)
{
    msgpack_buffer_chunk_t* c = b->head;
    while(c != &b->tail) {
        rb_gc_mark(c->mapped_string);
        c = c->next;
    }
    rb_gc_mark(c->mapped_string);
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

size_t msgpack_buffer_read_to_string(msgpack_buffer_t* b, VALUE string, size_t length)
{
    size_t chunk_size = msgpack_buffer_top_readable_size(b);
    if(chunk_size == 0) {
        return 0;
    }

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

static VALUE _msgpack_buffer_chunk_as_string(msgpack_buffer_chunk_t* c)
{
    if(c->mapped_string != NO_MAPPED_STRING) {
        return rb_str_dup(c->mapped_string);
    }
    size_t sz = c->last - c->first;
    if(sz == 0) {
        return rb_str_buf_new(0);
#ifdef USE_STR_NEW_MOVE
        /* note: this code is magical: */
    } else if(sz > RSTRING_EMBED_LEN_MAX) {
        c->mapped_string = msgpack_pool_static_move_to_string(
                c->first, 0, c->last - c->first);
        //VALUE mapped_string = rb_str_new_move(c->first,
        //        c->last - c->first,
        //        c->last - c->first);
        //c->mapped_string = mapped_string;
        return c->mapped_string;
#endif
    } else {
        return rb_str_new(c->first, c->last - c->first);
    }
}

VALUE msgpack_buffer_all_as_string(msgpack_buffer_t* b)
{
    if(b->head == &b->tail && b->read_buffer == b->tail.first) {
        /*
         * b->read_buffer == b->tail.first and b->head != &b->tail
         * if mapped_string is shared string
         */
        if(b->tail.mapped_string != NO_MAPPED_STRING) {
            return rb_str_dup(b->tail.mapped_string);
        }
        size_t sz = b->tail.last - b->tail.first;
        if(sz == 0) {
            return rb_str_buf_new(0);
#ifdef USE_STR_NEW_MOVE
        /* note: this code is magical: */
        } else if(sz > RSTRING_EMBED_LEN_MAX) {
            b->tail.mapped_string = msgpack_pool_static_move_to_string(
                    b->tail.first, 0, b->tail.last - b->tail.first);
            //VALUE mapped_string = rb_str_new_move(b->tail.first,
            //        b->tail.last - b->tail.first,
            //        b->tail_buffer_end - b->tail.first);
            //b->tail.mapped_string = mapped_string;
            return b->tail.mapped_string;
#endif
        }
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
#ifndef USE_STR_NEW_MOVE
            && b->tail.mapped_string != NO_MAPPED_STRING;
#endif
            ) {

        size_t offset;
        VALUE src;

        if(b->head == &b->tail && b->read_buffer == b->tail.first) {
            offset = 0;
            src = msgpack_buffer_all_as_string(b);
        } else {
            offset = b->read_buffer - b->head->first;
            src = _msgpack_buffer_chunk_as_string(b->head);
        }

		*dest = rb_str_substr(src, offset, length);

        _msgpack_buffer_consumed(b, length);

        return true;
    }

    return false;
}

VALUE msgpack_buffer_all_as_string_array(msgpack_buffer_t* b)
{
    VALUE ary = rb_ary_new();
    VALUE s;

    if(b->head == &b->tail && b->read_buffer == b->tail.first) {
        s = msgpack_buffer_all_as_string(b);
        rb_ary_push(ary, s);
        return ary;
    }

    msgpack_buffer_chunk_t* c = b->head;

    if(b->read_buffer == b->head->first) {
        s = _msgpack_buffer_chunk_as_string(b->head);
        rb_ary_push(ary, s);
        c = b->head->next;
        if(c == &b->tail) {
            return ary;
        }
    }

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
        char* mem = msgpack_pool_static_malloc(length, &capacity);

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

//msgpack_buffer_chunk_t* msgpack_buffer_append_chunk(msgpack_buffer_t* b, size_t required_length)
//{
//    if(tail->mapped_string != NO_MAPPED_STRING) {
//        // TODO realloc
//    } else {
//        // TODO alloc from pool
//        size_t bsize;
//        char* buf = msgpack_pool_allocate(required, &bsize);
//    }
//}

//msgpack_buffer_t* _msgpack_buffer_push_reference_chunk(msgpack_buffer_t* b, const char* data, size_t length, VALUE mapped_string)
//{
//    // TODO
//    //msgpack_buffer_chunk_t* tail = &b->array[b->array_size];
//    tail->data = data;
//    tail->length = length;
//    tail->mapped_string = mapped_string;
//    b->write_buffer = NULL;
//    b->write_buffer_size = 0;
//    return tail;
//}

//msgpack_buffer_chunk_t* msgpack_buffer_append_reference_chunk(msgpack_buffer_t* b, const char* data, size_t length, VALUE mapped_string)
//{
//}

//void msgpack_buffer_copy_last_reference_chunk(msgpack_buffer_t* b, msgpack_buffer_chunk_t* last_chunk)
//{
//    if(last_chunk->data == NULL) {
//        return;
//    }
//
//    size_t bsize;
//    char* copy = msgpack_pool_allocate(last_chunk->length, &bsize);
//    memcpy(copy, last_chunlast_chunk->data, last_chunk->length);
//    if(head == last_chunk) {
//        b->head_buffer = copy + (last_chunk->head_buffer - last_chunk->data);
//    }
//    if(tail == last_chunk) {
//        b->write_buffer = copy + (last_chunk->write_buffer - last_chunk->data);
//        b->write_buffer_size = bsize - last_chunk->length;
//    }
//    last_chunk->data = copy;
//}

