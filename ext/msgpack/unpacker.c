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

#include "unpacker.h"

#define HEAD_BYTE_REQUIRED 0xc6

static ID s_read; // TODO initialzie for read_all_data_from_io_to_string
static ID s_sysread;
static ID s_readpartial;

void msgpack_unpacker_static_init()
{
    s_read = rb_intern("read");
    s_sysread = rb_intern("sysread");
    s_readpartial = rb_intern("readpartial");
}

void msgpack_unpacker_init(msgpack_unpacker_t* uk)
{
    memset(uk, 0, sizeof(msgpack_unpacker_t));

    msgpack_buffer_init(UNPACKER_BUFFER_(uk));

    uk->head_byte = HEAD_BYTE_REQUIRED;

    uk->last_object = Qnil;
    uk->reading_raw = Qnil;
    uk->io = Qnil;
    uk->io_buffer = rb_str_buf_new(0);

    uk->stack = calloc(MSGPACK_UNPACKER_STACK_CAPACITY, sizeof(msgpack_unpacker_stack_t));
    uk->stack_capacity = MSGPACK_UNPACKER_STACK_CAPACITY;
}

void msgpack_unpacker_destroy(msgpack_unpacker_t* uk)
{
    free(uk->stack);
    msgpack_buffer_destroy(UNPACKER_BUFFER_(uk));
}

void msgpack_unpacker_mark(msgpack_unpacker_t* uk)
{
    rb_gc_mark(uk->last_object);
    rb_gc_mark(uk->reading_raw);
    rb_gc_mark(uk->io);
    rb_gc_mark(uk->io_buffer);

    msgpack_unpacker_stack_t* s = uk->stack;
    msgpack_unpacker_stack_t* send = uk->stack + uk->stack_depth;
    for(; s < send; s++) {
        rb_gc_mark(s->object);
        rb_gc_mark(s->key);
    }
}

/* stack funcs */
static inline msgpack_unpacker_stack_t* _msgpack_unpacker_stack_top(msgpack_unpacker_t* uk)
{
    return &uk->stack[uk->stack_depth-1];
}

static inline int _msgpack_unpacker_stack_push(msgpack_unpacker_t* uk, enum stack_type_t type, size_t count, VALUE object)
{
    if(uk->stack_capacity - uk->stack_depth <= 0) {
        return PRIMITIVE_STACK_TOO_DEEP;
    }

    msgpack_unpacker_stack_t* next = &uk->stack[uk->stack_depth];
    next->count = count;
    next->type = type;
    next->object = object;
    next->key = Qnil;

//printf("push count: %d depth:%d type:%d\n", count, uk->stack_depth, type);
    uk->stack_depth++;
    return PRIMITIVE_CONTAINER_START;
}

static inline VALUE msgpack_unpacker_stack_pop(msgpack_unpacker_t* uk)
{
    return --uk->stack_depth;
}

static inline bool msgpack_unpacker_stack_is_empty(msgpack_unpacker_t* uk)
{
    return uk->stack_depth == 0;
}

#ifdef USE_CASE_RANGE

#define SWITCH_RANGE_BEGIN(BYTE)     { switch(BYTE) {
#define SWITCH_RANGE(BYTE, FROM, TO) } case FROM ... TO: {
#define SWITCH_RANGE_DEFAULT         } default: {
#define SWITCH_RANGE_END             } }

#else

#define SWITCH_RANGE_BEGIN(BYTE)     { if(0) {
#define SWITCH_RANGE(BYTE, FROM, TO) } else if(FROM <= (BYTE) && (BYTE) <= TO) {
#define SWITCH_RANGE_DEFAULT         } else {
#define SWITCH_RANGE_END             } }

#endif

#ifndef MSGPACK_UNPACKER_IO_READ_SIZE
#define MSGPACK_UNPACKER_IO_READ_SIZE (64*1024)
#endif

static size_t feed_buffer_from_io(msgpack_unpacker_t* uk)
{
    rb_funcall(uk->io, uk->io_partial_read_method, 2, LONG2FIX(MSGPACK_UNPACKER_IO_READ_SIZE), uk->io_buffer);

    /* TODO zero-copy optimize */
    msgpack_buffer_append(UNPACKER_BUFFER_(uk), RSTRING_PTR(uk->io_buffer), RSTRING_LEN(uk->io_buffer));

    return RSTRING_LEN(uk->io_buffer);
}

