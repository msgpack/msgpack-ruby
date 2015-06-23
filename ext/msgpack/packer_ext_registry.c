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
static VALUE sc_entry_struct;

void msgpack_packer_ext_registry_static_init()
{
    s_call = rb_intern("call");
    sc_entry_struct = rb_struct_define_without_accessor(NULL, rb_cStruct, NULL, "index", "proc", NULL);
    //rb_obj_hide(sc_entry_struct);
}

void msgpack_packer_ext_registry_static_destroy()
{ }

void msgpack_packer_ext_registry_init(msgpack_packer_ext_registry_t* pkrg)
{
    pkrg->hash = rb_hash_new();
}

void msgpack_packer_ext_registry_destroy(msgpack_packer_ext_registry_t* pkrg)
{ }

void msgpack_packer_ext_registry_mark(msgpack_packer_ext_registry_t* pkrg)
{
    rb_gc_mark(pkrg->hash);
}

void msgpack_packer_ext_registry_dup(msgpack_packer_ext_registry_t* src,
        msgpack_packer_ext_registry_t* dst)
{
    dst->hash = rb_hash_dup(src->hash);
}

VALUE msgpack_packer_ext_registry_put(msgpack_packer_ext_registry_t* pkrg,
        VALUE ext_class, int ext_type, VALUE ext_proc)
{
    VALUE e = rb_struct_new(sc_entry_struct, INT2FIX(ext_type), ext_proc);
    return rb_hash_aset(pkrg->hash, ext_class, e);
}

VALUE msgpack_packer_ext_registry_call(msgpack_packer_ext_registry_t* pkrg,
        VALUE proc, VALUE ext_value)
{
    VALUE string = rb_funcall(proc, s_call, 1, ext_value);
    StringValue(string);
    return string;
}
