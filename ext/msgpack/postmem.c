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

#include "postmem.h"

void msgpack_postmem_init(msgpack_postmem_t* pm,
        size_t alloc_size, size_t pool_size)
{
    memset(pm, 0, sizeof(msgpack_postmem_t));

    pm->array_head = calloc(pool_size, sizeof(void*));
    pm->array_tail = pm->array_head;
    pm->array_end = pm->array_head + pool_size;
    pm->alloc_size = alloc_size;
}

void msgpack_postmem_destroy(msgpack_postmem_t* pm)
{
    void** p = pm->array_head;
    for(; p < pm->array_end; p++) {
        free(*p);
    }
    free(pm->array_head);
}

void* msgpack_postmem_alloc(msgpack_postmem_t* pm,
        size_t required_size, size_t* allocated_size)
{
#ifndef DISABLE_STR_NEW_MOVE
    /* +1: for rb_str_new_move */
    required_size += 1;
#endif
    if(required_size > pm->alloc_size) {
#ifndef DISABLE_STR_NEW_MOVE
        *allocated_size = required_size - 1;
#else
        *allocated_size = required_size;
#endif
        return malloc(required_size);
    }
    if(pm->array_head == pm->array_tail) {
#ifndef DISABLE_STR_NEW_MOVE
        *allocated_size = pm->alloc_size - 1;
#else
        *allocated_size = pm->alloc_size;
#endif
        return malloc(pm->alloc_size);
    }
#ifndef DISABLE_STR_NEW_MOVE
    *allocated_size = pm->alloc_size - 1;
#else
    *allocated_size = pm->alloc_size;
#endif
    pm->array_tail--;
    return *pm->array_tail;
}

void* msgpack_postmem_realloc(msgpack_postmem_t* pm,
        void* ptr, size_t required_size, size_t* current_size)
{
    if(ptr == NULL) {
        return msgpack_postmem_alloc(pm, required_size, current_size);
    }
#ifndef DISABLE_STR_NEW_MOVE
    /* +1: for rb_str_new_move */
    required_size += 1;
#endif
    size_t next_size = *current_size * 2;
    while(next_size < required_size) {
        next_size *= 2;
    }
#ifndef DISABLE_STR_NEW_MOVE
    *current_size = next_size - 1;
#else
    *current_size = next_size;
#endif
    return realloc(ptr, next_size);
}

void msgpack_postmem_free(msgpack_postmem_t* pm,
        void* ptr, size_t size_hint)
{
    if(pm->array_end == pm->array_tail
            /* free reallocated buffer */
            || pm->alloc_size < size_hint) {
        free(ptr);
        return;
    }
    *pm->array_tail = ptr;
    pm->array_tail++;
}

#ifndef DISABLE_STR_NEW_MOVE
/* note: this code is magical: */
static VALUE rb_str_new_move(char* data, size_t length)
{
    NEWOBJ(str, struct RString);
    OBJSETUP(str, rb_cString, T_STRING);

    str->as.heap.ptr = data;
    str->as.heap.len = length;
    str->as.heap.aux.capa = length;

    FL_SET(str, FL_USER1);  /* set STR_NOEMBED */
    RBASIC(str)->flags &= ~RSTRING_EMBED_LEN_MASK;

    /* this is safe. see msgpack_postmem_alloc() */
    data[length] = '\0';

    /* ???: unknown whether this is needed or not */
    OBJ_FREEZE(str);

    return (VALUE) str;
}
#endif

VALUE msgpack_postmem_move_to_string(msgpack_postmem_t* pm,
        void* ptr, size_t size)
{
    return rb_str_new_move(ptr, size);
}

