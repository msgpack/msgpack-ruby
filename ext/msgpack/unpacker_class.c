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
#include "unpacker_class.h"
#include "buffer_class.h"

/*
 * Document-class: MessagePack::Unpacker
 *
 * MessagePack::Unpacker is an interface to deserialize objects from MessagePack::Buffer.
 * It has an internal buffer which is an instance of the MessagePack::Buffer.
 *
 */
#if 0
VALUE mMessagePack = rb_define_module("MessagePack");  /* dummy for rdoc */
#endif

VALUE cMessagePack_Unpacker;

static ID s_read;
static ID s_readpartial;

static VALUE s_unpacker_value;
static msgpack_unpacker_t* s_unpacker;

static VALUE eUnpackError;
static VALUE eMalformedFormatError;
static VALUE eStackError;
static VALUE eTypeError;

#define UNPACKER(from, name) \
    msgpack_unpacker_t *name = NULL; \
    Data_Get_Struct(from, msgpack_unpacker_t, name); \
    if(name == NULL) { \
        rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
    }

static void Unpacker_free(msgpack_unpacker_t* uk)
{
    if(uk == NULL) {
        return;
    }
    msgpack_unpacker_destroy(uk);
    free(uk);
}

static VALUE Unpacker_alloc(VALUE klass)
{
    msgpack_unpacker_t* uk = ALLOC_N(msgpack_unpacker_t, 1);
    msgpack_unpacker_init(uk);

    VALUE self = Data_Wrap_Struct(klass, msgpack_unpacker_mark, Unpacker_free, uk);

    uk->buffer_ref = MessagePack_Buffer_wrap(UNPACKER_BUFFER_(uk), self);

    return self;
}

static ID get_read_method(VALUE io)
{
    if(rb_respond_to(io, s_readpartial)) {
        return s_readpartial;
    } else if(rb_respond_to(io, s_read)) {
        return s_read;
    } else {
        return 0;
    }
}

static ID read_method_of(VALUE io)
{
    ID m = get_read_method(io);
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
 * Creates an instance of the MessagePack::Unpacker.
 *
 * If the optional _io_ argument is given, it reads data from the IO to fill the internal buffer.
 * _io_ must respond to _readpartial(length,string)_ or _read(length,string)_ method.
 *
 * Currently, no options are supported.
 *
 */
static VALUE Unpacker_initialize(int argc, VALUE* argv, VALUE self)
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

    UNPACKER(self, uk);

    if(io != Qnil) {
        ID read_method = read_method_of(io);
        msgpack_unpacker_set_io(uk, io, read_method);
    }

    return self;
}

