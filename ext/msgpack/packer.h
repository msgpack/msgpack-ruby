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
#ifndef MSGPACK_RUBY_PACKER_H__
#define MSGPACK_RUBY_PACKER_H__

#include "buffer.h"

#ifndef MSGPACK_PACKER_IO_FLUSH_THRESHOLD_TO_WRITE_STRING_BODY
#define MSGPACK_PACKER_IO_FLUSH_THRESHOLD_TO_WRITE_STRING_BODY (1024)
#endif

struct msgpack_packer_t;
typedef struct msgpack_packer_t msgpack_packer_t;

struct msgpack_packer_t {
    msgpack_buffer_t buffer;

    VALUE io;
    ID io_write_all_method;

    ID to_msgpack_method;
    VALUE to_msgpack_arg;

    VALUE buffer_ref;
};

#define PACKER_BUFFER_(pk) (&(pk)->buffer)

void msgpack_packer_static_init();

void msgpack_packer_static_destroy();

void msgpack_packer_init(msgpack_packer_t* pk);

void msgpack_packer_destroy(msgpack_packer_t* pk);

void msgpack_packer_mark(msgpack_packer_t* pk);

static inline void msgpack_packer_set_to_msgpack_method(msgpack_packer_t* pk,
        ID to_msgpack_method, VALUE to_msgpack_arg)
{
    pk->to_msgpack_method = to_msgpack_method;
    pk->to_msgpack_arg = to_msgpack_arg;
}

static inline void msgpack_packer_set_io(msgpack_packer_t* pk, VALUE io, ID io_write_all_method)
{
    pk->io = io;
    pk->io_write_all_method = io_write_all_method;
}

void msgpack_packer_reset(msgpack_packer_t* pk);


static inline void msgpack_packer_write_nil(msgpack_packer_t* pk)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
    msgpack_buffer_write_1(PACKER_BUFFER_(pk), 0xc0);
}

static inline void msgpack_packer_write_true(msgpack_packer_t* pk)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
    msgpack_buffer_write_1(PACKER_BUFFER_(pk), 0xc3);
}

static inline void msgpack_packer_write_false(msgpack_packer_t* pk)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
    msgpack_buffer_write_1(PACKER_BUFFER_(pk), 0xc2);
}

static inline void _msgpack_packer_write_fixint(msgpack_packer_t* pk, int8_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
    msgpack_buffer_write_1(PACKER_BUFFER_(pk), v);
}

static inline void _msgpack_packer_write_uint8(msgpack_packer_t* pk, uint8_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 2);
    msgpack_buffer_write_2(PACKER_BUFFER_(pk), 0xcc, v);
}

static inline void _msgpack_packer_write_uint16(msgpack_packer_t* pk, uint16_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 3);
    uint16_t be = _msgpack_be16(v);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xcd, (const void*)&be, 2);
}

static inline void _msgpack_packer_write_uint32(msgpack_packer_t* pk, uint32_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 5);
    uint32_t be = _msgpack_be32(v);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xce, (const void*)&be, 4);
}

static inline void _msgpack_packer_write_uint64(msgpack_packer_t* pk, uint64_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 9);
    uint64_t be = _msgpack_be64(v);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xcf, (const void*)&be, 8);
}

static inline void _msgpack_packer_write_int8(msgpack_packer_t* pk, int8_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 2);
    msgpack_buffer_write_2(PACKER_BUFFER_(pk), 0xd0, v);
}

static inline void _msgpack_packer_write_int16(msgpack_packer_t* pk, int16_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 3);
    uint16_t be = _msgpack_be16(v);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xd1, (const void*)&be, 2);
}

static inline void _msgpack_packer_write_int32(msgpack_packer_t* pk, int32_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 5);
    uint32_t be = _msgpack_be32(v);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xd2, (const void*)&be, 4);
}

static inline void _msgpack_packer_write_int64(msgpack_packer_t* pk, int64_t v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 9);
    uint64_t be = _msgpack_be64(v);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xd3, (const void*)&be, 8);
}

