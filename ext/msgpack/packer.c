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

#include "packer.h"

#ifdef RUBINIUS
static ID s_to_iter;
static ID s_next;
static ID s_key;
static ID s_value;
#endif

static ID s_call;

static void msgpack_packer_increment_depth(msgpack_packer_t* pk)
{
    if(!pk->max_depth) {
        return;
    }

    if(pk->depth >= pk->max_depth) {
        rb_raise(rb_eArgError, "stack level too deep");
    }

    pk->depth += 1;
}

static VALUE msgpack_packer_decrement_depth(VALUE p) {
    msgpack_packer_t* pk = (void*) p;
    pk->depth -= 1;
    return Qnil;
}

void msgpack_packer_static_init()
{
#ifdef RUBINIUS
    s_to_iter = rb_intern("to_iter");
    s_next = rb_intern("next");
    s_key = rb_intern("key");
    s_value = rb_intern("value");
#endif

    s_call = rb_intern("call");
}

void msgpack_packer_static_destroy()
{ }

void msgpack_packer_init(msgpack_packer_t* pk)
{
    memset(pk, 0, sizeof(msgpack_packer_t));

    msgpack_buffer_init(PACKER_BUFFER_(pk));
}

void msgpack_packer_destroy(msgpack_packer_t* pk)
{
    msgpack_buffer_destroy(PACKER_BUFFER_(pk));
}

void msgpack_packer_mark(msgpack_packer_t* pk)
{
    /* See MessagePack_Buffer_wrap */
    /* msgpack_buffer_mark(PACKER_BUFFER_(pk)); */
    rb_gc_mark(pk->buffer_ref);
}

void msgpack_packer_reset(msgpack_packer_t* pk)
{
    msgpack_buffer_clear(PACKER_BUFFER_(pk));

    pk->buffer_ref = Qnil;
}

struct msgpack_packer_write_collection_args {
    msgpack_packer_t* pk;
    VALUE v;
};

static VALUE msgpack_packer_write_array_value_cleared(VALUE p) {
    struct msgpack_packer_write_collection_args* args = (void*) p;
    msgpack_packer_t* pk = args->pk;
    VALUE v = args->v;

    /* actual return type of RARRAY_LEN is long */
    unsigned long len = RARRAY_LEN(v);
    if(len > 0xffffffffUL) {
        rb_raise(rb_eArgError, "size of array is too long to pack: %lu bytes should be <= %lu", len, 0xffffffffUL);
    }
    unsigned int len32 = (unsigned int)len;
    msgpack_packer_write_array_header(pk, len32);

    unsigned int i;
    for(i=0; i < len32; ++i) {
        VALUE e = rb_ary_entry(v, i);
        msgpack_packer_write_value(pk, e);
    }

    return Qnil;
}

void msgpack_packer_write_array_value(msgpack_packer_t* pk, VALUE v)
{
    msgpack_packer_increment_depth(pk);

    struct msgpack_packer_write_collection_args args = (struct msgpack_packer_write_collection_args){
        .pk = pk,
        .v = v,
    };
    rb_ensure(msgpack_packer_write_array_value_cleared, (VALUE) &args,
            msgpack_packer_decrement_depth, (VALUE) pk);
}

static int write_hash_foreach(VALUE key, VALUE value, VALUE pk_value)
{
    if (key == Qundef) {
        return ST_CONTINUE;
    }
    msgpack_packer_t* pk = (msgpack_packer_t*) pk_value;
    msgpack_packer_write_value(pk, key);
    msgpack_packer_write_value(pk, value);
    return ST_CONTINUE;
}

static VALUE msgpack_packer_write_hash_value_cleared(VALUE p)
{
    struct msgpack_packer_write_collection_args* args = (void*) p;
    msgpack_packer_t* pk = args->pk;
    VALUE v = args->v;

    /* actual return type of RHASH_SIZE is long (if SIZEOF_LONG == SIZEOF_VOIDP
     * or long long (if SIZEOF_LONG_LONG == SIZEOF_VOIDP. See st.h. */
    unsigned long len = RHASH_SIZE(v);
    if(len > 0xffffffffUL) {
        rb_raise(rb_eArgError, "size of array is too long to pack: %ld bytes should be <= %lu", len, 0xffffffffUL);
    }
    unsigned int len32 = (unsigned int)len;
    msgpack_packer_write_map_header(pk, len32);

#ifdef RUBINIUS
    VALUE iter = rb_funcall(v, s_to_iter, 0);
    VALUE entry = Qnil;
    while(RTEST(entry = rb_funcall(iter, s_next, 1, entry))) {
        VALUE key = rb_funcall(entry, s_key, 0);
        VALUE val = rb_funcall(entry, s_value, 0);
        write_hash_foreach(key, val, (VALUE) pk);
    }
#else
    rb_hash_foreach(v, write_hash_foreach, (VALUE) pk);
#endif

    return Qnil;
}

void msgpack_packer_write_hash_value(msgpack_packer_t* pk, VALUE v) {
    msgpack_packer_increment_depth(pk);

    struct msgpack_packer_write_collection_args args = (struct msgpack_packer_write_collection_args){
        .pk = pk,
        .v = v,
    };
    rb_ensure(msgpack_packer_write_hash_value_cleared, (VALUE) &args,
            msgpack_packer_decrement_depth, (VALUE) pk);
}

void msgpack_packer_write_other_value(msgpack_packer_t* pk, VALUE v)
{
    int ext_type;

    VALUE lookup_class;

    /*
     * Objects of type Integer (Fixnum, Bignum), Float, Symbol and frozen
     * String have no singleton class and raise a TypeError when trying to get
     * it. See implementation of #singleton_class in ruby's source code:
     * VALUE rb_singleton_class(VALUE obj);
     *
     * Since all but symbols are already filtered out when reaching this code
     * only symbols are checked here.
     */
    if (SYMBOL_P(v)) {
      lookup_class = rb_obj_class(v);
    } else {
      lookup_class = rb_singleton_class(v);
    }

    VALUE proc = msgpack_packer_ext_registry_lookup(&pk->ext_registry, lookup_class,
      &ext_type);

    if(proc != Qnil) {
        VALUE payload = rb_funcall(proc, s_call, 1, v);
        StringValue(payload);
        msgpack_packer_write_ext(pk, ext_type, payload);
    } else {
        rb_funcall(v, pk->to_msgpack_method, 1, pk->to_msgpack_arg);
    }
}

void msgpack_packer_write_value(msgpack_packer_t* pk, VALUE v)
{
    switch(rb_type(v)) {
    case T_NIL:
        msgpack_packer_write_nil(pk);
        break;
    case T_TRUE:
        msgpack_packer_write_true(pk);
        break;
    case T_FALSE:
        msgpack_packer_write_false(pk);
        break;
    case T_FIXNUM:
        msgpack_packer_write_fixnum_value(pk, v);
        break;
    case T_SYMBOL:
        msgpack_packer_write_symbol_value(pk, v);
        break;
    case T_STRING:
        msgpack_packer_write_string_value(pk, v);
        break;
    case T_ARRAY:
        msgpack_packer_write_array_value(pk, v);
        break;
    case T_HASH:
        msgpack_packer_write_hash_value(pk, v);
        break;
    case T_BIGNUM:
        msgpack_packer_write_bignum_value(pk, v);
        break;
    case T_FLOAT:
        msgpack_packer_write_float_value(pk, v);
        break;
    default:
        msgpack_packer_write_other_value(pk, v);
    }
}

