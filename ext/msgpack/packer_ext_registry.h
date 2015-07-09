/*
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2015 Sadayuki Furuhashi
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
#ifndef MSGPACK_RUBY_UNPACKER_PACKER_EXT_REGISTRY_H__
#define MSGPACK_RUBY_UNPACKER_PACKER_EXT_REGISTRY_H__

#include "compat.h"
#include "ruby.h"

struct msgpack_packer_ext_registry_t;
typedef struct msgpack_packer_ext_registry_t msgpack_packer_ext_registry_t;

struct msgpack_packer_ext_registry_t {
    VALUE hash;
    /*
     * lookup cache for subclasses of registered classes
     */
    VALUE cache;
};

void msgpack_packer_ext_registry_static_init();

void msgpack_packer_ext_registry_static_destroy();

void msgpack_packer_ext_registry_init(msgpack_packer_ext_registry_t* pkrg);

static inline void msgpack_packer_ext_registry_destroy(msgpack_packer_ext_registry_t* pkrg)
{ }

void msgpack_packer_ext_registry_mark(msgpack_packer_ext_registry_t* pkrg);

void msgpack_packer_ext_registry_dup(msgpack_packer_ext_registry_t* src,
        msgpack_packer_ext_registry_t* dst);

VALUE msgpack_packer_ext_registry_put(msgpack_packer_ext_registry_t* pkrg,
        VALUE ext_class, int ext_type, VALUE proc, VALUE arg);

static int msgpack_packer_ext_find_inherited(VALUE key, VALUE value, VALUE arg)
{
    VALUE *args = (VALUE *) arg;
    if(key == Qundef) {
        return ST_CONTINUE;
    }
    if(rb_class_inherited_p(args[0], key) == Qtrue) {
        args[1] = key;
        return ST_STOP;
    }
    return ST_CONTINUE;
}


static inline VALUE msgpack_packer_ext_registry_lookup(msgpack_packer_ext_registry_t* pkrg,
        VALUE ext_class, int* ext_type_result)
{
    VALUE e = rb_hash_lookup(pkrg->hash, ext_class);
    if(e != Qnil) {
        *ext_type_result = FIX2INT(rb_ary_entry(e, 0));
        return rb_ary_entry(e, 1);
    }

    VALUE c = rb_hash_lookup(pkrg->cache, ext_class);
    if(c != Qnil) {
        *ext_type_result = FIX2INT(rb_ary_entry(c, 0));
        return rb_ary_entry(c, 1);
    }

    /*
     * check all keys whether it is super class of ext_class, or not
     */
    VALUE args[2];
    args[0] = ext_class;
    args[1] = Qnil;

#ifdef RUBINIUS
    ID s_next = rb_intern("next");
    ID s_key = rb_intern("key");
    ID s_value = rb_intern("value");
    VALUE iter = rb_funcall(v, rb_intern("to_iter"), 0);
    VALUE entry = Qnil;
    while(RTEST(entry = rb_funcall(iter, s_next, 1, entry))) {
        VALUE key = rb_funcall(entry, s_key, 0);
        VALUE val = rb_funcall(entry, s_value, 0);
        msgpack_packer_ext_find_inherited(key, val, ext_class, (VALUE) args);
    }
#else
    rb_hash_foreach(pkrg->hash, msgpack_packer_ext_find_inherited, (VALUE) args);
#endif

    VALUE hit = args[1];
    if(hit != Qnil) {
        rb_hash_aset(pkrg->cache, ext_class, hit);
        return hit;
    }

    return Qnil;
}

VALUE msgpack_packer_ext_registry_call(msgpack_packer_ext_registry_t* pkrg,
        VALUE proc, VALUE ext_value);

#endif
