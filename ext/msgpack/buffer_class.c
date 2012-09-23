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
#include "buffer_class.h"

/*
 * Document-class: MessagePack::Buffer
 *
 * MessagePack::Buffer is a byte-array buffer which is intended to handle
 * large binary objects efficiently. It suppresses copying data when appending
 * or reading them if it's possible. (this technique is know as Copy-on-Write)
 *
 */
#if 0
VALUE mMessagePack = rb_define_module("MessagePack");  /* dummy for rdoc */
#endif

VALUE cMessagePack_Buffer;

static ID s_write;

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

static VALUE Buffer_initialize(VALUE self)
{
    return self;
}


VALUE MessagePack_Buffer_wrap(msgpack_buffer_t* b, VALUE owner)
{
    b->owner = owner;
    return Data_Wrap_Struct(cMessagePack_Buffer, msgpack_buffer_mark, NULL, b);
}


static VALUE Buffer_clear(VALUE self)
{
    BUFFER(self, b);
    msgpack_buffer_clear(b);
    return Qnil;
}

static VALUE Buffer_size(VALUE self)
{
    BUFFER(self, b);
    size_t size = msgpack_buffer_all_readable_size(b);
    return SIZET2NUM(size);
}

static VALUE Buffer_empty_p(VALUE self)
{
    BUFFER(self, b);
    if(msgpack_buffer_top_readable_size(b) == 0) {
        return Qtrue;
    } else {
        return Qfalse;
    }
}

static VALUE Buffer_write(VALUE self, VALUE string_or_buffer)
{
    BUFFER(self, b);

    VALUE string = string_or_buffer;  // TODO optimize if string_or_buffer is a Buffer
    StringValue(string);

    size_t length = msgpack_buffer_append_string(b, string);

    return SIZET2NUM(length);
}

static VALUE Buffer_append(VALUE self, VALUE string_or_buffer)
{
    BUFFER(self, b);

    VALUE string = string_or_buffer;  // TODO optimize if string_or_buffer is a Buffer
    StringValue(string);

    msgpack_buffer_append_string(b, string);

    return self;
}

static VALUE Buffer_skip(VALUE self, VALUE n)
{
    BUFFER(self, b);

    unsigned long sn = FIX2ULONG(n);

    /* do nothing */
    if(sn == 0) {
        return LONG2FIX(0);
    }

    size_t sz = msgpack_buffer_all_readable_size(b);

    if(sz < sn) {
        sn = sz;
    }

    msgpack_buffer_skip_all(b, sn);

    return LONG2FIX(sn);
}

static VALUE Buffer_skip_all(VALUE self, VALUE n)
{
    BUFFER(self, b);

    unsigned long sn = FIX2ULONG(n);

    /* do nothing */
    if(sn == 0) {
        return self;
    }

    /* skip all or raise */
    if(!msgpack_buffer_skip_all(b, sn)) {
        rb_raise(rb_eEOFError, "end of buffer reached");
    }

    return self;
}

