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

#include "core_ext.h"
#include "packer.h"
#include "packer_class.h"
#include "extension_value_class.h"

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

static VALUE NilClass_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_nil(pk);
    return packer;
}

static VALUE TrueClass_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_true(pk);
    return packer;
}

static VALUE FalseClass_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_false(pk);
    return packer;
}

static VALUE Integer_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    if (FIXNUM_P(self)) {
        msgpack_packer_write_fixnum_value(pk, self);
    } else {
        msgpack_packer_write_bignum_value(pk, self);
    }
    return packer;
}

static VALUE Fixnum_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_fixnum_value(pk, self);
    return packer;
}

static VALUE Bignum_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_bignum_value(pk, self);
    return packer;
}

static VALUE Float_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_float_value(pk, self);
    return packer;
}

static VALUE String_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_string_value(pk, self);
    return packer;
}

static VALUE Array_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_array_value(pk, self);
    return packer;
}

static VALUE Hash_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_hash_value(pk, self);
    return packer;
}

static VALUE Symbol_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_symbol_value(pk, self);
    return packer;
}

static VALUE ExtensionValue_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    int ext_type = FIX2INT(RSTRUCT_GET(self, 0));
    if(ext_type < -128 || ext_type > 127) {
        rb_raise(rb_eRangeError, "integer %d too big to convert to `signed char'", ext_type);
    }
    VALUE payload = RSTRUCT_GET(self, 1);
    StringValue(payload);
    msgpack_packer_write_ext(pk, ext_type, payload);
    return packer;
}

void MessagePack_core_ext_module_init()
{
    rb_define_method(rb_cNilClass,   "to_msgpack", NilClass_to_msgpack, -1);
    rb_define_method(rb_cTrueClass,  "to_msgpack", TrueClass_to_msgpack, -1);
    rb_define_method(rb_cFalseClass, "to_msgpack", FalseClass_to_msgpack, -1);
#ifdef RUBY_INTEGER_UNIFICATION
    rb_define_method(rb_cInteger, "to_msgpack", Integer_to_msgpack, -1);
#else
    rb_define_method(rb_cFixnum, "to_msgpack", Fixnum_to_msgpack, -1);
    rb_define_method(rb_cBignum, "to_msgpack", Bignum_to_msgpack, -1);
#endif
    rb_define_method(rb_cFloat,  "to_msgpack", Float_to_msgpack, -1);
    rb_define_method(rb_cString, "to_msgpack", String_to_msgpack, -1);
    rb_define_method(rb_cArray,  "to_msgpack", Array_to_msgpack, -1);
    rb_define_method(rb_cHash,   "to_msgpack", Hash_to_msgpack, -1);
    rb_define_method(rb_cSymbol, "to_msgpack", Symbol_to_msgpack, -1);
    rb_define_method(cMessagePack_ExtensionValue, "to_msgpack", ExtensionValue_to_msgpack, -1);
}

