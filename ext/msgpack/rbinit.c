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

#include "buffer_class.h"
#include "packer_class.h"
#include "unpacker_class.h"
#include "core_ext.h"

#ifdef COMPAT_HAVE_ENCODING
/* see compat.h*/
int s_enc_utf8;
int s_enc_ascii8bit;
int s_enc_usascii;
VALUE s_enc_utf8_value;
#endif

void Init_msgpack(void)
{
#ifdef COMPAT_HAVE_ENCODING
    s_enc_ascii8bit = rb_ascii8bit_encindex();
    s_enc_utf8 = rb_utf8_encindex();
    s_enc_usascii = rb_usascii_encindex();
    s_enc_utf8_value = rb_enc_from_encoding(rb_utf8_encoding());
#endif

    VALUE mMessagePack = rb_define_module("MessagePack");

    MessagePack_Buffer_module_init(mMessagePack);
    MessagePack_Packer_module_init(mMessagePack);
    MessagePack_Unpacker_module_init(mMessagePack);
    MessagePack_core_ext_module_init();
}