static inline void _msgpack_packer_write_long32(msgpack_packer_t* pk, long v)
{
    if(v < -0x20L) {
        if(v < -0x8000L) {
            _msgpack_packer_write_int32(pk, (int32_t) v);
        } else if(v < -0x80L) {
            _msgpack_packer_write_int16(pk, (int16_t) v);
        } else {
            _msgpack_packer_write_int8(pk, (int8_t) v);
        }
    } else if(v <= 0x7fL) {
        _msgpack_packer_write_fixint(pk, (int8_t) v);
    } else {
        if(v <= 0xffL) {
            _msgpack_packer_write_uint8(pk, (uint8_t) v);
        } else if(v <= 0xffffL) {
            _msgpack_packer_write_uint16(pk, (uint16_t) v);
        } else {
            _msgpack_packer_write_uint32(pk, (uint32_t) v);
        }
    }
}

static inline void _msgpack_packer_write_long_long64(msgpack_packer_t* pk, long long v)
{
    if(v < -0x20LL) {
        if(v < -0x8000LL) {
            if(v < -0x80000000LL) {
                _msgpack_packer_write_int64(pk, (int64_t) v);
            } else {
                _msgpack_packer_write_int32(pk, (int32_t) v);
            }
        } else {
            if(v < -0x80LL) {
                _msgpack_packer_write_int16(pk, (int16_t) v);
            } else {
                _msgpack_packer_write_int8(pk, (int8_t) v);
            }
        }
    } else if(v <= 0x7fLL) {
        _msgpack_packer_write_fixint(pk, (int8_t) v);
    } else {
        if(v <= 0xffffLL) {
            if(v <= 0xffLL) {
                _msgpack_packer_write_uint8(pk, (uint8_t) v);
            } else {
                _msgpack_packer_write_uint16(pk, (uint16_t) v);
            }
        } else {
            if(v <= 0xffffffffLL) {
                _msgpack_packer_write_uint32(pk, (uint32_t) v);
            } else {
                _msgpack_packer_write_uint64(pk, (uint64_t) v);
            }
        }
    }
}

static inline void msgpack_packer_write_long(msgpack_packer_t* pk, long v)
{
#if defined(SIZEOF_LONG)
#  if SIZEOF_LONG <= 4
    _msgpack_packer_write_long32(pk, v);
#  else
    _msgpack_packer_write_long_long64(pk, v);
#  endif

#elif defined(LONG_MAX)
#  if LONG_MAX <= 0x7fffffffL
    _msgpack_packer_write_long32(pk, v);
#  else
    _msgpack_packer_write_long_long64(pk, v);
#  endif

#else
    if(sizeof(long) <= 4) {
        _msgpack_packer_write_long32(pk, v);
    } else {
        _msgpack_packer_write_long_long64(pk, v);
    }
#endif
}

static inline void msgpack_packer_write_long_long(msgpack_packer_t* pk, long long v)
{
    /* assuming sizeof(long long) == 8 */
    _msgpack_packer_write_long_long64(pk, v);
}

static inline void msgpack_packer_write_u64(msgpack_packer_t* pk, uint64_t v)
{
    if(v <= 0xffULL) {
        if(v <= 0x7fULL) {
            _msgpack_packer_write_fixint(pk, (int8_t) v);
        } else {
            _msgpack_packer_write_uint8(pk, (uint8_t) v);
        }
    } else {
        if(v <= 0xffffULL) {
            _msgpack_packer_write_uint16(pk, (uint16_t) v);
        } else if(v <= 0xffffffffULL) {
            _msgpack_packer_write_uint32(pk, (uint32_t) v);
        } else {
            _msgpack_packer_write_uint64(pk, (uint64_t) v);
        }
    }
}

static inline void msgpack_packer_write_double(msgpack_packer_t* pk, double v)
{
    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 9);
    union {
        double d;
        uint64_t u64;
        char mem[8];
    } castbuf = { v };
    castbuf.u64 = _msgpack_be_double(castbuf.u64);
    msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xcb, castbuf.mem, 8);
}

