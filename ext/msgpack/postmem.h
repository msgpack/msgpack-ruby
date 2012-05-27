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
#ifndef MSGPACK_RUBY_POSTMEM_H__
#define MSGPACK_RUBY_POSTMEM_H__

#include "compat.h"
#include "sysdep.h"

struct msgpack_postmem_t;
typedef struct msgpack_postmem_t msgpack_postmem_t;

struct msgpack_postmem_chunk_t;
typedef struct msgpack_postmem_chunk_t msgpack_postmem_chunk_t;

/*
 *   chunk
 *   +----------------+
 *   |   alloc_size   |
 *   +----------------+
 *   ^
 * +-|-----+-------+-------+-------+
 * | void* | void* |       |       |
 * +-------+-------+-------+-------+
 * ^array_head     ^array_tail     ^array_end
 *
 * +---------------------------+
 * array_end - array_head = alloc_size
 */
struct msgpack_postmem_t {
    void** array_tail;
    void** array_head;
    void** array_end;
    size_t alloc_size;
};

void msgpack_postmem_init(msgpack_postmem_t* pm,
        size_t alloc_size, size_t pool_size);

void msgpack_postmem_destroy(msgpack_postmem_t* pm);

void* msgpack_postmem_alloc(msgpack_postmem_t* pm,
        size_t required_size, size_t* allocated_size);

void* msgpack_postmem_realloc(msgpack_postmem_t* pm,
        void* ptr, size_t required_size, size_t* current_size);

void msgpack_postmem_free(msgpack_postmem_t* pm,
        void* ptr, size_t size_hint);

#ifdef USE_STR_NEW_MOVE
/* assert size > RSTRING_EMBED_LEN_MAX */
VALUE msgpack_postmem_move_to_string(msgpack_postmem_t* pm,
        void* ptr, size_t size);
#endif


#endif