static void raise_unpacker_error(int r)
{
    switch(r) {
    case PRIMITIVE_EOF:
        rb_raise(rb_eEOFError, "end of buffer reached");
    case PRIMITIVE_INVALID_BYTE:
        rb_raise(eMalformedFormatError, "invalid byte");
    case PRIMITIVE_STACK_TOO_DEEP:
        rb_raise(eStackError, "stack level too deep");
    case PRIMITIVE_UNEXPECTED_TYPE:
        rb_raise(eTypeError, "unexpected type");
    default:
        rb_raise(eUnpackError, "logically unknown error %d", r);
    }
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
static VALUE Unpacker_buffer(VALUE self)
{
    UNPACKER(self, uk);
    return uk->buffer_ref;
}

/**
 * Document-method: read
 *
 * call-seq:
 *   read -> object
 *   unpack -> object
 *
 * Deserializes an object and returns it.
 *
 */
static VALUE Unpacker_read(VALUE self)
{
    UNPACKER(self, uk);

    int r = msgpack_unpacker_read(uk, 0);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return msgpack_unpacker_get_last_object(uk);
}

/**
 * Document-method: skip
 *
 * call-seq:
 *   skip
 *
 * Deserializes an object and ignores it. This method is faster than _read_.
 *
 */
static VALUE Unpacker_skip(VALUE self)
{
    UNPACKER(self, uk);

    int r = msgpack_unpacker_skip(uk, 0);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return Qnil;
}

/**
 * Document-method: skip_nil
 *
 * call-seq:
 *   skip_nil -> bool
 *
 * Deserializes a nil value if it exists and returns _true_.
 * Otherwise, if a byte exists but the byte is not nil value or the internal buffer is empty, returns _false_.
 *
 */
static VALUE Unpacker_skip_nil(VALUE self)
{
    UNPACKER(self, uk);

    int r = msgpack_unpacker_skip_nil(uk);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    if(r) {
        return Qtrue;
    }
    return Qfalse;
}

/**
 * Document-method: read_array_header
 *
 * call-seq:
 *   read_array_header -> integer
 *
 * Read a header of an array and returns its size.
 * It converts a serialized array into a stream of elements.
 *
 * If the serialized object is not an array, it raises MessagePack::TypeError which extends MessagePack::UnpackError.
 *
 */
static VALUE Unpacker_read_array_header(VALUE self)
{
    UNPACKER(self, uk);

    uint32_t size;
    int r = msgpack_unpacker_read_array_header(uk, &size);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return LONG2NUM(size);
}

/**
 * Document-method: read_map_header
 *
 * call-seq:
 *   read_map_header -> integer
 *
 * Read a header of an map and returns its size.
 * It converts a serialized map into a stream of key-value pairs.
 *
 * If the serialized object is not a map, it raises MessagePack::TypeError which extends MessagePack::UnpackError.
 *
 */
static VALUE Unpacker_read_map_header(VALUE self)
{
    UNPACKER(self, uk);

    uint32_t size;
    int r = msgpack_unpacker_read_map_header(uk, &size);
    if(r < 0) {
        raise_unpacker_error((int)r);
    }

    return LONG2NUM(size);
}

static VALUE Unpacker_peek_next_type(VALUE self)
{
    UNPACKER(self, uk);

    int r = msgpack_unpacker_peek_next_object_type(uk);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    switch((enum msgpack_unpacker_object_type) r) {
    case TYPE_NIL:
        return rb_intern("nil");
    case TYPE_BOOLEAN:
        return rb_intern("boolean");
    case TYPE_INTEGER:
        return rb_intern("integer");
    case TYPE_FLOAT:
        return rb_intern("float");
    case TYPE_RAW:
        return rb_intern("raw");
    case TYPE_ARRAY:
        return rb_intern("array");
    case TYPE_MAP:
        return rb_intern("map");
    default:
        rb_raise(eUnpackError, "logically unknown type %d", r);
    }
}

/**
 * Document-method: feed
 *
 * call-seq:
 *   feed(string) -> self
 *
 * Appends data into the internal buffer.
 *
 */
static VALUE Unpacker_feed(VALUE self, VALUE data)
{
    UNPACKER(self, uk);

    StringValue(data);

    msgpack_buffer_append_string(UNPACKER_BUFFER_(uk), data);

    return self;
}

static VALUE Unpacker_each_impl(VALUE self)
{
    UNPACKER(self, uk);

    while(true) {
        int r = msgpack_unpacker_read(uk, 0);
        if(r < 0) {
            if(r == PRIMITIVE_EOF) {
                return Qnil;
            }
            raise_unpacker_error(r);
        }
        rb_yield(msgpack_unpacker_get_last_object(uk));
    }
}

static VALUE Unpacker_rescue_EOFError(VALUE self)
{
    return Qnil;
}

/**
 * Document-method: each
 *
 * call-seq:
 *   each {|object| }
 *
 * Repeats to deserialize objects.
 *
 * It repeats until the internal buffer does not include any complete objects.
 *
 * If the internal IO was set, it reads data from the IO when the buffer becomes empty,
 * and returns when the IO raised EOFError.
 *
 */
static VALUE Unpacker_each(VALUE self)
{
    UNPACKER(self, uk);

#ifdef RETURN_ENUMERATOR
    RETURN_ENUMERATOR(self, 0, 0);
#endif

    if(uk->io == Qnil) {
	    return Unpacker_each_impl(self);
    } else {
        /* rescue EOFError if io is set */
	    return rb_rescue2(Unpacker_each_impl, self,
			    Unpacker_rescue_EOFError, self,
                rb_eEOFError, NULL);
    }
}

/**
 * Document-method: feed_each
 *
 * call-seq:
 *   feed_each(string) {|object| }
 *
 * Appends data into the internal buffer and repeats to deserialize objects.
 * Same as _feed(string).each {|object }_.
 *
 */
static VALUE Unpacker_feed_each(VALUE self, VALUE data)
{
    // TODO optimize
    Unpacker_feed(self, data);
    return Unpacker_each(self);
}

VALUE MessagePack_unpack(int argc, VALUE* argv)
{
    VALUE src;
    ID read_method = 0;

    switch(argc) {
    case 1:
        src = argv[0];
        break;
    default:
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 1)", argc);
    }

    if(rb_type(src) != T_STRING) {
        read_method = get_read_method(src);
        if(read_method == 0) {
            src = StringValue(src);
        }
    }

    //VALUE self = Unpacker_alloc(cMessagePack_Unpacker);
    //UNPACKER(self, uk);
    msgpack_unpacker_reset(s_unpacker);

    if(read_method == 0) {
        // TODO change buffer reference threshold instead of calling _msgpack_buffer_append_reference
        msgpack_buffer_append_string(UNPACKER_BUFFER_(s_unpacker), src);
        //_msgpack_buffer_append_reference(UNPACKER_BUFFER_(s_unpacker), RSTRING_PTR(src), RSTRING_LEN(src), src);
    } else {
        msgpack_unpacker_set_io(s_unpacker, src, read_method);
    }

    int r = msgpack_unpacker_read(s_unpacker, 0);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    /* raise if extra bytes follow */
    if(msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(s_unpacker)) > 0) {
        rb_raise(eMalformedFormatError, "extra bytes follow after a deserialized object");
    }

    return msgpack_unpacker_get_last_object(s_unpacker);
}