static inline void msgpack_packer_write_raw_header(msgpack_packer_t* pk, unsigned int n)
{
    if(n < 32) {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
        unsigned char h = 0xa0 | (uint8_t) n;
        msgpack_buffer_write_1(PACKER_BUFFER_(pk), h);
    } else if(n < 65536) {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 3);
        uint16_t be = _msgpack_be16(n);
        msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xda, (const void*)&be, 2);
    } else {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 5);
        uint32_t be = _msgpack_be32(n);
        msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xdb, (const void*)&be, 4);
    }
}

static inline void msgpack_packer_write_array_header(msgpack_packer_t* pk, unsigned int n)
{
    if(n < 16) {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
        unsigned char h = 0x90 | (uint8_t) n;
        msgpack_buffer_write_1(PACKER_BUFFER_(pk), h);
    } else if(n < 65536) {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 3);
        uint16_t be = _msgpack_be16(n);
        msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xdc, (const void*)&be, 2);
    } else {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 5);
        uint32_t be = _msgpack_be32(n);
        msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xdd, (const void*)&be, 4);
    }
}

static inline void msgpack_packer_write_map_header(msgpack_packer_t* pk, unsigned int n)
{
    if(n < 16) {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 1);
        unsigned char h = 0x80 | (uint8_t) n;
        msgpack_buffer_write_1(PACKER_BUFFER_(pk), h);
    } else if(n < 65536) {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 3);
        uint16_t be = _msgpack_be16(n);
        msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xde, (const void*)&be, 2);
    } else {
        msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), 5);
        uint32_t be = _msgpack_be32(n);
        msgpack_buffer_write_byte_and_data(PACKER_BUFFER_(pk), 0xdf, (const void*)&be, 4);
    }
}


void _msgpack_packer_write_string_to_io(msgpack_packer_t* pk, VALUE string);

static inline void msgpack_packer_write_string_value(msgpack_packer_t* pk, VALUE v)
{
    /* TODO encoding conversion? */
    /* actual return type of RSTRING_LEN is long */
    unsigned long len = RSTRING_LEN(v);
    if(len > 0xffffffffUL) {
        // TODO rb_eArgError?
        rb_raise(rb_eArgError, "size of string is too long to pack: %lu bytes should be <= %lu", len, 0xffffffffUL);
    }
    msgpack_packer_write_raw_header(pk, (unsigned int)len);
    msgpack_buffer_append_string(PACKER_BUFFER_(pk), v);
}

static inline void msgpack_packer_write_symbol_value(msgpack_packer_t* pk, VALUE v)
{
    const char* name = rb_id2name(SYM2ID(v));
    /* actual return type of strlen is size_t */
    unsigned long len = strlen(name);
    if(len > 0xffffffffUL) {
        // TODO rb_eArgError?
        rb_raise(rb_eArgError, "size of symbol is too long to pack: %lu bytes should be <= %lu", len, 0xffffffffUL);
    }
    msgpack_packer_write_raw_header(pk, (unsigned int)len);
    msgpack_buffer_append(PACKER_BUFFER_(pk), name, len);
}

static inline void msgpack_packer_write_fixnum_value(msgpack_packer_t* pk, VALUE v)
{
#ifdef JRUBY
    msgpack_packer_write_long(pk, FIXNUM_P(v) ? FIX2LONG(v) : rb_num2ll(v));
#else
    msgpack_packer_write_long(pk, FIX2LONG(v));
#endif
}

static inline void msgpack_packer_write_bignum_value(msgpack_packer_t* pk, VALUE v)
{
    if(RBIGNUM_POSITIVE_P(v)) {
        msgpack_packer_write_u64(pk, rb_big2ull(v));
    } else {
        msgpack_packer_write_long_long(pk, rb_big2ll(v));
    }
}

static inline void msgpack_packer_write_float_value(msgpack_packer_t* pk, VALUE v)
{
    msgpack_packer_write_double(pk, rb_num2dbl(v));
}

void msgpack_packer_write_array_value(msgpack_packer_t* pk, VALUE v);

void msgpack_packer_write_hash_value(msgpack_packer_t* pk, VALUE v);

void msgpack_packer_write_value(msgpack_packer_t* pk, VALUE v);


#endif

