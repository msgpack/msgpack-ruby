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

#include "compat.h"
#include "ruby.h"
#include "buffer.h"

#define BUFFER(from, name) \
    msgpack_buffer_t *name = NULL; \
    Data_Get_Struct(from, msgpack_buffer_t, name); \
    if(name == NULL) { \
        rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
    }

#define CHECK_STRING_TYPE(value) \
    value = rb_check_string_type(value); \
    if( NIL_P(value) ) { \
        rb_raise(rb_eTypeError, "instance of String needed"); \
    }

static void Buffer_free(void* data)
{
    if(data == NULL) {
        return;
    }
    msgpack_buffer_t* b = (msgpack_buffer_t*) data;
    msgpack_buffer_destroy(b);
    free(b);
}

static VALUE Buffer_alloc(VALUE klass)
{
    msgpack_buffer_t* b = ALLOC_N(msgpack_buffer_t, 1);
    msgpack_buffer_init(b);

    return Data_Wrap_Struct(klass, msgpack_buffer_mark, Buffer_free, b);
}

/**
 * Document-method: MessagePack::Buffer#initialize
 *
 * call-seq:
 *   MessagePack::Buffer.new
 *
 * Creates an instance of the MessagePack::Buffer.
 *
 */
static VALUE Buffer_initialize(VALUE self)
{
    return self;
}

/**
 * Document-method: MessagePack::Buffer#size
 *
 * call-seq:
 *   MessagePack::Buffer#size
 *
 *
 */
static VALUE Buffer_size(VALUE self)
{
    BUFFER(self, b);
    size_t size = msgpack_buffer_all_readable_size(b);
    return LONG2FIX(size);
}

/**
 * Document-method: MessagePack::Buffer#empty?
 *
 * call-seq:
 *   MessagePack::Buffer#empty?
 *
 *
 */
static VALUE Buffer_empty_p(VALUE self)
{
    BUFFER(self, b);
    if(msgpack_buffer_all_readable_size(b) == 0) {
        return Qtrue;
    } else {
        return Qfalse;
    }
}

/**
 * Document-method: MessagePack::Buffer#append
 *
 * call-seq:
 *   MessagePack::Buffer#append(string_or_buffer)
 *
 *
 */
static VALUE Buffer_append(VALUE self, VALUE string_or_buffer)
{
    BUFFER(self, b);

    // TODO if string_or_buffer is a Buffer
    VALUE string = string_or_buffer;

    StringValue(string);

    msgpack_buffer_append_string(b, string);

    return self;
}

/**
 * Document-method: MessagePack::Buffer#skip
 *
 * call-seq:
 *   MessagePack::Buffer#skip(n)
 *
 *
 */
static VALUE Buffer_skip(VALUE self, VALUE n)
{

    BUFFER(self, b);

    long sn = FIX2LONG(n);

    /* do nothing */
    if(sn == 0) {
        return Qnil;
    }

    /* skip all or raise */
    if(!msgpack_buffer_skip_all(b, sn)) {
        rb_raise(rb_eEOFError, "end of buffer reached");
    }
    return Qnil;
}

/**
 * Document-method: MessagePack::Buffer#read
 *
 * call-seq:
 *   MessagePack::Buffer#read(n=nil, out='')
 *
 *
 */
static VALUE Buffer_read(int argc, VALUE* argv, VALUE self)
{
    VALUE out = Qnil;
    long n = -1;

    switch(argc) {
    case 2:
        out = argv[1];
        /* pass through */
    case 1:
        if(argv[0] != Qnil) {
            n = FIX2LONG(argv[0]);
            if(n < 0) {
                rb_raise(rb_eArgError, "negative length %ld given", n);
            }
            break;
        }
        /* pass through */
    case 0:
        break;
    default:
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..2)", argc);
    }

    BUFFER(self, b);

    if(out != Qnil) {
        CHECK_STRING_TYPE(out);
	    rb_str_resize(out, 0);
    }

    /* do nothing */
    if(n == 0) {
        if(out == Qnil) {
            out = rb_str_buf_new(0);
        }
        return out;
    }

    size_t sz = msgpack_buffer_all_readable_size(b);

    if(n < 0) {
        /* read all available */
        n = sz;

    } else if(sz < (size_t) n) {
        /* read all or raise */
        rb_raise(rb_eEOFError, "end of buffer reached");
    }

    if(out == Qnil) {
        out = rb_str_buf_new(n);
    }

    msgpack_buffer_read_to_string(b, out, n);

    return out;
}

/**
 * Document-method: MessagePack::Buffer#readpartial
 *
 * call-seq:
 *   MessagePack::Buffer#readpartial(n=nil, out='')
 *
 *
 */
static VALUE Buffer_readpartial(int argc, VALUE* argv, VALUE self)
{
    VALUE out = Qnil;
    long n = -1;

    switch(argc) {
    case 2:
        out = argv[1];
        /* pass through */
    case 1:
        if(argv[0] != Qnil) {
            n = FIX2LONG(argv[0]);
            if(n < 0) {
                rb_raise(rb_eArgError, "negative length %ld given", n);
            }
            break;
        }
        /* pass through */
    case 0:
        break;
    default:
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..2)", argc);
    }

    BUFFER(self, b);

    if(out != Qnil) {
        CHECK_STRING_TYPE(out);
	    rb_str_resize(out, 0);
    }

    /* do nothing */
    if(n == 0) {
        if(out == Qnil) {
            out = rb_str_buf_new(0);
        }
        return out;
    }

    size_t sz = msgpack_buffer_all_readable_size(b);

    /* read at least one upto n */
    if(sz == 0) {
        rb_raise(rb_eEOFError, "end of buffer reached");
    }

    if(n < 0 || sz < (size_t)n) {
        n = sz;
    }

    if(out == Qnil) {
        out = rb_str_buf_new(n);
    }
    msgpack_buffer_read_to_string(b, out, n);

    return out;
}

/**
 * Document-method: MessagePack::Buffer#to_str
 *
 * call-seq:
 *   MessagePack::Buffer#to_str
 *
 *
 */
static VALUE Buffer_to_str(VALUE self)
{
    BUFFER(self, b);

    return msgpack_buffer_all_as_string(b);
}

/**
 * Document-method: MessagePack::Buffer#to_a
 *
 * call-seq:
 *   MessagePack::Buffer#to_a
 *
 *
 */
static VALUE Buffer_to_a(VALUE self)
{
    BUFFER(self, b);

    return msgpack_buffer_all_as_string_array(b);
}

void Buffer_module_init(VALUE mMessagePack)
{
    msgpack_pool_static_init_default();

    VALUE cBuffer = rb_define_class_under(mMessagePack, "Buffer", rb_cObject);

    rb_define_alloc_func(cBuffer, Buffer_alloc);

    rb_define_method(cBuffer, "initialize", Buffer_initialize, 0);
    rb_define_method(cBuffer, "size", Buffer_size, 0);
    rb_define_method(cBuffer, "empty?", Buffer_empty_p, 0);
    rb_define_method(cBuffer, "append", Buffer_append, 1);
    rb_define_method(cBuffer, "<<", Buffer_append, 1); /* alias */
    rb_define_method(cBuffer, "skip", Buffer_skip, 1);
    rb_define_method(cBuffer, "read", Buffer_read, -1);
    rb_define_method(cBuffer, "readpartial", Buffer_readpartial, -1);
    rb_define_method(cBuffer, "to_str", Buffer_to_str, 0);
    rb_define_method(cBuffer, "to_s", Buffer_to_str, 0); /* alias */
    rb_define_method(cBuffer, "to_a", Buffer_to_a, 0);
}

