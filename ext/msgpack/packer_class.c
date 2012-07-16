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
#include "packer_class.h"
#include "buffer_class.h"

/*
 * Document-class: MessagePack::Packer
 *
 * MessagePack::Packer is an interface to serialize objects into MessagePack::Buffer.
 * It has an internal buffer which is an instance of the MessagePack::Buffer.
 *
 */
#if 0
VALUE mMessagePack = rb_define_module("MessagePack");  /* dummy for rdoc */
#endif

VALUE cMessagePack_Packer;

static ID s_to_msgpack;
static ID s_append;
static ID s_write;

#define PACKER(from, name) \
    msgpack_packer_t* name; \
    Data_Get_Struct(from, msgpack_packer_t, name); \
    if(name == NULL) { \
        rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
    }

static void Packer_free(msgpack_packer_t* pk)
{
    if(pk == NULL) {
        return;
    }
    msgpack_packer_destroy(pk);
    free(pk);
}

static VALUE Packer_alloc(VALUE klass)
{
    msgpack_packer_t* pk = ALLOC_N(msgpack_packer_t, 1);
    msgpack_packer_init(pk);

    VALUE self = Data_Wrap_Struct(klass, msgpack_packer_mark, Packer_free, pk);

    msgpack_packer_set_to_msgpack_method(pk, s_to_msgpack, self);
    pk->buffer_ref = MessagePack_Buffer_wrap(PACKER_BUFFER_(pk), self);

    return self;
}

static ID get_write_method(VALUE io)
{
    if(rb_respond_to(io, s_write)) {
        return s_write;
    } else if(rb_respond_to(io, s_append)) {
        return s_append;
    } else {
        return 0;
    }
}

static ID write_method_of(VALUE io)
{
    ID m = get_write_method(io);
    if(m == 0) {
        rb_raise(rb_eArgError, "expected String or IO-like but found %s.", rb_obj_classname(io));
    }
    return m;
}

/**
 * Document-method: initialize
 *
 * call-seq:
 *   initialize(options={})
 *   initialize(io, options={})
 *
 * Creates an instance of the MessagePack::Packer.
 *
 * If the optional _io_ argument is given, it writes serialzied objects into the IO when the internal buffer is filled.
 * _io_ must respond to _write(string)_ or _append(string)_ method.
 *
 * Currently, no options are supported.
 *
 */
static VALUE Packer_initialize(int argc, VALUE* argv, VALUE self)
{
    if(argc == 0 || (argc == 1 && argv[0] == Qnil)) {
        return self;
    }

    VALUE io = Qnil;
    VALUE options = Qnil;

    if(argc == 1) {
        VALUE v = argv[0];
        if(rb_type(v) == T_HASH) {
            options = v;
        } else {
            io = v;
        }

    } else if(argc == 2) {
        io = argv[0];
        options = argv[1];
        if(rb_type(options) != T_HASH) {
            rb_raise(rb_eArgError, "expected Hash but found %s.", rb_obj_classname(io));
        }

    } else {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..1)", argc);
    }

    PACKER(self, pk);

    // TODO options
    // TODO reference threshold

    if(io != Qnil) {
        ID write_method = write_method_of(io);
        msgpack_packer_set_io(pk, io, write_method);
    }

    return self;
}

/**
 * Document-method: buffer
 *
 * call-seq:
 *   buffer -> #<MessagePack::Buffer>
 *
 * Returns internal buffer.
 *
 */
static VALUE Packer_buffer(VALUE self)
{
    PACKER(self, pk);
    return pk->buffer_ref;
}

/**
 * Document-method: write
 *
 * call-seq:
 *   write(obj) -> self
 *
 * Serializes the given object to the internal buffer.
 *
 * If it could not serialize the object, it raises
 * NoMethodError: undefined method `to_msgpack' for #<the_object>.
 *
 */
static VALUE Packer_write(VALUE self, VALUE v)
{
    PACKER(self, pk);
    msgpack_packer_write_value(pk, v);
    return self;
}

/**
 * Document-method: write_nil
 *
 * call-seq:
 *   write_nil -> self
 *
 * Same as _write(nil)_.
 *
 */
static VALUE Packer_write_nil(VALUE self)
{
    PACKER(self, pk);
    msgpack_packer_write_nil(pk);
    return self;
}

/**
 * Document-method: write_array_header
 *
 * call-seq:
 *   write_array_header(n) -> self
 *
 * Write a header of an array whose size is _n_.
 *
 * For example, _write_array_header(1).write(true)_ is same as _write([ true ])_.
 *
 */