static void read_all_data_from_io_to_string(msgpack_unpacker_t* uk, VALUE string, size_t length)
{
    if(RSTRING_LEN(string) == 0) {
        rb_funcall(uk->io, s_read, 2, LONG2FIX(length), string);
    } else {
        rb_funcall(uk->io, s_read, 2, LONG2FIX(length), uk->io_buffer);
        rb_str_buf_cat(string, (const void*)RSTRING_PTR(uk->io_buffer), RSTRING_LEN(uk->io_buffer));
    }
}

static union msgpack_buffer_cast_block_t* read_cast_block(msgpack_unpacker_t* uk, int n)
{
    /* TODO */
    if(uk->io == Qnil) {
        return NULL;
    }
    union msgpack_buffer_cast_block_t* cb;
    do {
        feed_buffer_from_io(uk);
        cb = msgpack_buffer_read_cast_block(UNPACKER_BUFFER_(uk), n);
    } while(cb == NULL);
}



#define READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, n) \
    union msgpack_buffer_cast_block_t* cb; \
    if(avail < n) { \
        cb = read_cast_block(uk, n); \
        if(cb == NULL) { \
            return PRIMITIVE_EOF; \
        } \
    } \
    cb = msgpack_buffer_refer_cast_block(UNPACKER_BUFFER_(uk), n); \

static int read_head_byte(msgpack_unpacker_t* uk)
{
    if(msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk)) <= 0) {
        if(uk->io == Qnil) {
            return PRIMITIVE_EOF;
        }
        feed_buffer_from_io(uk);
    }
//printf("%lu\n", msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk)));
    return msgpack_buffer_read_1(UNPACKER_BUFFER_(uk));
}

static inline int get_head_byte(msgpack_unpacker_t* uk)
{
    int b = uk->head_byte;
    if(b == HEAD_BYTE_REQUIRED) {
        return read_head_byte(uk);
    }
    return b;
}

static inline void reset_head_byte(msgpack_unpacker_t* uk)
{
    uk->head_byte = HEAD_BYTE_REQUIRED;
}

static inline int object_complete(msgpack_unpacker_t* uk, VALUE object)
{
    uk->last_object = object;
    reset_head_byte(uk);
    return PRIMITIVE_OBJECT_COMPLETE;
}

static int read_raw_body_cont(msgpack_unpacker_t* uk)
{
    /* try zero-copy */
    if(uk->reading_raw == Qnil || RSTRING_LEN(uk->reading_raw) == 0) {
        VALUE string;
        if(msgpack_buffer_try_refer_string(UNPACKER_BUFFER_(uk), uk->reading_raw_remaining, &string)) {
            object_complete(uk, string);
            uk->reading_raw = Qnil;
            uk->reading_raw_remaining = 0;
            return PRIMITIVE_OBJECT_COMPLETE;
        }
        uk->reading_raw = rb_str_buf_new(uk->reading_raw_remaining);
    }

    if(uk->io != Qnil) {
        if(msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk)) > 0) {
            /* read from buffer */
            size_t n = msgpack_buffer_read_to_string(UNPACKER_BUFFER_(uk), uk->reading_raw, uk->reading_raw_remaining);
            uk->reading_raw_remaining -= n;

            if(uk->reading_raw_remaining == 0) {
                object_complete(uk, uk->reading_raw);
                uk->reading_raw = Qnil;
                return PRIMITIVE_OBJECT_COMPLETE;
            }
        }

        /* read from io */
        read_all_data_from_io_to_string(uk, uk->reading_raw, uk->reading_raw_remaining);
        uk->reading_raw_remaining = 0;

        object_complete(uk, uk->reading_raw);
        uk->reading_raw = Qnil;
        return PRIMITIVE_OBJECT_COMPLETE;

    } else {
        size_t n = msgpack_buffer_read_to_string(UNPACKER_BUFFER_(uk), uk->reading_raw, uk->reading_raw_remaining);
        uk->reading_raw_remaining -= n;

        if(uk->reading_raw_remaining > 0) {
            return PRIMITIVE_EOF;
        }

        object_complete(uk, uk->reading_raw);
        uk->reading_raw = Qnil;
        return PRIMITIVE_OBJECT_COMPLETE;
    }
}

