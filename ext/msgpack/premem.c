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

#include "premem.h"

void msgpack_premem_init(msgpack_premem_t* pm,
        size_t alloc_size)
{
    memset(pm, 0, sizeof(msgpack_premem_t));
    pm->alloc_size = alloc_size;
    pm->head.buffers = malloc(alloc_size*32);
    pm->head.mask = 0xffffffff;  /* all bit is 1 = available */
}

void msgpack_premem_destroy(msgpack_premem_t* pm)
{
    msgpack_premem_chunk_t* c = &pm->head;
    for(; c != NULL; c = c->next) {
        free(c->buffers);
    }
}

void* _msgpack_premem_alloc2(msgpack_premem_t* pm)
{
    msgpack_premem_chunk_t* c = pm->head.next;
    for(; c != NULL; c = c->next) {
        _msgpack_premem_chunk_try_alloc(c, pm->alloc_size);
    }

    /* allocate new chunk */
    c = calloc(1, sizeof(msgpack_premem_chunk_t));
    *c = pm->head;
    pm->head.mask = 0xffffffff;
    pm->head.buffers = malloc(32*pm->alloc_size);
    pm->head.next = c;

    _msgpack_premem_chunk_alloc(&pm->head, pm->alloc_size);
}

bool _msgpack_premem_free2(msgpack_premem_t* pm, void* ptr)
{
    msgpack_premem_chunk_t* c = pm->head.next;
    for(; c != NULL; c = c->next) {
        _msgpack_premem_chunk_try_free(c, ptr, pm->alloc_size);
    }
    return false;
}

bool msgpack_premem_check(msgpack_premem_t* pm, void* ptr)
{
    msgpack_premem_chunk_t* c = &pm->head;
    for(; c != NULL; c = c->next) {
        _msgpack_premem_chunk_try_check(c, ptr, pm->alloc_size);
    }
    return false;
}

