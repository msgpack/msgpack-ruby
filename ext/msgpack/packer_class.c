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
#include "packer.h"

// TODO
//static ID s_to_msgpack;
//static ID s_append;
//static ID s_write;


#define PACKER(from, name) \
    msgpack_packer_t *name = NULL; \
    Data_Get_Struct(from, msgpack_packer_t, name); \
    if(name == NULL) { \
        rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
    }

static void Packer_free(void* data)
{
    if(data == NULL) {
        return;
    }
    msgpack_packer_t* pk = (msgpack_packer_t*) data;
    msgpack_packer_destroy(pk);
    free(pk);
}

static VALUE Packer_alloc(VALUE klass)
{
    msgpack_packer_t* pk = ALLOC_N(msgpack_packer_t, 1);
    msgpack_packer_init(pk);

    return Data_Wrap_Struct(klass, msgpack_packer_mark, Packer_free, pk);
}

static VALUE Packer_initialize(int argc, VALUE* argv, VALUE self)
{
    // TODO io
    // TODO reference threshold
    return self;
}

static VALUE Packer_write(VALUE self, VALUE v)
{
    PACKER(self, pk);
    msgpack_packer_write_value(pk, v);
    return Qnil;
}

static VALUE Packer_write_nil(VALUE self)
{
    PACKER(self, pk);
    msgpack_packer_write_nil(pk);
    return Qnil;
}

static VALUE Packer_write_array_header(VALUE self, VALUE n)
{
    PACKER(self, pk);
    msgpack_packer_write_array_header(pk, NUM2UINT(n));
    return Qnil;
}

static VALUE Packer_write_map_header(VALUE self, VALUE n)
{
    PACKER(self, pk);
    msgpack_packer_write_map_header(pk, NUM2UINT(n));
    return Qnil;
}

static VALUE Packer_to_str(VALUE self)
{
    PACKER(self, pk);
    return msgpack_buffer_all_as_string(PACKER_BUFFER_(pk));
}

static VALUE Packer_to_a(VALUE self)
{
    PACKER(self, pk);
    return msgpack_buffer_all_as_string_array(PACKER_BUFFER_(pk));
}

static VALUE Packer_append(VALUE self, VALUE string_or_buffer)
{
    PACKER(self, pk);

    // TODO if string_or_buffer is a Buffer
    VALUE string = string_or_buffer;

    msgpack_buffer_append_string(PACKER_BUFFER_(pk), string);

    return self;
}

void Packer_module_init(VALUE mMessagePack)
{
    VALUE cPacker = rb_define_class_under(mMessagePack, "Packer", rb_cObject);

    rb_define_alloc_func(cPacker, Packer_alloc);

    rb_define_method(cPacker, "initialize", Packer_initialize, -1);
    rb_define_method(cPacker, "write", Packer_write, 1);
    rb_define_method(cPacker, "write_nil", Packer_write_nil, 0);
    rb_define_method(cPacker, "write_array_header", Packer_write_array_header, 1);
    rb_define_method(cPacker, "write_map_header", Packer_write_map_header, 1);
    rb_define_method(cPacker, "to_str", Packer_to_str, 0);
    rb_define_method(cPacker, "to_s", Packer_to_str, 0);
    rb_define_method(cPacker, "to_a", Packer_to_a, 0);
    rb_define_method(cPacker, "append", Packer_append, 1);
}

