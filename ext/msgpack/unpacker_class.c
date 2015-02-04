/*
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2013 Sadayuki Furuhashi
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

VALUE cMessagePack_Unpacker;

//static VALUE s_unpacker_value;
//static msgpack_unpacker_t* s_unpacker;

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
    _msgpack_unpacker_destroy(uk);
    free(uk);
}

static VALUE Unpacker_alloc(VALUE klass)
{
    msgpack_unpacker_t* uk = ALLOC_N(msgpack_unpacker_t, 1);
    _msgpack_unpacker_init(uk);

    VALUE self = Data_Wrap_Struct(klass, msgpack_unpacker_mark, Unpacker_free, uk);

    uk->buffer_ref = MessagePack_Buffer_wrap(UNPACKER_BUFFER_(uk), self);

    return self;
}

static VALUE Unpacker_initialize(int argc, VALUE* argv, VALUE self)
{
    VALUE io = Qnil;
    VALUE options = Qnil;

    if(argc == 0 || (argc == 1 && argv[0] == Qnil)) {
        /* Qnil */

    } else if(argc == 1) {
        VALUE v = argv[0];
        if(rb_type(v) == T_HASH) {
            options = v;
            if(rb_type(options) != T_HASH) {
                rb_raise(rb_eArgError, "expected Hash but found %s.", rb_obj_classname(options));
            }
        } else {
            io = v;
        }

    } else if(argc == 2) {
        io = argv[0];
        options = argv[1];
        if(rb_type(options) != T_HASH) {
            rb_raise(rb_eArgError, "expected Hash but found %s.", rb_obj_classname(options));
        }

    } else {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..2)", argc);
    }

    UNPACKER(self, uk);

    MessagePack_Unpacker_initialize(uk, io, options);

    return self;
}

void MessagePack_Unpacker_initialize(msgpack_unpacker_t* uk, VALUE io, VALUE options)
{
    MessagePack_Buffer_initialize(UNPACKER_BUFFER_(uk), io, options);

    if(options != Qnil) {
        VALUE v;

        v = rb_hash_aref(options, ID2SYM(rb_intern("symbolize_keys")));
        msgpack_unpacker_set_symbolized_keys(uk, RTEST(v));
    }
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

    uint32_t size;
    int r = msgpack_unpacker_read_array_header(uk, &size);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    return ULONG2NUM(size);
}

static VALUE Unpacker_read_map_header(VALUE self)
{
    UNPACKER(self, uk);

    uint32_t size;
    int r = msgpack_unpacker_read_map_header(uk, &size);
    if(r < 0) {
        raise_unpacker_error((int)r);
    }

    return ULONG2NUM(size);
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
        VALUE v = msgpack_unpacker_get_last_object(uk);
#ifdef JRUBY
        /* TODO JRuby's rb_yield behaves differently from Ruby 1.9.3 or Rubinius. */
        if(rb_type(v) == T_ARRAY) {
            v = rb_ary_new3(1, v);
        }
#endif
        rb_yield(v);
    }
}

static VALUE Unpacker_rescue_EOFError(VALUE self)
{
    UNUSED(self);
    return Qnil;
}

static VALUE Unpacker_each(VALUE self)
{
    UNPACKER(self, uk);

#ifdef RETURN_ENUMERATOR
    RETURN_ENUMERATOR(self, 0, 0);
#endif

    if(msgpack_buffer_has_io(UNPACKER_BUFFER_(uk))) {
        /* rescue EOFError only if io is set */
        return rb_rescue2(Unpacker_each_impl, self,
                Unpacker_rescue_EOFError, self,
                rb_eEOFError, NULL);
    } else {
        return Unpacker_each_impl(self);
    }
}

static VALUE Unpacker_feed_each(VALUE self, VALUE data)
{
    // TODO optimize
    Unpacker_feed(self, data);
    return Unpacker_each(self);
}

