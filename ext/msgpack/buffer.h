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
#ifndef MSGPACK_RUBY_BUFFER_H__
#define MSGPACK_RUBY_BUFFER_H__

#include "compat.h"
#include "sysdep.h"
#include "pool.h"

#ifndef MSGPACK_BUFFER_STRING_APPEND_REFERENCE_THRESHOLD
#define MSGPACK_BUFFER_STRING_APPEND_REFERENCE_THRESHOLD (1024*1024)
#endif

#ifndef MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD
#define MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD 256
#endif

// TODO
//#if MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD < RSTRING_EMBED_LEN_MAX
//#undef MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD
//#define MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD RSTRING_EMBED_LEN_MAX
//#endif

#define NO_MAPPED_STRING ((VALUE)0)

struct msgpack_buffer_chunk_t;
typedef struct msgpack_buffer_chunk_t msgpack_buffer_chunk_t;

struct msgpack_buffer_t;
typedef struct msgpack_buffer_t msgpack_buffer_t;

/*
 * msgpack_buffer_chunk_t
 * +----------------+
 * | filled  | free |
 * +---------+------+
 * ^ first   ^ last
 */
struct msgpack_buffer_chunk_t {
    char* first;
    char* last;
    VALUE mapped_string;  /* RBString or NO_MAPPED_STRING */
    msgpack_buffer_chunk_t* next;
};

union msgpack_buffer_cast_block_t {
    char buffer[8];
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f;
    double d;
};

/*
 * head
 * +----------------+
 * | filled  | free |
 * +---------+------+
 * ^ first   ^ last
 *     ^ read_buffer -> read()
 *     +-----+
 *     last - read_buffer = top_readable_size
 * +---+
 * read_buffer - first = read_offset
 *
 * tail
 * +----------------+
 * | filled  | free |
 * +---------+------+
 * ^ first   ^ last -> write()
 *                  ^ tail_buffer_end
 *           +------+
 *           tail_buffer_end - last = writable_size
 */
struct msgpack_buffer_t {
    char* read_buffer;
    char* tail_buffer_end;

    msgpack_buffer_chunk_t tail;
    msgpack_buffer_chunk_t* head;
    msgpack_buffer_chunk_t* free_list;

    union msgpack_buffer_cast_block_t cast_block;
};

/*
 * initialization functions
 */
void msgpack_buffer_init(msgpack_buffer_t* b);

void msgpack_buffer_destroy(msgpack_buffer_t* b);

void msgpack_buffer_mark(msgpack_buffer_t* b);


/*
 * writer functions
 */

void _msgpack_buffer_add_new_chunk(msgpack_buffer_t* b);

static inline size_t msgpack_buffer_writable_size(const msgpack_buffer_t* b)
{
    return b->tail_buffer_end - b->tail.last;
}

static inline void msgpack_buffer_write_1(msgpack_buffer_t* b, int byte)
{
    (*b->tail.last++) = (char) byte;
}

static inline void msgpack_buffer_write_2(msgpack_buffer_t* b, int byte1, unsigned char byte2)
{
    *(b->tail.last++) = (char) byte1;
    *(b->tail.last++) = (char) byte2;
}

static inline void msgpack_buffer_write_byte_and_data(msgpack_buffer_t* b, int byte, const void* data, size_t length)
{
    (*b->tail.last++) = (char) byte;

    memcpy(b->tail.last, data, length);
    b->tail.last += length;
}

void _msgpack_buffer_append2(msgpack_buffer_t* b, const char* data, size_t length);

static inline void msgpack_buffer_expand(msgpack_buffer_t* b, size_t require)
{
    _msgpack_buffer_append2(b, NULL, require);
}

static inline void msgpack_buffer_append(msgpack_buffer_t* b, const char* data, size_t length)
{
    if(length == 0) {
        return;
    }

    if(msgpack_buffer_writable_size(b) < length) {
        _msgpack_buffer_append2(b, data, length);
        return;
    }

    memcpy(b->tail.last, data, length);
    b->tail.last += length;
}

