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
#include "rb_ext.h"

#ifndef DISABLE_STR_NEW_MOVE
VALUE rb_str_new_move(char* data, size_t length)
{
/* MRI 1.9 */
#ifdef RUBY_VM
    /* note: this code is magical: */
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

/* MRI 1.8 */
#else
    NEWOBJ(str, struct RString);
    OBJSETUP(str, rb_cString, T_STRING);

    str->ptr = data;
    str->len = length;
    str->aux.capa = length;

    /* this is safe. see msgpack_postmem_alloc() */
    data[length] = '\0';

    return (VALUE) str;
#endif
}
#endif

