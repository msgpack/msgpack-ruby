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

void msgpack_pool_static_init();

void msgpack_pool_init();

void msgpack_pool_destroy();

//TODO msgpack_pool_realloc(void* ptr, size_t& allocated_size, size_t required_size);
void* msgpack_pool_malloc(size_t aligned_size);

//TODO msgpack_pool_realloc(void* ptr, size_t& current_size, size_t required_size);
void* msgpack_pool_realloc(void* ptr, size_t aligned_size);

void msgpack_pool_free(void* ptr);

#endif