static int read_primitive(msgpack_unpacker_t* uk)
{
    if(uk->reading_raw_remaining > 0) {
        return read_raw_body_cont(uk);
    }

    int b = get_head_byte(uk);
    if(b < 0) {
        return b;
    }
    //printf("b:%d\n", b);

    SWITCH_RANGE_BEGIN(b)
    SWITCH_RANGE(b, 0x00, 0x7f)  // Positive Fixnum
        return object_complete(uk, INT2FIX(b));

    SWITCH_RANGE(b, 0xe0, 0xff)  // Negative Fixnum
        return object_complete(uk, INT2FIX((int8_t)b));

    SWITCH_RANGE(b, 0xa0, 0xbf)  // FixRaw
        int count = b & 0x1f;
        if(count == 0) {
            return object_complete(uk, rb_str_buf_new(0));
        }
        //uk->reading_raw = rb_str_buf_new(count);
        uk->reading_raw_remaining = count;
        return read_raw_body_cont(uk);

    SWITCH_RANGE(b, 0x90, 0x9f)  // FixArray
        int count = b & 0x0f;
        if(count == 0) {
            return object_complete(uk, rb_ary_new());
        }
//printf("fix array %d\n", count);
        return _msgpack_unpacker_stack_push(uk, STACK_TYPE_ARRAY, count, rb_ary_new2(count));

    SWITCH_RANGE(b, 0x80, 0x8f)  // FixMap
        int count = b & 0x0f;
        if(count == 0) {
            return object_complete(uk, rb_hash_new());
        }
//printf("fix map %d %x\n", count, b);
        return _msgpack_unpacker_stack_push(uk, STACK_TYPE_MAP, count*2, rb_hash_new());

    SWITCH_RANGE(b, 0xc0, 0xdf)  // Variable
        size_t avail = msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk));

        switch(b) {
        case 0xc0:  // nil
            return object_complete(uk, Qnil);

        //case 0xc1:  // string

        case 0xc2:  // false
            return object_complete(uk, Qfalse);

        case 0xc3:  // true
            return object_complete(uk, Qtrue);

        //case 0xc4:
        //case 0xc5:
        //case 0xc6:
        //case 0xc7:
        //case 0xc8:
        //case 0xc9:

        case 0xca:  // float
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 4);
                cb->u32 = _msgpack_be32(cb->u32);
                return object_complete(uk, rb_float_new(cb->f));
            }

        case 0xcb:  // double
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 8);
                cb->u64 = _msgpack_be64(cb->u64);
                return object_complete(uk, rb_float_new(cb->d));
            }

            //int n = 1 << (((unsigned int)*p) & 0x03)
        case 0xcc:  // unsigned int  8
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 1);
                return cb->u8;
            }

        case 0xcd:  // unsigned int 16
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
                return _msgpack_be16(cb->u16);
            }

        case 0xce:  // unsigned int 32
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 4);
                return _msgpack_be32(cb->u32);
            }

        case 0xcf:  // unsigned int 64
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 8);
                // TODO
                uint64_t u64 = _msgpack_be64(cb->u64);
                return object_complete(uk, Qnil);
            }

        case 0xd0:  // signed int  8
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 1);
                return cb->i8;
            }

        case 0xd1:  // signed int 16
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
                return _msgpack_be16(cb->i16);
            }

        case 0xd2:  // signed int 32
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 4);
                return _msgpack_be32(cb->i32);
            }

        case 0xd3:  // signed int 64
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 8);
                // TODO
                int64_t i64 = _msgpack_be64(cb->i64);
                return object_complete(uk, Qnil);
            }

        //case 0xd4:
        //case 0xd5:
        //case 0xd6:  // big integer 16
        //case 0xd7:  // big integer 32
        //case 0xd8:  // big float 16
        //case 0xd9:  // big float 32

            //int n = 2 << (((unsigned int)*p) & 0x01);
        case 0xda:  // raw 16
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
                uint16_t count = _msgpack_be16(cb->u16);
                if(count == 0) {
                    return object_complete(uk, rb_str_buf_new(0));
                }
                //uk->reading_raw = rb_str_buf_new(count);
                uk->reading_raw_remaining = count;
                return read_raw_body_cont(uk);
            }

        case 0xdb:  // raw 32
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 4);
                uint32_t count = _msgpack_be32(cb->u32);
                if(count == 0) {
                    return object_complete(uk, rb_str_buf_new(0));
                }
                //uk->reading_raw = rb_str_buf_new(count);
                uk->reading_raw_remaining = count;
                return read_raw_body_cont(uk);
            }

        case 0xdc:  // array 16
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
                uint16_t count = _msgpack_be16(cb->u16);
                if(count == 0) {
                    return object_complete(uk, rb_ary_new());
                }
                return _msgpack_unpacker_stack_push(uk, STACK_TYPE_ARRAY, count, rb_ary_new2(count));
            }

        case 0xdd:  // array 32
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 4);
                uint32_t count = _msgpack_be32(cb->u32);
                if(count == 0) {
                    return object_complete(uk, rb_ary_new());
                }
                return _msgpack_unpacker_stack_push(uk, STACK_TYPE_ARRAY, count, rb_ary_new2(count));
            }

        case 0xde:  // map 16
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
                uint16_t count = _msgpack_be16(cb->u16);
                if(count == 0) {
                    return object_complete(uk, rb_hash_new());
                }
                return _msgpack_unpacker_stack_push(uk, STACK_TYPE_MAP, count*2, rb_hash_new());
            }

        case 0xdf:  // map 32
            {
                READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 4);
                uint32_t count = _msgpack_be32(cb->u32);
                if(count == 0) {
                    return object_complete(uk, rb_hash_new());
                }
                return _msgpack_unpacker_stack_push(uk, STACK_TYPE_MAP, count*2, rb_hash_new());
            }

        default:
            printf("invalid byte: %x\n", b);
            return PRIMITIVE_INVALID_BYTE;
        }

    SWITCH_RANGE_DEFAULT
        printf("invalid byte: %x\n", b);
        return PRIMITIVE_INVALID_BYTE;

    SWITCH_RANGE_END
}

