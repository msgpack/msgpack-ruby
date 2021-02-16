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

#include "packer_ext_registry.h"

static ID s_call;

void msgpack_packer_ext_registry_static_init()
{
    s_call = rb_intern("call");
}

void msgpack_packer_ext_registry_static_destroy()
{ }

void msgpack_packer_ext_registry_init(msgpack_packer_ext_registry_t* pkrg)
{
    pkrg->hash = rb_hash_new();
    pkrg->cache = rb_hash_new();
}

void msgpack_packer_ext_registry_mark(msgpack_packer_ext_registry_t* pkrg)
{
    rb_gc_mark(pkrg->hash);
    rb_gc_mark(pkrg->cache);
}

void msgpack_packer_ext_registry_dup(msgpack_packer_ext_registry_t* src,
        msgpack_packer_ext_registry_t* dst)
{
    dst->hash = rb_hash_dup(src->hash);
    dst->cache = rb_hash_dup(src->cache);
}

VALUE msgpack_packer_ext_registry_put(msgpack_packer_ext_registry_t* pkrg,
        VALUE ext_module, int ext_type, VALUE proc, VALUE arg)
{
    VALUE e = rb_ary_new3(3, INT2FIX(ext_type), proc, arg);
    /* clear lookup cache not to miss added type */
    rb_hash_clear(pkrg->cache);
    return rb_hash_aset(pkrg->hash, ext_module, e);
}
