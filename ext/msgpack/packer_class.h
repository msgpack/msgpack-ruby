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
#ifndef MSGPACK_RUBY_PACKER_CLASS_H__
#define MSGPACK_RUBY_PACKER_CLASS_H__

#include "packer.h"

extern VALUE cMessagePack_Packer;

void MessagePack_Packer_module_init(VALUE mMessagePack);

VALUE MessagePack_pack(int argc, VALUE* argv);

static inline VALUE delegete_to_pack(int argc, VALUE* argv, VALUE self)
{
    if(argc == 0) {
        return MessagePack_pack(1, &self);
    } else if(argc == 1) {
        /* write to io */
        VALUE argv2[2];
        argv2[0] = self;
        argv2[1] = argv[0];
        return MessagePack_pack(2, argv2);
    } else {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0..1)", argc);
    }
}

#define ENSURE_PACKER(argc, argv, packer, pk) \
    if(argc != 1 || rb_class_of(argv[0]) != cMessagePack_Packer) { \
        return delegete_to_pack(argc, argv, self); \
    } \
    VALUE packer = argv[0]; \
    msgpack_packer_t *pk; \
    Data_Get_Struct(packer, msgpack_packer_t, pk);


#endif
