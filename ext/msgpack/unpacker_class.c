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

static ID s_read;
static ID s_readpartial;

static VALUE cUnpacker;

static VALUE eUnpackerError;
static VALUE eMalformedFormatError;
static VALUE eUnapackerStackError;  // < eMalformedFormatError?

#define UNPACKER(from, name) \
    msgpack_unpacker_t *name = NULL; \
    Data_Get_Struct(from, msgpack_unpacker_t, name); \
    if(name == NULL) { \
        rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
    }

static void Unpacker_free(void* data)
{
    if(data == NULL) {
        return;
    }
    msgpack_unpacker_t* uk = (msgpack_unpacker_t*) data;
    msgpack_unpacker_destroy(uk);
    free(uk);
}

static VALUE Unpacker_alloc(VALUE klass)
{
    msgpack_unpacker_t* uk = ALLOC_N(msgpack_unpacker_t, 1);
    msgpack_unpacker_init(uk);

    return Data_Wrap_Struct(klass, msgpack_unpacker_mark, Unpacker_free, uk);
}

static VALUE Unpacker_initialize(int argc, VALUE* argv, VALUE self)
{
    // TODO io
    // TODO stack size
    return self;
}

static void raise_unpacker_error(int r)
{
    switch(r) {
    case PRIMITIVE_EOF:
        rb_raise(rb_eEOFError, "end of buffer reached");
        break;
    case PRIMITIVE_INVALID_BYTE:
        // TODO
        //rb_raise(eMalformedFormatError, "invalid byte");
        rb_raise(rb_eRuntimeError, "invalid byte");
        break;
    case PRIMITIVE_STACK_TOO_DEEP:
        // TODO
        //rb_raise(eMalformedFormatError, "stack level too deep");
        rb_raise(rb_eRuntimeError, "stack level too deep");
        break;
    default:
        // TODO
        //rb_raise(eUnpackerError, "logically unknown error %d", r);
        rb_raise(rb_eRuntimeError, "logically unknown error %d", r);
    }
}

static VALUE Unpacker_read(int argc, VALUE* argv, VALUE self)
{
    UNPACKER(self, uk);

    // TODO args check

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
        raise_unpacker_error(r);
    }

    return LONG2NUM(r);
}

static VALUE Unpacker_read_map_header(VALUE self)
{
    UNPACKER(self, uk);

    long r = msgpack_unpacker_read_map_header(uk);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return LONG2NUM(r);
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

VALUE MessagePack_Unpacker_create(int argc, VALUE* args)
{
    if(argc == 0 || (argc == 1 && args[0] == Qnil)) {
        return Unpacker_alloc(cUnpacker);
    }

    if(argc != 1) {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..1)", argc);
    }
    VALUE v = args[0];

    VALUE klass = rb_class_of(v);
    if(klass == cUnpacker) {
        return v;
    }

    if(klass == rb_cString) {
        VALUE self = Unpacker_alloc(cUnpacker);
        UNPACKER(self, uk);
        msgpack_buffer_append_string(UNPACKER_BUFFER_(uk), v);
        return self;
    }

    ID read_method;

    if(rb_respond_to(v, s_readpartial)) {
        read_method = s_readpartial;
    } else if(rb_respond_to(v, s_read)) {
        read_method = s_read;
    } else {
        rb_raise(rb_eArgError, "expected String or IO-like but found %s.", "TODO"); // TODO klass.to_s
    }

    VALUE self = Unpacker_alloc(cUnpacker);
    UNPACKER(self, uk);
    msgpack_unpacker_set_io(uk, v, read_method);
    return self;
}

static VALUE MessagePack_Unpacker(int argc, VALUE* argv, VALUE self)
{
    return MessagePack_Unpacker_create(argc, argv);
}

VALUE MessagePack_Unpacker_module_init(VALUE mMessagePack)
{
    s_read = rb_intern("read");
    s_readpartial = rb_intern("readpartial");

    cUnpacker = rb_define_class_under(mMessagePack, "Unpacker", rb_cObject);

    rb_define_alloc_func(cUnpacker, Unpacker_alloc);

    rb_define_method(cUnpacker, "initialize", Unpacker_initialize, -1);
    rb_define_method(cUnpacker, "read", Unpacker_read, -1);
    rb_define_method(cUnpacker, "skip", Unpacker_skip, 0);
    rb_define_method(cUnpacker, "skip_nil", Unpacker_skip_nil, 0);
    rb_define_method(cUnpacker, "read_array_header", Unpacker_read_array_header, 0);
    rb_define_method(cUnpacker, "read_map_header", Unpacker_read_map_header, 0);
    rb_define_method(cUnpacker, "feed", Unpacker_feed, 1);
    rb_define_method(cUnpacker, "each", Unpacker_each, 0);
    rb_define_method(cUnpacker, "feed_each", Unpacker_feed_each, 1);

    /* MessagePack::Unpacker(x) */
    rb_define_module_function(mMessagePack, "Unpacker", MessagePack_Unpacker, 1);

    return cUnpacker;
}

