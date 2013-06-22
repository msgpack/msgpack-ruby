#ifndef MSGPACK_RUBY_EXTENDED_CLASS_H__
#define MSGPACK_RUBY_EXTENDED_CLASS_H__

#include "compat.h"

struct msgpack_extended_t;
typedef struct msgpack_extended_t msgpack_extended_t;

struct msgpack_extended_t {
    VALUE type;
    VALUE data;
};

extern VALUE cMessagePack_Extended;

void MessagePack_Extended_module_init(VALUE mMessagePack);

void MessagePack_Extended_initialize(VALUE self, VALUE type, VALUE data);

#endif