void _msgpack_buffer_append_reference(msgpack_buffer_t* b, const char* data, size_t length, VALUE mapped_string);

static inline void msgpack_buffer_append_string(msgpack_buffer_t* b, VALUE string)
{
    size_t length = RSTRING_LEN(string);
    /* if FL_ALL(string, FL_USER1|FL_USER3) == STR_ASSOC_P(string) returns true, rb_str_dup will copy the string */
    if(length >= MSGPACK_BUFFER_STRING_APPEND_REFERENCE_THRESHOLD && !FL_ALL(string, FL_USER1|FL_USER3)) {
        VALUE mapped_string = rb_str_dup(string);
        _msgpack_buffer_append_reference(b, RSTRING_PTR(mapped_string), length, mapped_string);
    } else {
        msgpack_buffer_append(b, RSTRING_PTR(string), length);
    }
}


/*
 * reader functions
 */

static inline size_t msgpack_buffer_top_readable_size(const msgpack_buffer_t* b)
{
    return b->head->last - b->read_buffer;
}

size_t msgpack_buffer_all_readable_size(const msgpack_buffer_t* b);

bool _msgpack_buffer_pop_chunk(msgpack_buffer_t* b);

static inline void _msgpack_buffer_consumed(msgpack_buffer_t* b, size_t length)
{
    b->read_buffer += length;
    if(b->read_buffer >= b->head->last) {
//size_t i = 0;
//msgpack_buffer_chunk_t* c = b->head;
//while(c != &b->tail) {
//    c = c->next;
//    i++;
//}
        _msgpack_buffer_pop_chunk(b);
//printf("pop %d tail_filled=%lu\n", i, b->tail.last - b->tail.first);
    }
}

static inline int msgpack_buffer_peek_1(msgpack_buffer_t* b)
{
    return (int) (unsigned char) b->read_buffer[0];
}

union msgpack_buffer_cast_block_t* _msgpack_buffer_build_cast_block(msgpack_buffer_t* b, size_t n);

static inline int msgpack_buffer_read_1(msgpack_buffer_t* b)
{
    int r = (int) (unsigned char) b->read_buffer[0];

    _msgpack_buffer_consumed(b, 1);

    return r;
}

static inline union msgpack_buffer_cast_block_t* msgpack_buffer_refer_cast_block(msgpack_buffer_t* b, size_t n)
{
    memcpy(b->cast_block.buffer, b->read_buffer, n);

    _msgpack_buffer_consumed(b, n);

    return &b->cast_block;
}

bool msgpack_buffer_read_all(msgpack_buffer_t* b, char* buffer, size_t length);

bool msgpack_buffer_skip_all(msgpack_buffer_t* b, size_t length);

static inline union msgpack_buffer_cast_block_t* msgpack_buffer_read_cast_block(msgpack_buffer_t* b, size_t n)
{
    if(msgpack_buffer_top_readable_size(b) < n) {
        if(msgpack_buffer_read_all(b, b->cast_block.buffer, n)) {
            return &b->cast_block;
        }
        return NULL;
    }
    return msgpack_buffer_refer_cast_block(b, n);
}

size_t msgpack_buffer_read(msgpack_buffer_t* b, char* buffer, size_t length);

size_t msgpack_buffer_read_to_string(msgpack_buffer_t* b, VALUE string, size_t length);

VALUE msgpack_buffer_all_as_string(msgpack_buffer_t* b);

VALUE msgpack_buffer_all_as_string_array(msgpack_buffer_t* b);

bool msgpack_buffer_try_refer_string(msgpack_buffer_t* b, size_t length, VALUE* dest);

//static inline int msgpack_buffer_remaining(msgpack_buffer_t* b)
//{
//    size_t rem = b->head_buffer_size;
//    msgpack_buffer_chunk_t* c = b->head;
//    msgpack_buffer_chunk_t* const tail = b->tail;
//    while(c != c->tail) {
//        c = c->next;  /* skip head buffer */
//        rem += c->length;
//    }
//    return rem;
//}

#endif