static VALUE Unpacker_reset(VALUE self)
{
    UNPACKER(self, uk);

    _msgpack_unpacker_reset(uk);

    return Qnil;
}

VALUE MessagePack_unpack(int argc, VALUE* argv)
{
    VALUE src;
    VALUE options = Qnil;

    switch(argc) {
    case 2:
        options = argv[1];
        if(rb_type(options) != T_HASH) {
            rb_raise(rb_eArgError, "expected Hash but found %s.", rb_obj_classname(options));
        }
        /* pass-through */
    case 1:
        src = argv[0];
        break;
    default:
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 1..2)", argc);
    }

    VALUE self = Unpacker_alloc(cMessagePack_Unpacker);
    UNPACKER(self, uk);
    //_msgpack_unpacker_reset(s_unpacker);
    //msgpack_buffer_reset_io(UNPACKER_BUFFER_(s_unpacker));

    /* prefer reference than copying; see MessagePack_Unpacker_module_init */
    msgpack_buffer_set_write_reference_threshold(UNPACKER_BUFFER_(uk), 0);

    if(rb_type(src) == T_STRING) {
        MessagePack_Unpacker_initialize(uk, Qnil, options);
        msgpack_buffer_append_string(UNPACKER_BUFFER_(uk), src);
    } else {
        MessagePack_Unpacker_initialize(uk, src, options);
    }

    int r = msgpack_unpacker_read(uk, 0);
    if(r < 0) {
        raise_unpacker_error(r);
    }

    /* raise if extra bytes follow */
    if(msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk)) > 0) {
        rb_raise(eMalformedFormatError, "extra bytes follow after a deserialized object");
    }

#ifdef RB_GC_GUARD
    /* This prevents compilers from optimizing out the `self` variable
     * from stack. Otherwise GC free()s it. */
    RB_GC_GUARD(self);
#endif

    return msgpack_unpacker_get_last_object(uk);
}

static VALUE MessagePack_load_module_method(int argc, VALUE* argv, VALUE mod)
{
    UNUSED(mod);
    return MessagePack_unpack(argc, argv);
}

static VALUE MessagePack_unpack_module_method(int argc, VALUE* argv, VALUE mod)
{
    UNUSED(mod);
    return MessagePack_unpack(argc, argv);
}

void MessagePack_Unpacker_module_init(VALUE mMessagePack)
{
    msgpack_unpacker_static_init();

    cMessagePack_Unpacker = rb_define_class_under(mMessagePack, "Unpacker", rb_cObject);

    eUnpackError = rb_define_class_under(mMessagePack, "UnpackError", rb_eStandardError);

    eMalformedFormatError = rb_define_class_under(mMessagePack, "MalformedFormatError", eUnpackError);

    eStackError = rb_define_class_under(mMessagePack, "StackError", eUnpackError);

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
    //rb_define_method(cMessagePack_Unpacker, "peek_next_type", Unpacker_peek_next_type, 0);  // TODO
    rb_define_method(cMessagePack_Unpacker, "feed", Unpacker_feed, 1);
    rb_define_method(cMessagePack_Unpacker, "each", Unpacker_each, 0);
    rb_define_method(cMessagePack_Unpacker, "feed_each", Unpacker_feed_each, 1);
    rb_define_method(cMessagePack_Unpacker, "reset", Unpacker_reset, 0);

    //s_unpacker_value = Unpacker_alloc(cMessagePack_Unpacker);
    //rb_gc_register_address(&s_unpacker_value);
    //Data_Get_Struct(s_unpacker_value, msgpack_unpacker_t, s_unpacker);
    /* prefer reference than copying */
    //msgpack_buffer_set_write_reference_threshold(UNPACKER_BUFFER_(s_unpacker), 0);

    /* MessagePack.unpack(x) */
    rb_define_module_function(mMessagePack, "load", MessagePack_load_module_method, -1);
    rb_define_module_function(mMessagePack, "unpack", MessagePack_unpack_module_method, -1);
}

