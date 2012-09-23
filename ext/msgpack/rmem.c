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

#include "rmem.h"

void msgpack_rmem_init(msgpack_rmem_t* pm)
{
    memset(pm, 0, sizeof(msgpack_rmem_t));
    pm->head.pages = malloc(MSGPACK_RMEM_PAGE_SIZE * 32);
    pm->head.mask = 0xffffffff;  /* all bit is 1 = available */
}

void msgpack_rmem_destroy(msgpack_rmem_t* pm)
{
    msgpack_rmem_chunk_t* c = &pm->head;
    for(; c != NULL; c = c->next) {
        free(c->pages);
    }
}

void* _msgpack_rmem_alloc2(msgpack_rmem_t* pm)
{
    msgpack_rmem_chunk_t* c = pm->head.next;
    for(; c != NULL; c = c->next) {
        _msgpack_rmem_chunk_try_alloc(c);
    }

    /* allocate new chunk */
    c = calloc(1, sizeof(msgpack_rmem_chunk_t));
    *c = pm->head;
    pm->head.mask = 0xffffffff & (~1);  /* first chunk is already allocated */
    pm->head.pages = malloc(MSGPACK_RMEM_PAGE_SIZE * 32);
    pm->head.next = c;

    return pm->head.pages;
}

bool _msgpack_rmem_free2(msgpack_rmem_t* pm, void* mem)
{
    msgpack_rmem_chunk_t* c = pm->head.next;
    for(; c != NULL; c = c->next) {
        _msgpack_rmem_chunk_try_free(c, mem);
    }
    return false;
}

bool msgpack_rmem_check(msgpack_rmem_t* pm, void* mem)
{
    msgpack_rmem_chunk_t* c = &pm->head;
    for(; c != NULL; c = c->next) {
        _msgpack_rmem_chunk_try_check(c, mem);
    }
    return false;
}

