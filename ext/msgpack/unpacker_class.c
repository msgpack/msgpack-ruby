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

static ID s_read;
static ID s_readpartial;

static VALUE cUnpacker;

static VALUE eUnpackError;
static VALUE eMalformedFormatError;
static VALUE eStackError;  // TODO < eMalformedFormatError?
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

static VALUE Unpacker_initialize(int argc, VALUE* argv, VALUE self)
{
    if(argc == 0 || (argc == 1 && argv[0] == Qnil)) {
        return self;
    }

    VALUE io = Qnil;
    VALUE options = Qnil;

    if(argc == 1) {
        VALUE v = argv[0];
        if(rb_type(v) == RUBY_T_HASH) {
            options = v;
        } else {
            io = v;
        }

    } else if(argc == 2) {
        io = argv[0];
        options = argv[1];
        if(rb_type(options) != RUBY_T_HASH) {
            rb_raise(rb_eArgError, "expected Hash but found %s.", "TODO"); // TODO klass.to_s
        }

    } else {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..1)", argc);
    }

    UNPACKER(self, uk);

    if(io != Qnil) {
        ID read_method;

        if(rb_respond_to(io, s_readpartial)) {
            read_method = s_readpartial;
        } else if(rb_respond_to(io, s_read)) {
            read_method = s_read;
        } else {
            rb_raise(rb_eArgError, "expected String or IO-like but found %s.", "TODO"); // TODO klass.to_s
        }

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

static VALUE Unpacker_buffer(VALUE self)
{
    UNPACKER(self, uk);
    return uk->buffer_ref;
}

static VALUE Unpacker_read(VALUE self)
{
    UNPACKER(self, uk);

    int r = msgpack_unpacker_read(uk, 0);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return msgpack_unpacker_get_last_object(uk);
}

static VALUE Unpacker_skip(VALUE self)
{
    UNPACKER(self, uk);

    int r = msgpack_unpacker_skip(uk, 0);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return Qnil;
}

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

static VALUE Unpacker_read_array_header(VALUE self)
{
    UNPACKER(self, uk);

    long r = msgpack_unpacker_read_array_header(uk);
    if(r < 0) {
        raise_unpacker_error((int)r);
    }

    return LONG2NUM(r);
}

static VALUE Unpacker_read_map_header(VALUE self)
{
    UNPACKER(self, uk);

    long r = msgpack_unpacker_read_map_header(uk);
    if(r < 0) {
        raise_unpacker_error((int)r);
    }

    return LONG2NUM(r);
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

static VALUE Unpacker_feed(VALUE self, VALUE data)
{
    UNPACKER(self, uk);

    StringValue(data);

    msgpack_buffer_append_string(UNPACKER_BUFFER_(uk), data);

    return Qnil;
}

static VALUE Unpacker_each(VALUE self)
{
    UNPACKER(self, uk);

#ifdef RETURN_ENUMERATOR
    RETURN_ENUMERATOR(self, 0, 0);
#endif

    while(true) {
        int r = msgpack_unpacker_read(uk, 0);
        if(r < 0) {
            if(r == PRIMITIVE_EOF) {
                return Qnil;
            }
printf("error: %d\n", r);
            raise_unpacker_error(r);
        }
        rb_yield(msgpack_unpacker_get_last_object(uk));
    }
}

static VALUE Unpacker_feed_each(VALUE self, VALUE data)
{
    // TODO optimize
    Unpacker_feed(self, data);
    return Unpacker_each(self);
}

//VALUE MessagePack_Unpacker_create(int argc, VALUE* argv)
//{
//    if(argc == 0 || (argc == 1 && argv[0] == Qnil)) {
//        return Unpacker_alloc(cUnpacker);
//    }
//
//    if(argc != 1) {
//        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..1)", argc);
//    }
//    VALUE io = argv[0];
//
//    VALUE klass = rb_class_of(io);
//    if(klass == cUnpacker) {
//        return io;
//    }
//
//    if(klass == rb_cString) {
//        VALUE self = Unpacker_alloc(cUnpacker);
//        UNPACKER(self, uk);
//        msgpack_buffer_append_string(UNPACKER_BUFFER_(uk), io);
//        return self;
//    }
//
//    ID read_method;
//
//    if(rb_respond_to(io, s_readpartial)) {
//        read_method = s_readpartial;
//    } else if(rb_respond_to(io, s_read)) {
//        read_method = s_read;
//    } else {
//        rb_raise(rb_eArgError, "expected String or IO-like but found %s.", "TODO"); // TODO klass.to_s
//    }
//
//    VALUE self = Unpacker_alloc(cUnpacker);
//    UNPACKER(self, uk);
//    msgpack_unpacker_set_io(uk, io, read_method);
//    return self;
//}
//
//static VALUE MessagePack_Unpacker(VALUE mod, VALUE arg)
//{
//    return MessagePack_Unpacker_create(1, &arg);
//}

VALUE MessagePack_Unpacker_module_init(VALUE mMessagePack)
{
    s_read = rb_intern("read");
    s_readpartial = rb_intern("readpartial");

    cUnpacker = rb_define_class_under(mMessagePack, "Unpacker", rb_cObject);

    eUnpackError = rb_define_class_under(mMessagePack, "UnpackError", rb_eStandardError);
    eMalformedFormatError = rb_define_class_under(mMessagePack, "MalformedFormatError", eUnpackError);
    eStackError = rb_define_class_under(mMessagePack, "StackError", eUnpackError);
    eTypeError = rb_define_class_under(mMessagePack, "TypeError", rb_eStandardError);

    rb_define_alloc_func(cUnpacker, Unpacker_alloc);

    rb_define_method(cUnpacker, "initialize", Unpacker_initialize, -1);
    rb_define_method(cUnpacker, "buffer", Unpacker_buffer, 0);
    rb_define_method(cUnpacker, "read", Unpacker_read, 0);
    rb_define_method(cUnpacker, "skip", Unpacker_skip, 0);
    rb_define_method(cUnpacker, "skip_nil", Unpacker_skip_nil, 0);
    rb_define_method(cUnpacker, "read_array_header", Unpacker_read_array_header, 0);
    rb_define_method(cUnpacker, "read_map_header", Unpacker_read_map_header, 0);
    //rb_define_method(cUnpacker, "peek_next_type", Unpacker_peek_next_type, 0);
    rb_define_method(cUnpacker, "feed", Unpacker_feed, 1);
    rb_define_method(cUnpacker, "each", Unpacker_each, 0);
    rb_define_method(cUnpacker, "feed_each", Unpacker_feed_each, 1);

    /* MessagePack::Unpacker(x) */
    //rb_define_module_function(mMessagePack, "Unpacker", MessagePack_Unpacker, 1);

    return cUnpacker;
}

