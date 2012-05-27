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
#ifndef MSGPACK_RUBY_PREMEM_H__
#define MSGPACK_RUBY_PREMEM_H__

#include "compat.h"
#include "sysdep.h"

struct msgpack_premem_t;
typedef struct msgpack_premem_t msgpack_premem_t;

struct msgpack_premem_chunk_t;
typedef struct msgpack_premem_chunk_t msgpack_premem_chunk_t;

/*
 * a chunk contains buffers.
 * size of each buffer is alloc_size bytes.
 */
struct msgpack_premem_chunk_t {
    unsigned int mask;
    char* buffers;
    msgpack_premem_chunk_t* next;
};

struct msgpack_premem_t {
    msgpack_premem_chunk_t head;
    size_t alloc_size;
};

void msgpack_premem_init(msgpack_premem_t* pm,
        size_t alloc_size);

void msgpack_premem_destroy(msgpack_premem_t* pm);

void* _msgpack_premem_alloc2(msgpack_premem_t* pm);

#define _msgpack_premem_chunk_alloc(c, alloc_size) \
    _msgpack_bsp32(pos, (c)->mask); \
    (c)->mask &= ~(1 << pos); \
    return ((char*)(c)->buffers) + (pos * (alloc_size))

#define _msgpack_premem_chunk_try_alloc(c, alloc_size) \
    if((c)->mask != 0) { \
        _msgpack_premem_chunk_alloc(c, alloc_size); \
    }

#define _msgpack_premem_chunk_try_free(c, ptr, alloc_size) \
    { \
        ptrdiff_t pdiff = ((char*)(ptr)) - ((char*)(c)->buffers); \
        if(0 <= pdiff) { \
            size_t pos = pdiff / alloc_size; \
            if(pos < 32) { \
                (c)->mask |= (1 << pos); \
                return true; \
            } \
        } \
    }

static inline void* msgpack_premem_alloc(msgpack_premem_t* pm)
{
    _msgpack_premem_chunk_try_alloc(&pm->head, pm->alloc_size);
    return _msgpack_premem_alloc2(pm);
}

bool _msgpack_premem_free2(msgpack_premem_t* pm, void* ptr);

static inline bool msgpack_premem_free(msgpack_premem_t* pm, void* ptr)
{
    _msgpack_premem_chunk_try_free(&pm->head, ptr, pm->alloc_size);
    _msgpack_premem_free2(pm, ptr);
}

#endif