int msgpack_unpacker_read_array_header(msgpack_unpacker_t* uk)
{
    int b = get_head_byte(uk);

    // TODO
    //READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
    //int count = _msgpack_be16(cb->u16);
}

int msgpack_unpacker_read_map_header(msgpack_unpacker_t* uk)
{
    // TODO
    //READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, avail, 2);
    //int count = _msgpack_be16(cb->u16);
}

int msgpack_unpacker_read(msgpack_unpacker_t* uk, size_t target_stack_depth)
{
    while(true) {
        int r = read_primitive(uk);
        if(r < 0) {
            return r;
        }
        if(r == PRIMITIVE_CONTAINER_START) {
            continue;
        }
        /* PRIMITIVE_OBJECT_COMPLETE */

        if(msgpack_unpacker_stack_is_empty(uk)) {
            return PRIMITIVE_OBJECT_COMPLETE;
        }

        container_completed:
        {
            msgpack_unpacker_stack_t* top = _msgpack_unpacker_stack_top(uk);
            switch(top->type) {
            case STACK_TYPE_ARRAY:
                rb_ary_push(top->object, uk->last_object);
                break;
            case STACK_TYPE_MAP:
                if(top->count % 2 == 0) {
                    top->key = uk->last_object;
                } else {
                    rb_hash_aset(top->object, top->key, uk->last_object);
                }
                break;
            }
            size_t count = --top->count;
//printf("calc count: %d  depth=%d\n", count, uk->stack_depth);

            if(count == 0) {
                object_complete(uk, top->object);
                if(msgpack_unpacker_stack_pop(uk) <= target_stack_depth) {
                    return PRIMITIVE_OBJECT_COMPLETE;
                }
                goto container_completed;
            }
        }
    }
}

int msgpack_unpacker_skip(msgpack_unpacker_t* uk, size_t target_stack_depth)
{
    while(true) {
        int r = read_primitive(uk);
        if(r < 0) {
            return r;
        }
        if(r == PRIMITIVE_CONTAINER_START) {
            continue;
        }
        /* PRIMITIVE_OBJECT_COMPLETE */

        if(msgpack_unpacker_stack_is_empty(uk)) {
            return PRIMITIVE_OBJECT_COMPLETE;
        }

        container_completed:
        {
            msgpack_unpacker_stack_t* top = _msgpack_unpacker_stack_top(uk);
            /* deleted section */
            /* TODO optimize: #define SKIP and #include unpacker_incl.h */
            size_t count = --top->count;

            if(count == 0) {
                object_complete(uk, Qnil);
                if(msgpack_unpacker_stack_pop(uk) <= target_stack_depth) {
                    return PRIMITIVE_OBJECT_COMPLETE;
                }
                goto container_completed;
            }
        }
    }
}

int get_next_object_type(msgpack_unpacker_t* uk)
{
    int b = get_head_byte(uk);
    if(b < 0) {
        return b;
    }
    // TODO swtich
    // TODO raise EOFError if buffer is empty
}

int msgpack_unpacker_skip_nil(msgpack_unpacker_t* uk)
{
    // TODO
    int b = get_head_byte(uk);
    if(b < 0) {
        return b;
    }
    if(b == 0xc0) {
        return 1;
    }
    return 0;
}