/**
 * call-seq:
 *   load(string) -> object
 *   load(io) -> object
 *
 * Deserializes an object from the given _src_ and returns the deserialized object.
 *
 * If the given argument is not a string, it assumes the argument is an IO and reads data from the IO.
 * _io_ must respond to _readpartial(length,string)_ or _read(length,string)_ method.
 *
 */
static VALUE MessagePack_load_module_method(int argc, VALUE* argv, VALUE mod)
{
    return MessagePack_unpack(argc, argv);
}

/**
 * call-seq:
 *   unapck(string) -> object
 *   unapck(io) -> object
 *
 * Alias of load
 */
static VALUE MessagePack_unpack_module_method(int argc, VALUE* argv, VALUE mod)
{
    return MessagePack_unpack(argc, argv);
}

void MessagePack_Unpacker_module_init(VALUE mMessagePack)
{
    s_read = rb_intern("read");
    s_readpartial = rb_intern("readpartial");

    cMessagePack_Unpacker = rb_define_class_under(mMessagePack, "Unpacker", rb_cObject);

    /**
     * TODO rdoc comments
     */
    eUnpackError = rb_define_class_under(mMessagePack, "UnpackError", rb_eStandardError);

    /**
     * TODO rdoc comments
     */
    eMalformedFormatError = rb_define_class_under(mMessagePack, "MalformedFormatError", eUnpackError);

    /**
     * TODO rdoc comments
     */
    eStackError = rb_define_class_under(mMessagePack, "StackError", eUnpackError);

    /**
     * TODO rdoc comments
     */
    eTypeError = rb_define_class_under(mMessagePack, "TypeError", rb_eStandardError);

    rb_define_alloc_func(cMessagePack_Unpacker, Unpacker_alloc);

    rb_define_method(cMessagePack_Unpacker, "initialize", Unpacker_initialize, -1);
    rb_define_method(cMessagePack_Unpacker, "buffer", Unpacker_buffer, 0);
    rb_define_method(cMessagePack_Unpacker, "read", Unpacker_read, 0);
    rb_define_alias(cMessagePack_Unpacker, "unpack", "read");
    rb_define_method(cMessagePack_Unpacker, "skip", Unpacker_skip, 0);
    rb_define_method(cMessagePack_Unpacker, "skip_nil", Unpacker_skip_nil, 0);
    rb_define_method(cMessagePack_Unpacker, "read_array_header", Unpacker_read_array_header, 0);
    rb_define_method(cMessagePack_Unpacker, "read_map_header", Unpacker_read_map_header, 0);
    //rb_define_method(cMessagePack_Unpacker, "peek_next_type", Unpacker_peek_next_type, 0);
    rb_define_method(cMessagePack_Unpacker, "feed", Unpacker_feed, 1);
    rb_define_method(cMessagePack_Unpacker, "each", Unpacker_each, 0);
    rb_define_method(cMessagePack_Unpacker, "feed_each", Unpacker_feed_each, 1);

    s_unpacker_value = Unpacker_alloc(cMessagePack_Unpacker);
    rb_gc_register_address(&s_unpacker_value);
    Data_Get_Struct(s_unpacker_value, msgpack_unpacker_t, s_unpacker);

    /* MessagePack.unpack(x) */
    rb_define_module_function(mMessagePack, "load", MessagePack_load_module_method, -1);
    rb_define_module_function(mMessagePack, "unpack", MessagePack_unpack_module_method, -1);
}

