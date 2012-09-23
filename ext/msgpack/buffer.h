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

#ifndef MSGPACK_BUFFER_STRING_APPEND_REFERENCE_DEFAULT
#define MSGPACK_BUFFER_STRING_APPEND_REFERENCE_DEFAULT (1024*1024)
#endif

#ifndef MSGPACK_BUFFER_STRING_APPEND_REFERENCE_MINIMUM
#define MSGPACK_BUFFER_STRING_APPEND_REFERENCE_MINIMUM 256
#endif

#ifndef MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD
#define MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD 256
#endif

#if MSGPACK_BUFFER_STRING_APPEND_REFERENCE_MINIMUM <= 23
#error MSGPACK_BUFFER_STRING_APPEND_REFERENCE_MINIMUM must be > RSTRING_EMBED_LEN_MAX which is 11 or 23
#endif

#if MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD <= 23
#error MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD must be > RSTRING_EMBED_LEN_MAX which is 11 or 23
#endif

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
    void* mem;
    msgpack_buffer_chunk_t* next;
    VALUE mapped_string;  /* RBString or NO_MAPPED_STRING */
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

struct msgpack_buffer_t {
    char* read_buffer;
    char* tail_buffer_end;

    msgpack_buffer_chunk_t tail;
    msgpack_buffer_chunk_t* head;
    msgpack_buffer_chunk_t* free_list;

    char* rmem_last;
    char* rmem_end;
    void** rmem_owner;

    union msgpack_buffer_cast_block_t cast_block;

    size_t append_reference_threshold;

    VALUE owner;
};

/*
 * initialization functions
 */
void msgpack_buffer_static_init();

void msgpack_buffer_static_destroy();

void msgpack_buffer_init(msgpack_buffer_t* b);

void msgpack_buffer_destroy(msgpack_buffer_t* b);

void msgpack_buffer_mark(msgpack_buffer_t* b);

void msgpack_buffer_clear(msgpack_buffer_t* b);

static inline void msgpack_buffer_set_append_reference_threshold(msgpack_buffer_t* b, size_t length)
{
    if(length < MSGPACK_BUFFER_STRING_APPEND_REFERENCE_MINIMUM) {
        b->append_reference_threshold = MSGPACK_BUFFER_STRING_APPEND_REFERENCE_MINIMUM;
    } else {
        b->append_reference_threshold = length;
    }
}

/*
 * writer functions
 */

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

static inline void msgpack_buffer_ensure_sequential(msgpack_buffer_t* b, size_t require)
{
    if(require == 0) {
        return;
    }
    _msgpack_buffer_append2(b, NULL, require);
}

static inline void msgpack_buffer_append(msgpack_buffer_t* b, const char* data, size_t length)
{
    if(length == 0) {
        return;
    }

    if(length <= msgpack_buffer_writable_size(b)) {
        memcpy(b->tail.last, data, length);
        b->tail.last += length;
        return;
    }

    _msgpack_buffer_append2(b, data, length);
}

void _msgpack_buffer_append_reference(msgpack_buffer_t* b, const char* data, size_t length, VALUE mapped_string);

static inline void msgpack_buffer_append_string(msgpack_buffer_t* b, VALUE string)
{
    size_t length = RSTRING_LEN(string);
    if(length >= b->append_reference_threshold && !STR_DUP_LIKELY_DOES_COPY(string)) {
        VALUE mapped_string = rb_str_dup(string);
#ifdef COMPAT_HAVE_ENCODING
        ENCODING_SET(mapped_string, s_enc_ascii8bit);
#endif
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

bool _msgpack_buffer_shift_chunk(msgpack_buffer_t* b);

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
        _msgpack_buffer_shift_chunk(b);
//printf("shift %d tail_filled=%lu\n", i, b->tail.last - b->tail.first);
    }
}

static inline int msgpack_buffer_peek_1(msgpack_buffer_t* b)
{
    return (int) (unsigned char) b->read_buffer[0];
}

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


/*
 * bulk read / skip functions
 */

bool msgpack_buffer_read_all(msgpack_buffer_t* b, char* buffer, size_t length);

static inline bool msgpack_buffer_skip_all(msgpack_buffer_t* b, size_t length)
{
    return msgpack_buffer_read_all(b, NULL, length);
}

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

static inline VALUE _msgpack_buffer_refer_head_mapped_string(msgpack_buffer_t* b, size_t length)
{
    size_t offset = b->read_buffer - b->head->first;
    return rb_str_substr(b->head->mapped_string, offset, length);
}

static inline VALUE msgpack_buffer_read_top_as_string(msgpack_buffer_t* b, size_t length, bool suppress_reference)
{
    VALUE result;
    if(!suppress_reference &&
            b->head->mapped_string != NO_MAPPED_STRING &&
            length >= MSGPACK_BUFFER_READ_STRING_REFERENCE_THRESHOLD) {
        result = _msgpack_buffer_refer_head_mapped_string(b, length);
    } else {
        result = rb_str_new(b->read_buffer, length);
    }

    _msgpack_buffer_consumed(b, length);
    return result;
}


void msgpack_buffer_flush_to_io(msgpack_buffer_t* b, VALUE io, ID write_method);


#endif

