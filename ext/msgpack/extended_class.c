#include "compat.h"
#include "ruby.h"
#include "extended_class.h"
#include "packer.h"
#include "packer.h"
#include "packer_class.h"

VALUE cMessagePack_Extended;

#define EXTENDED(from, name) \
    msgpack_extended_t *name = NULL; \
    Data_Get_Struct(from, msgpack_extended_t, name); \
    if(name == NULL) { \
        rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
    }

static void Extended_free(void* data)
{
    if(data == NULL) {
        return;
    }
    msgpack_extended_t* ext = (msgpack_extended_t*) data;
    free(ext);
}

void msgpack_extended_mark(msgpack_extended_t* ext)
{
    rb_gc_mark(ext->type);
    rb_gc_mark(ext->data);
}


void msgpack_extended_init(msgpack_extended_t* ext)
{
    memset(ext, 0, sizeof(msgpack_extended_t));

    ext->data = Qnil;
    ext->type = Qnil;
}

static VALUE Extended_alloc(VALUE klass)
{
    msgpack_extended_t* ext = ALLOC_N(msgpack_extended_t, 1);
    msgpack_extended_init(ext);

    return Data_Wrap_Struct(klass, msgpack_extended_mark, Extended_free, ext);
}

static VALUE Extended_initialize(VALUE self, VALUE type, VALUE data)
{
    EXTENDED(self, ext);
    Check_Type(type, T_FIXNUM);

    if ((FIX2INT(type) < INT8_MIN) || (FIX2INT(type) > INT8_MAX)) {
        rb_raise(rb_eRangeError, "type should be <= %d and >= %d", INT8_MIN, INT8_MAX);
    }

    ext->type = type;
    ext->data = StringValue(data);

    return self;
}

static VALUE Extended_data(VALUE self)
{
    EXTENDED(self, ext);
    return ext->data;
}

static VALUE Extended_type(VALUE self)
{
    EXTENDED(self, ext);
    return ext->type;
}

static VALUE Extended_to_msgpack(int argc, VALUE* argv, VALUE self)
{
    ENSURE_PACKER(argc, argv, packer, pk);
    msgpack_packer_write_ext_value(pk, self);
    return packer;
}

static VALUE Extended_to_eql(VALUE self, VALUE other)
{
    EXTENDED(self, ext);
    EXTENDED(other, other_ext);

    if(ext->type != other_ext->type) {
        return Qfalse;
    }

    unsigned long self_len  = RSTRING_LEN((ext->data));
    unsigned long other_len = RSTRING_LEN((other_ext->data));

    if(self_len != other_len) {
        return Qfalse;
    }

    if(memcmp(StringValuePtr(ext->data), StringValuePtr(other_ext->data), self_len) != 0) {
        return Qfalse;
    }

    return Qtrue;
}


void MessagePack_Extended_module_init(VALUE mMessagePack)
{
    cMessagePack_Extended = rb_define_class_under(mMessagePack, "Extended", rb_cObject);

    rb_define_alloc_func(cMessagePack_Extended, Extended_alloc);

    rb_define_method(cMessagePack_Extended, "initialize", Extended_initialize, 2);
    rb_define_method(cMessagePack_Extended, "type", Extended_type, 0);
    rb_define_method(cMessagePack_Extended, "data", Extended_data, 0);
    rb_define_method(cMessagePack_Extended, "to_msgpack", Extended_to_msgpack, -1);
    rb_define_method(cMessagePack_Extended, "==", Extended_to_eql, 1);
    rb_define_method(cMessagePack_Extended, "eql?", Extended_to_eql, 1);
}

