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
};

void msgpack_packer_ext_registry_static_init();

void msgpack_packer_ext_registry_static_destroy();

void msgpack_packer_ext_registry_init(msgpack_packer_ext_registry_t* pkrg);

void msgpack_packer_ext_registry_destroy(msgpack_packer_ext_registry_t* pkrg);

void msgpack_packer_ext_registry_mark(msgpack_packer_ext_registry_t* pkrg);

void msgpack_packer_ext_registry_dup(msgpack_packer_ext_registry_t* src,
        msgpack_packer_ext_registry_t* dst);

VALUE msgpack_packer_ext_registry_put(msgpack_packer_ext_registry_t* pkrg,
        VALUE ext_class, int ext_type, VALUE proc);

static inline VALUE msgpack_packer_ext_registry_lookup(msgpack_packer_ext_registry_t* pkrg,
        VALUE ext_class, int* ext_type_result)
{
    VALUE e = rb_hash_lookup(pkrg->hash, ext_class);
    if(e == Qnil) {
        return Qnil;
    }
    *ext_type_result = FIX2INT(rb_ary_entry(e, 0));
    return rb_ary_entry(e, 1);
}

VALUE msgpack_packer_ext_registry_call(msgpack_packer_ext_registry_t* pkrg,
        VALUE proc, VALUE ext_value);

#endif