static VALUE Packer_write_array_header(VALUE self, VALUE n)
{
    PACKER(self, pk);
    msgpack_packer_write_array_header(pk, NUM2UINT(n));
    return self;
}

/**
 * Document-method: write_map_header
 *
 * call-seq:
 *   write_map_header(n) -> self
 *
 * Write a header of a map whose size is _n_.
 *
 * For example, _write_map_header(1).write('key').write(true)_ is same as _write('key'=>true)_.
 *
 */
static VALUE Packer_write_map_header(VALUE self, VALUE n)
{
    PACKER(self, pk);
    msgpack_packer_write_map_header(pk, NUM2UINT(n));
    return self;
}

/**
 * Document-method: flush
 *
 * call-seq:
 *   flush -> self
 *
 * Flushes data in the internal buffer to the internal IO.
 *
 * If internal IO is not set, it doesn nothing.
 *
 */
static VALUE Packer_flush(VALUE self)
{
    PACKER(self, pk);

    if(pk->io != Qnil) {
        msgpack_buffer_flush_to_io(PACKER_BUFFER_(pk), pk->io, pk->io_write_all_method);
    }

    return self;
}

/**
 * Document-method: clear
 *
 * call-seq:
 *   clear
 *
 * Makes the internal buffer empty. Same as _buffer.clear_.
 *
 */
static VALUE Packer_clear(VALUE self)
{
    PACKER(self, pk);
    msgpack_buffer_clear(PACKER_BUFFER_(pk));
    return Qnil;
}

/**
 * Document-method: size
 *
 * call-seq:
 *   size -> integer
 *
 * Returns size of the internal buffer. Same as _buffer.size_.
 *
 */
static VALUE Packer_size(VALUE self)
{
    PACKER(self, pk);
    size_t size = msgpack_buffer_all_readable_size(PACKER_BUFFER_(pk));
    return SIZET2NUM(size);
}

/**
 * Document-method: empty?
 *
 * call-seq:
 *   empty? -> bool
 *
 * Returns _true_ if the internal buffer is empty. Same as _buffer.empty?_.
 *
 */
static VALUE Packer_empty_p(VALUE self)
{
    PACKER(self, pk);
    if(msgpack_buffer_top_readable_size(PACKER_BUFFER_(pk)) == 0) {
        return Qtrue;
    } else {
        return Qfalse;
    }
}

/**
 * Document-method: to_str
 *
 * call-seq:
 *   to_str -> string
 *
 * Returns all data in the buffer as a string. Same as _buffer.to_str_.
 *
 */
static VALUE Packer_to_str(VALUE self)
{
    PACKER(self, pk);
    return msgpack_buffer_all_as_string(PACKER_BUFFER_(pk));
}

/**
 * Document-method: to_a
 *
 * call-seq:
 *   to_a -> array_of_strings
 *
 * Returns content of the internal buffer as an array of strings. Same as _buffer.to_a_.
 *
 */
static VALUE Packer_to_a(VALUE self)
{
    PACKER(self, pk);
    return msgpack_buffer_all_as_string_array(PACKER_BUFFER_(pk));
}

/**
 * Document-method: write_to
 *
 * call-seq:
 *   write_to(io)
 *
 * Writes all of data in the internal buffer into the given IO. Same as _buffer.write_to(io)_.
 *
 */
static VALUE Packer_write_to(VALUE self, VALUE io)
{
    PACKER(self, pk);
    VALUE ary = msgpack_buffer_all_as_string_array(PACKER_BUFFER_(pk));

    unsigned int len = (unsigned int)RARRAY_LEN(ary);
    unsigned int i;
    for(i=0; i < len; ++i) {
        VALUE e = rb_ary_entry(ary, i);
        rb_funcall(io, s_write, 1, e);
    }

    return Qnil;
}

//static VALUE Packer_append(VALUE self, VALUE string_or_buffer)
//{
//    PACKER(self, pk);
//
//    // TODO if string_or_buffer is a Buffer
//    VALUE string = string_or_buffer;
//
//    msgpack_buffer_append_string(PACKER_BUFFER_(pk), string);
//
//    return self;
//}

