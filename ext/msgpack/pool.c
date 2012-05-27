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

#include "pool.h"

void msgpack_pool_init(msgpack_pool_t* pl,
        size_t alloc_size, size_t pool_size)
{
    memset(pl, 0, sizeof(msgpack_pool_t));

    pl->array_head = calloc(pool_size, sizeof(void*));
    pl->array_tail = pl->array_head;
    pl->array_end = pl->array_head + pool_size;
    pl->alloc_size = alloc_size;
}

void msgpack_pool_destroy(msgpack_pool_t* pl)
{
    void** p = pl->array_head;
    for(; p < pl->array_end; p++) {
        free(*p);
    }
    free(pl->array_head);
}

void* msgpack_pool_malloc(msgpack_pool_t* pl,
        size_t required_size, size_t* allocated_size)
{
#ifdef USE_STR_NEW_MOVE
    /* +1: for rb_str_new_move */
    required_size += 1;
#endif
    if(required_size > pl->alloc_size) {
        *allocated_size = required_size;
        return malloc(required_size);
    }
    if(pl->array_head == pl->array_tail) {
        *allocated_size = pl->alloc_size;
        return malloc(pl->alloc_size);
    }
    *allocated_size = pl->alloc_size;
    return pl->array_tail--;
}

void* msgpack_pool_realloc(msgpack_pool_t* pl,
        void* ptr, size_t required_size, size_t* current_size)
{
    if(ptr == NULL) {
        return msgpack_pool_malloc(pl, required_size, current_size);
    }
#ifdef USE_STR_NEW_MOVE
    /* +1: for rb_str_new_move */
    required_size += 1;
#endif
    size_t next_size = *current_size * 2;
    while(next_size < required_size) {
        next_size *= 2;
    }
    *current_size = next_size;
    return realloc(ptr, next_size);
}

void msgpack_pool_free(msgpack_pool_t* pl,
        void* ptr, size_t size)
{
    if(pl->array_end == pl->array_tail || pl->alloc_size < size) {
        free(ptr);
    }
    *pl->array_tail = ptr;
    pl->array_tail++;
}

msgpack_pool_t msgpack_pool_static_instance;

#ifdef USE_STR_NEW_MOVE
/* note: this code is magical: */
static VALUE rb_str_new_move(char* data, size_t length, size_t capacity)
{
    NEWOBJ(str, struct RString);
    OBJSETUP(str, rb_cString, T_STRING);

    str->as.heap.ptr = data;
    str->as.heap.len = length;
    str->as.heap.aux.capa = capacity;

    FL_SET(str, FL_USER1);  /* set STR_NOEMBED */
    RBASIC(str)->flags &= ~RSTRING_EMBED_LEN_MASK;

    /* this is safe. see msgpack_pool_malloc() */
    data[length] = '\0';

    /* ???: unknown whether this is needed or not */
    OBJ_FREEZE(str);

    return (VALUE) str;
}
#endif

VALUE msgpack_pool_move_to_string(msgpack_pool_t* pl,
        void* ptr, size_t offset, size_t size)
{
    VALUE s = rb_str_new_move(ptr, offset+size, offset+size);
    if(offset > 0) {
        s = rb_str_substr(s, offset, size);
    } else {
        s = rb_str_dup(s);
    }
    return s;
}

