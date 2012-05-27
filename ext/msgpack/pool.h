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
#ifndef MSGPACK_RUBY_POOL_H__
#define MSGPACK_RUBY_POOL_H__

#include "compat.h"
#include "sysdep.h"

struct msgpack_pool_t;
typedef struct msgpack_pool_t msgpack_pool_t;

struct msgpack_pool_chunk_t;
typedef struct msgpack_pool_chunk_t msgpack_pool_chunk_t;

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
struct msgpack_pool_t {
    void** array_tail;
    void** array_head;
    void** array_end;
    size_t alloc_size;
};

#ifndef MSGPACK_POOL_CHUNK_SIZE
#define MSGPACK_POOL_CHUNK_SIZE 2*1024
#endif

#ifndef MSGPACK_POOL_SIZE
#define MSGPACK_POOL_SIZE 1024
#endif

void msgpack_pool_init(msgpack_pool_t* pl,
        size_t alloc_size, size_t pool_size);

static inline void msgpack_pool_init_default(msgpack_pool_t* pl)
{
    msgpack_pool_init(pl, MSGPACK_POOL_CHUNK_SIZE, MSGPACK_POOL_SIZE);
}

void msgpack_pool_destroy(msgpack_pool_t* pl);

void* msgpack_pool_malloc(msgpack_pool_t* pl,
        size_t required_size, size_t* allocated_size);

void* msgpack_pool_realloc(msgpack_pool_t* pl,
        void* ptr, size_t required_size, size_t* current_size);

void msgpack_pool_free(msgpack_pool_t* pl,
        void* ptr, size_t size);

#ifdef USE_STR_NEW_MOVE
/* assert size > RSTRING_EMBED_LEN_MAX */
VALUE msgpack_pool_move_to_string(msgpack_pool_t* pl,
        void* ptr, size_t offset, size_t size);
#endif


extern msgpack_pool_t msgpack_pool_static_instance;

static inline void msgpack_pool_static_init(
        size_t alloc_size, size_t pool_size)
{
    msgpack_pool_init(&msgpack_pool_static_instance, alloc_size, pool_size);
}

static inline void msgpack_pool_static_init_default()
{
    msgpack_pool_init_default(&msgpack_pool_static_instance);
}

static inline void msgpack_pool_static_destroy()
{
    msgpack_pool_destroy(&msgpack_pool_static_instance);
}

static inline void* msgpack_pool_static_malloc(
        size_t required_size, size_t* allocated_size)
{
    return msgpack_pool_malloc(&msgpack_pool_static_instance, required_size, allocated_size);
}

static inline void* msgpack_pool_static_realloc(
        void* ptr, size_t required_size, size_t* current_size)
{
    return msgpack_pool_realloc(&msgpack_pool_static_instance, ptr, required_size, current_size);
}

static inline void msgpack_pool_static_free(
        void* ptr, size_t size)
{
    return msgpack_pool_free(&msgpack_pool_static_instance, ptr, size);
}

static inline VALUE msgpack_pool_static_move_to_string(
        void* ptr, size_t offset, size_t size)
{
    return msgpack_pool_move_to_string(&msgpack_pool_static_instance, ptr, offset, size);
}

#endif

