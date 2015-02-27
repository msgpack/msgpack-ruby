/*
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2015 Sadayuki Furuhashi, Scott Francis
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

#include "custom.h"
#include "packer.h"
#include "packer_class.h"
#include "core_ext.h"

static ID s_to_msgpack;
static ID s_dump;
static ID s_load;
static VALUE mMarshal;
static VALUE ePackError;

static st_table *msgpack_custom_types;

#define MSGPACK_EXT_TYPE_RUBY 1

VALUE msgpack_custom_unpack_type(const char *buffer, size_t sz)
{
    const char *p = buffer;
    size_t data_sz;

    if (*p++ != MSGPACK_EXT_TYPE_RUBY) {
        return Qnil;
    }

    /* only deserialize types that we've registered */
    if (!st_lookup(msgpack_custom_types, (st_data_t) p, NULL)) {
        return Qnil;
    }

    p += strlen(p) + 1;
    data_sz = sz - (p - buffer) + 1;
    return rb_funcall(mMarshal, s_load, 1, rb_str_new(p, data_sz));
}

static unsigned char get_ext_byte(size_t sz)
{
    if (sz >= 16 && sz < 256)
        return 0xc7;
    else if (sz >= 256 && sz < (1UL << 16))
        return 0xc8;
    else if (sz <= 4)
        return 0xd6;
    else if (sz > 4 && sz <= 8)
        return 0xd7;
    else if (sz > 8 && sz <= 16)
        return 0xd8;
    else if (sz >= (1UL << 16) && sz < (1UL << 32))
        return 0xc9;
    else
        rb_raise(rb_eArgError, "cannot serialize %zu byte object", sz);

    return 0;
}

static VALUE custom_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    const char * clsname;
    VALUE data;
    unsigned char ext;
    size_t header_sz = 0, body_sz = 0, data_sz = 0, clsname_sz;

    ENSURE_PACKER(argc, argv, packer, pk);

    clsname = rb_obj_classname(self);
    clsname_sz = strlen(clsname);
    data = rb_funcall(mMarshal, s_dump, 1, self);
    Check_Type(data, T_STRING);

    /*
     * see https://github.com/msgpack/msgpack/blob/master/spec.md#formats-ext
     *
     * format of the packed object is class_name\0marshal_data
     */
    data_sz = body_sz = clsname_sz + 1 + RSTRING_LEN(data);
    ext = get_ext_byte(body_sz);
    switch (ext & 0xf0) {
    case 0xd0:
        header_sz = 3;
        body_sz = 1 << (ext - 0xd4);
        break;
    case 0xc0:
        header_sz = 2 + (1 << (ext - 0xc7));
        break;
    default:
        rb_raise(ePackError, "cannot serialize %s class", clsname);
    }

    msgpack_buffer_ensure_writable(PACKER_BUFFER_(pk), header_sz + body_sz);
    msgpack_buffer_write_1(PACKER_BUFFER_(pk), ext);

    if (ext == 0xc7) {
        msgpack_buffer_write_1(PACKER_BUFFER_(pk), (uint8_t) data_sz);
    } else if (ext == 0xc8) {
        uint16_t be = _msgpack_be16((uint16_t) data_sz);
        msgpack_buffer_write_data(PACKER_BUFFER_(pk), (void *) &be, 2);
    } else if (ext == 0xc9) {
        uint32_t be = _msgpack_be32((uint32_t) data_sz);
        msgpack_buffer_write_data(PACKER_BUFFER_(pk), (void *) &be, 4);
    }

    msgpack_buffer_write_1(PACKER_BUFFER_(pk), MSGPACK_EXT_TYPE_RUBY);
    msgpack_buffer_write_data(PACKER_BUFFER_(pk), (void *) clsname, clsname_sz);
    msgpack_buffer_write_1(PACKER_BUFFER_(pk), '\0');
    msgpack_buffer_write_data(PACKER_BUFFER_(pk), RSTRING_PTR(data), RSTRING_LEN(data));

    if ((ext & 0xf0) == 0xd0) {
        /* pad the rest of the data */
        for (size_t i = 0; i < body_sz - data_sz; i++) {
            msgpack_buffer_write_1(PACKER_BUFFER_(pk), '\0');
        }
    }

    return packer;
}

static VALUE MessagePack_register_type_module_method(VALUE self, VALUE cls)
{
    const char * clsname;

    Check_Type(cls, T_CLASS);

    clsname = rb_class2name(cls);
    if (!st_lookup(msgpack_custom_types, (st_data_t) clsname, NULL)) {
        if (!rb_method_boundp(cls, s_to_msgpack, 0)) {
            rb_define_method(cls, "to_msgpack", custom_to_msgpack, -1);
        }
        st_insert(msgpack_custom_types, (st_data_t) clsname, 1);
    }

    return Qnil;
}

void MessagePack_custom_module_init(VALUE mMessagePack)
{
    s_to_msgpack = rb_intern("to_msgpack");
    s_dump = rb_intern("dump");
    s_load = rb_intern("load");

    mMarshal = rb_define_module("Marshal");

    msgpack_custom_types = st_init_strtable();

    ePackError = rb_define_class_under(mMessagePack, "PackError", rb_eStandardError);
    rb_define_module_function(mMessagePack, "register_type", MessagePack_register_type_module_method, 1);
}