static VALUE Buffer_read_all(int argc, VALUE* argv, VALUE self)
{
    VALUE out = Qnil;
    bool length_spec = false;
    unsigned long n = 0;

    switch(argc) {
    case 2:
        out = argv[1];
        /* pass through */
    case 1:
        if(argv[0] != Qnil) {
            n = FIX2ULONG(argv[0]);
            length_spec = true;
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
    }

    /* do nothing */
    if(length_spec && n == 0) {
        if(out == Qnil) {
            out = rb_str_buf_new(0);
        } else {
            rb_str_resize(out, 0);
        }
        return out;
    }

    size_t sz = msgpack_buffer_all_readable_size(b);

    if(length_spec) {
        if(sz < n) {
            /* read all or raise */
            rb_raise(rb_eEOFError, "end of buffer reached");
        }

    } else {
        /* read all available */
        if(sz == 0) {
            if(out == Qnil) {
                out = rb_str_buf_new(0);
            } else {
                rb_str_resize(out, 0);
            }
            return out;
        }
        n = sz;
    }

#ifndef DISABLE_BUFFER_READ_TO_S_OPTIMIZE
    /* same as to_s; optimize */
    if(sz == n && out == Qnil) {
        VALUE str = msgpack_buffer_all_as_string(b);
        msgpack_buffer_clear(b);
        return str;
    }
#endif

    if(out == Qnil) {
        out = rb_str_buf_new(n);
    } else {
        rb_str_resize(out, 0);
    }
    msgpack_buffer_read_to_string(b, out, n);

    return out;
}

static VALUE Buffer_read(int argc, VALUE* argv, VALUE self)
{
    VALUE out = Qnil;
    bool length_spec = false;
    unsigned long n = -1;

    switch(argc) {
    case 2:
        out = argv[1];
        /* pass through */
    case 1:
        if(argv[0] != Qnil) {
            n = FIX2LONG(argv[0]);
            length_spec = true;
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
    }

    /* do nothing */
    if(length_spec && n == 0) {
        if(out == Qnil) {
            out = rb_str_buf_new(0);
        } else {
            rb_str_resize(out, 0);
        }
        return out;
    }

    size_t sz = msgpack_buffer_all_readable_size(b);

    if(sz == 0) {
        if(length_spec) {
            /* EOF => nil */
            return Qnil;
        } else {
            /* read zero or more bytes */
            if(out == Qnil) {
                out = rb_str_buf_new(0);
            } else {
                rb_str_resize(out, 0);
            }
            return out;
        }
    }

    if(!length_spec || sz < n) {
        n = sz;
    }

#ifndef DISABLE_BUFFER_READ_TO_S_OPTIMIZE
    /* same as to_s; optimize */
    if(sz == n && out == Qnil) {
        VALUE str = msgpack_buffer_all_as_string(b);
        msgpack_buffer_clear(b);
        return str;
    }
#endif

    if(out == Qnil) {
        out = rb_str_buf_new(n);
    } else {
        rb_str_resize(out, 0);
    }
    msgpack_buffer_read_to_string(b, out, n);

    return out;
}

static VALUE Buffer_to_str(VALUE self)
{
    BUFFER(self, b);
    return msgpack_buffer_all_as_string(b);
}

static VALUE Buffer_to_a(VALUE self)
{
    BUFFER(self, b);
    return msgpack_buffer_all_as_string_array(b);
}

static VALUE Buffer_write_to(VALUE self, VALUE io)
{
    BUFFER(self, b);
    VALUE ary = msgpack_buffer_all_as_string_array(b);

    unsigned int len = (unsigned int)RARRAY_LEN(ary);
    unsigned int i;
    for(i=0; i < len; ++i) {
        VALUE e = rb_ary_entry(ary, i);
        rb_funcall(io, s_write, 1, e);
    }

    return Qnil;
}

void MessagePack_Buffer_module_init(VALUE mMessagePack)
{
    s_write = rb_intern("write");

    msgpack_buffer_static_init();

    cMessagePack_Buffer = rb_define_class_under(mMessagePack, "Buffer", rb_cObject);

    rb_define_alloc_func(cMessagePack_Buffer, Buffer_alloc);

    rb_define_method(cMessagePack_Buffer, "initialize", Buffer_initialize, 0);
    rb_define_method(cMessagePack_Buffer, "clear", Buffer_clear, 0);
    rb_define_method(cMessagePack_Buffer, "size", Buffer_size, 0);
    rb_define_method(cMessagePack_Buffer, "empty?", Buffer_empty_p, 0);
    rb_define_method(cMessagePack_Buffer, "write", Buffer_write, 1);
    rb_define_method(cMessagePack_Buffer, "<<", Buffer_append, 1);
    rb_define_method(cMessagePack_Buffer, "skip", Buffer_skip, 1);
    rb_define_method(cMessagePack_Buffer, "skip_all", Buffer_skip_all, 1);
    rb_define_method(cMessagePack_Buffer, "read", Buffer_read, -1);
    rb_define_method(cMessagePack_Buffer, "read_all", Buffer_read_all, -1);
    rb_define_method(cMessagePack_Buffer, "write_to", Buffer_write_to, 1);
    rb_define_method(cMessagePack_Buffer, "to_str", Buffer_to_str, 0);
    rb_define_alias(cMessagePack_Buffer, "to_s", "to_str");
    rb_define_method(cMessagePack_Buffer, "to_a", Buffer_to_a, 0);
}