//VALUE MessagePack_Packer_create(int argc, VALUE* args)
//{
//    if(argc == 0 || (argc == 1 && args[0] == Qnil)) {
//        return Packer_alloc(cMessagePack_Packer);
//    }
//
//    if(argc != 1) {
//        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..1)", argc);
//    }
//    VALUE io = args[0];
//
//    VALUE klass = rb_class_of(io);
//    if(klass == cMessagePack_Packer) {
//        return io;
//    }
//
//    if(klass == rb_cString) {
//        VALUE self = Packer_alloc(cMessagePack_Packer);
//        PACKER(self, pk);
//        msgpack_buffer_append_string(PACKER_BUFFER_(pk), io);
//        return self;
//    }
//
//    ID write_method = write_method_of(io);
//
//    VALUE self = Packer_alloc(cMessagePack_Packer);
//    PACKER(self, pk);
//    msgpack_packer_set_io(pk, io, write_method);
//    return self;
//}

//static VALUE MessagePack_Packer(VALUE mod, VALUE arg)
//{
//    return MessagePack_Packer_create(1, &arg);
//}

VALUE MessagePack_pack(int argc, VALUE* argv)
{
    VALUE v;
    VALUE io = Qnil;
    ID write_method;

    switch(argc) {
    case 2:
        io = argv[1];
        if(io != Qnil) {
            write_method = write_method_of(io);
        }
        /* pass-through */
    case 1:
        v = argv[0];
        break;
    default:
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 1..2)", argc);
    }

    VALUE self = Packer_alloc(cMessagePack_Packer);
    PACKER(self, pk);

    if(io != Qnil) {
        msgpack_packer_set_io(pk, io, write_method);
        msgpack_packer_write_value(pk, v);
        Packer_flush(self);
        return Qnil;

    } else {
        msgpack_packer_write_value(pk, v);
        return msgpack_buffer_all_as_string(PACKER_BUFFER_(pk));
    }
}

/**
 * call-seq:
 *   pack(obj) -> string
 *   dump(obj) -> string
 *   pack(obj, io) -> nil
 *   dump(obj, io) -> nil
 *
 * Serializes the given _object_ and returns the serialized data as a string.
 *
 * If the optional _io_ argument is given, it writes serialzied objects into the IO and returns nil.
 * _io_ must respond to _readpartial(length,string)_ or _read(length,string)_ method.
 *
 */
static VALUE MessagePack_pack_module_method(int argc, VALUE* argv, VALUE mod)
{
    return MessagePack_pack(argc, argv);
}

void MessagePack_Packer_module_init(VALUE mMessagePack)
{
    s_to_msgpack = rb_intern("to_msgpack");
    s_append = rb_intern("<<");
    s_write = rb_intern("write");

    cMessagePack_Packer = rb_define_class_under(mMessagePack, "Packer", rb_cObject);

    rb_define_alloc_func(cMessagePack_Packer, Packer_alloc);

    rb_define_method(cMessagePack_Packer, "initialize", Packer_initialize, -1);
    rb_define_method(cMessagePack_Packer, "buffer", Packer_buffer, 0);
    rb_define_method(cMessagePack_Packer, "write", Packer_write, 1);
    rb_define_alias(cMessagePack_Packer, "pack", "write");
    rb_define_method(cMessagePack_Packer, "write_nil", Packer_write_nil, 0);
    rb_define_method(cMessagePack_Packer, "write_array_header", Packer_write_array_header, 1);
    rb_define_method(cMessagePack_Packer, "write_map_header", Packer_write_map_header, 1);
    rb_define_method(cMessagePack_Packer, "flush", Packer_flush, 0);

    /* delegation methods */
    rb_define_method(cMessagePack_Packer, "clear", Packer_clear, 0);
    rb_define_method(cMessagePack_Packer, "size", Packer_size, 0);
    rb_define_method(cMessagePack_Packer, "empty?", Packer_empty_p, 0);
    rb_define_method(cMessagePack_Packer, "write_to", Packer_write_to, 1);
    rb_define_method(cMessagePack_Packer, "to_str", Packer_to_str, 0);
    rb_define_alias(cMessagePack_Packer, "to_s", "to_str");
    rb_define_method(cMessagePack_Packer, "to_a", Packer_to_a, 0);
    //rb_define_method(cMessagePack_Packer, "append", Packer_append, 1);
    //rb_define_alias(cMessagePack_Packer, "<<", "append");

    /* MessagePack::Packer(x) */
    //rb_define_module_function(mMessagePack, "Packer", MessagePack_Packer, 1);

    /* MessagePack.pack(x) */
    rb_define_module_function(mMessagePack, "pack", MessagePack_pack_module_method, -1);
    rb_define_module_function(mMessagePack, "dump", MessagePack_pack_module_method, -1);
}

