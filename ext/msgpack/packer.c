/*
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2013 Sadayuki Furuhashi
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

#include "packer.h"

#ifdef RUBINIUS
static ID s_to_iter;
static ID s_next;
static ID s_key;
static ID s_value;
#endif

static ID s_call;

void msgpack_packer_static_init()
{
#ifdef RUBINIUS
    s_to_iter = rb_intern("to_iter");
    s_next = rb_intern("next");
    s_key = rb_intern("key");
    s_value = rb_intern("value");
#endif

    s_call = rb_intern("call");
}

void msgpack_packer_static_destroy()
{ }

void msgpack_packer_init(msgpack_packer_t* pk)
{
    memset(pk, 0, sizeof(msgpack_packer_t));

    msgpack_buffer_init(PACKER_BUFFER_(pk));
}

void msgpack_packer_destroy(msgpack_packer_t* pk)
{
    msgpack_buffer_destroy(PACKER_BUFFER_(pk));
}

void msgpack_packer_mark(msgpack_packer_t* pk)
{
    /* See MessagePack_Buffer_wrap */
    /* msgpack_buffer_mark(PACKER_BUFFER_(pk)); */
    rb_gc_mark(pk->buffer_ref);
}

void msgpack_packer_reset(msgpack_packer_t* pk)
{
    msgpack_buffer_clear(PACKER_BUFFER_(pk));

    pk->buffer_ref = Qnil;
}


void msgpack_packer_write_array_value(msgpack_packer_t* pk, VALUE v)
{
    /* actual return type of RARRAY_LEN is long */
    unsigned long len = RARRAY_LEN(v);
    if(len > 0xffffffffUL) {
        rb_raise(rb_eArgError, "size of array is too long to pack: %lu bytes should be <= %lu", len, 0xffffffffUL);
    }
    unsigned int len32 = (unsigned int)len;
    msgpack_packer_write_array_header(pk, len32);

    unsigned int i;
    for(i=0; i < len32; ++i) {
        VALUE e = rb_ary_entry(v, i);
        msgpack_packer_write_value(pk, e);
    }
}

static int write_hash_foreach(VALUE key, VALUE value, VALUE pk_value)
{
    if (key == Qundef) {
        return ST_CONTINUE;
    }
    msgpack_packer_t* pk = (msgpack_packer_t*) pk_value;
    msgpack_packer_write_value(pk, key);
    msgpack_packer_write_value(pk, value);
    return ST_CONTINUE;
}

void msgpack_packer_write_hash_value(msgpack_packer_t* pk, VALUE v)
{
    /* actual return type of RHASH_SIZE is long (if SIZEOF_LONG == SIZEOF_VOIDP
     * or long long (if SIZEOF_LONG_LONG == SIZEOF_VOIDP. See st.h. */
    unsigned long len = RHASH_SIZE(v);
    if(len > 0xffffffffUL) {
        rb_raise(rb_eArgError, "size of array is too long to pack: %ld bytes should be <= %lu", len, 0xffffffffUL);
    }
    unsigned int len32 = (unsigned int)len;
    msgpack_packer_write_map_header(pk, len32);

#ifdef RUBINIUS
    VALUE iter = rb_funcall(v, s_to_iter, 0);
    VALUE entry = Qnil;
    while(RTEST(entry = rb_funcall(iter, s_next, 1, entry))) {
        VALUE key = rb_funcall(entry, s_key, 0);
        VALUE val = rb_funcall(entry, s_value, 0);
        write_hash_foreach(key, val, (VALUE) pk);
    }
#else
    rb_hash_foreach(v, write_hash_foreach, (VALUE) pk);
#endif
}

static void _msgpack_packer_write_other_value(msgpack_packer_t* pk, VALUE v)
{
    int ext_type;
    VALUE proc = msgpack_packer_ext_registry_lookup(&pk->ext_registry,
            rb_obj_class(v), &ext_type);
    if(proc != Qnil) {
        VALUE payload = rb_funcall(proc, s_call, 1, v);
        StringValue(payload);
        msgpack_packer_write_ext(pk, ext_type, payload);
    } else {
        rb_funcall(v, pk->to_msgpack_method, 1, pk->to_msgpack_arg);
    }
}

void msgpack_packer_write_value(msgpack_packer_t* pk, VALUE v)
{
    switch(rb_type(v)) {
    case T_NIL:
        msgpack_packer_write_nil(pk);
        break;
    case T_TRUE:
        msgpack_packer_write_true(pk);
        break;
    case T_FALSE:
        msgpack_packer_write_false(pk);
        break;
    case T_FIXNUM:
        msgpack_packer_write_fixnum_value(pk, v);
        break;
    case T_SYMBOL:
        msgpack_packer_write_symbol_value(pk, v);
        break;
    case T_STRING:
        msgpack_packer_write_string_value(pk, v);
        break;
    case T_ARRAY:
        msgpack_packer_write_array_value(pk, v);
        break;
    case T_HASH:
        msgpack_packer_write_hash_value(pk, v);
        break;
    case T_BIGNUM:
        msgpack_packer_write_bignum_value(pk, v);
        break;
    case T_FLOAT:
        msgpack_packer_write_float_value(pk, v);
        break;
    default:
        _msgpack_packer_write_other_value(pk, v);
    }
}

#define ISJSONWS(c) (c==' '||c=='\r'||c=='\n'||c=='\t')
#define SKIPWS(p, pend) do{ if(!ISJSONWS(*p))break; }while(++p<pend)
#define GETC(c, p, pend) c = p == pend ? -1 : *p++
#define SCANC(c, p, pend) do{ SKIPWS(p, pend); GETC(c,p,pend); }while(0)
#define UNGETC(p) p--
#define X -1
static char hextable[256] = {
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    0,1,2,3,4,5,6,7,8,9,X,X,X,X,X,X,
    X,0xA,0xB,0xC,0xD,0xE,0xF,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,0xa,0xb,0xc,0xd,0xe,0xf,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
};
#define tohex(c) ((unsigned int)hextable[(unsigned char)(c)])
#define SCANSTR(pat, misc) do{ \
    if(p + strlen(pat) <= pend && memcmp(pat, p, strlen(pat)) == 0) { \
        p += strlen(pat); \
    } else { \
        err = MPERR_UNEXPECTED_TOKEN; \
        misc; \
        goto fail; \
    } \
} while (0)
#define mpseq_write(seq, type, value) do{*(type*)(seq->ptr+seq->idx)=value;seq->idx+=sizeof(type);}while(0)

struct mpseq {
    uint64_t capa;
    uint64_t idx;
    uint32_t stack_capacity;
    uint32_t stack_depth;
    uint64_t *stack_ptr;
    char *ptr;
};
static void
mpseq_init(struct mpseq *seq)
{
    seq->capa = 256;
    seq->idx = 0;
    seq->stack_capacity = 256;
    seq->stack_depth = 0;
    seq->stack_ptr = ALLOC_N(uint64_t, seq->stack_capacity);
    seq->ptr = ALLOC_N(char, seq->capa);
}
static void
mpseq_free(struct mpseq *seq)
{
    xfree(seq->stack_ptr);
    xfree(seq->ptr);
}
static void
mpseq_stack_push(struct mpseq *seq)
{
    seq->stack_depth++;
    if (seq->stack_depth >= seq->stack_capacity) {
        seq->stack_capacity *= 2;
        REALLOC_N(seq->stack_ptr, uint64_t, seq->stack_capacity);
    }
    seq->stack_ptr[seq->stack_depth] = seq->idx;
}
static void
mpseq_stack_pop(struct mpseq *seq)
{
    seq->stack_depth--;
}
static char *
mpseq_stack_peek(struct mpseq *seq)
{
    return seq->ptr + seq->stack_ptr[seq->stack_depth];
}
static bool
mpseq_stack_empty(struct mpseq *seq)
{
    return seq->stack_depth == 0;
}
static void
mpseq_increment(struct mpseq *seq)
{
    if (!mpseq_stack_empty(seq)) {
        char *p = mpseq_stack_peek(seq) + 1;
        uint32_t *sz = (uint32_t *)p;
        *sz += 1;
    }
}
static void
mpseq_ensure_writable(struct mpseq *seq, uint64_t len)
{
    if (seq->idx + len >= seq->capa) {
        seq->capa *= 2;
        REALLOC_N(seq->ptr, char, seq->capa);
    }
}
static bool
mpseq_root_p(struct mpseq *seq)
{
    return mpseq_stack_empty(seq);
}
static bool
mpseq_array_p(struct mpseq *seq)
{
    return (unsigned char)mpseq_stack_peek(seq)[0] == 0xdd;
}
static void
mpseq_array_open(struct mpseq *seq)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 5);
    mpseq_stack_push(seq);
    seq->ptr[seq->idx] = 0xdd;
    memset(seq->ptr+seq->idx+1, 0, 4);
    seq->idx += 5;
}
static int
mpseq_array_close(struct mpseq *seq)
{
    mpseq_stack_pop(seq);
    return 5;
}
static bool
mpseq_map_p(struct mpseq *seq)
{
    return (unsigned char)mpseq_stack_peek(seq)[0] == 0xdf;
}
static void
mpseq_map_open(struct mpseq *seq)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 5);
    mpseq_stack_push(seq);
    seq->ptr[seq->idx] = 0xdf;
    memset(seq->ptr+seq->idx+1, 0, 4);
    seq->idx += 5;
}
static int
mpseq_map_close(struct mpseq *seq)
{
    mpseq_stack_pop(seq);
    return 5;
}
static void
mpseq_add_nil(struct mpseq *seq)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 1);
    seq->ptr[seq->idx++] = 0xc0;
}
static void
mpseq_add_false(struct mpseq *seq)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 1);
    seq->ptr[seq->idx++] = 0xc2;
}
static void
mpseq_add_true(struct mpseq *seq)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 1);
    seq->ptr[seq->idx++] = 0xc3;
}
static void
mpseq_add_key(struct mpseq *seq, uint32_t count, const char *p)
{
    mpseq_ensure_writable(seq, 5+sizeof(char *));
    seq->ptr[seq->idx++] = 0xdb;
    mpseq_write(seq, uint32_t, count);
    mpseq_write(seq, const char *, p);
}
static void
mpseq_add_str(struct mpseq *seq, uint32_t count, const char *p)
{
    mpseq_increment(seq);
    mpseq_add_key(seq, count, p);
}
static int
mpseq_add_uint64(struct mpseq *seq, uint64_t v, int negative_p)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 9);
    if (negative_p) {
        if (v > 9223372036854775808ULL) { /* -INT64_MIN */
            return -1;
        }
        seq->ptr[seq->idx++] = 0xd3;
        mpseq_write(seq, int64_t, -v);
        return 0;
    }
    else {
        seq->ptr[seq->idx++] = 0xcf;
        mpseq_write(seq, uint64_t, v);
        return 0;
    }
}
static void
mpseq_add_double(struct mpseq *seq, double d)
{
    mpseq_increment(seq);
    mpseq_ensure_writable(seq, 9);
    seq->ptr[seq->idx++] = 0xcb;
    mpseq_write(seq, double, d);
}

enum {
    MPERR_INCOMPLETE_ESCAPE=1,
    MPERR_C0_IN_STRING,
    MPERR_INVALID_ESCAPE,
    MPERR_INCOMPLETE_UNICODE_ESCAPE,
    MPERR_INVALID_UNICODE_ESCAPE,
    MPERR_INVALID_LOW_SURROGATE_UNICODE_ESCAPE,
    MPERR_TOO_LARGE_STRING,
    MPERR_UNUSED_TOKENS,
    MPERR_UNEXPECTED_TOKEN,
};
#define mperr(code) do{err=code; goto fail;}while(0)
static int64_t
count_string(const char **pp, const char *pend)
{
    int64_t count = 0;
    int err = 0;
    const char *p = *pp;
    unsigned int v;

    while (p < pend) {
        int c = (unsigned char)*p++;
        if (c < 0x20) {
            mperr(MPERR_C0_IN_STRING);
        }
        switch (c) {
          case '"':
            if (count > UINT32_MAX) mperr(MPERR_TOO_LARGE_STRING);
            *pp = p;
            return count;
          case '\\':
            if (p >= pend) {
                p--;
                mperr(MPERR_INCOMPLETE_ESCAPE);
            }
            switch (c = *p++) {
              case '"': case '\\': case '/':
              case 'b': case 'f': case 'n': case 'r': case 't':
                count++;
                break;
              case 'u':
                if (p + 4 >= pend) {
                    p -= 2;
                    mperr(MPERR_INCOMPLETE_UNICODE_ESCAPE);
                }
                v  = (tohex(*p++) << 12);
                v |= (tohex(*p++) <<  8);
                v |= (tohex(*p++) <<  4);
                v |=  tohex(*p++);
bmp:
                if (v > 0xFFFF) {
                    p -= 6;
                    mperr(MPERR_INVALID_UNICODE_ESCAPE);
                }
                else if (v <= 0x7F) {
                    count++;
                }
                else if (v <= 0x7FF) {
                    count += 2;
                }
                else if (v < 0xD800 || 0xDC00 <= v) {
                    /* 0xDC00..0xDC00 is invalid but encode as UTF-8 */
                    count += 3;
                }
                else {
                    /* high surrogate: valid */
high_surroagate:
                    if (p+6 >= pend || p[0] != '\\' || p[1] != 'u') {
                        count += 3;
                        continue;
                    }
                    p += 2;
                    v  = (tohex(*p++) << 12);
                    v |= (tohex(*p++) <<  8);
                    v |= (tohex(*p++) <<  4);
                    v |=  tohex(*p++);

                    if (0xFFFF < v) {
                        mperr(MPERR_INVALID_LOW_SURROGATE_UNICODE_ESCAPE);
                    }
                    else if (v < 0xD800 || 0xDFFF < v) {
                        /* lonely high surrogate + valid BMP */
                        count += 3;
                        goto bmp;
                    }
                    else if (v < 0xDC00) {
                        /* high surrogate again */
                        count += 3;
                        goto high_surroagate;
                    }
                    else {
                        /* valid surrogate pair is 4 byte UTF-8 */
                        count += 4;
                    }
                }
                break;
              default:
                p -= 2;
                mperr(MPERR_INVALID_ESCAPE);
            }
            break;
          default:
            count++;
            break;
        }
    }
fail:
    *pp = p;
    return -err;
}
static void
msgpack_buffer_write_utf8_3(msgpack_buffer_t *b, unsigned int v)
{
    char buf[3];
    buf[0] = 0xE0 |  (v >> 12);
    buf[1] = 0x80 | ((v >>  6) & 0x3F);
    buf[2] = 0x80 | ((v      ) & 0x3F);
    msgpack_buffer_append(b, buf, 3);
}
static void
msgpack_buffer_append_json_string(msgpack_buffer_t* b, const char* ptr, int32_t len)
{
    const unsigned char *p = (const unsigned char *)ptr;
    unsigned int v, v1;
    unsigned char c;

    msgpack_buffer_ensure_writable(b, len);
    while ((c = *p++) != '"') {
        if (c != '\\') {
            msgpack_buffer_write_1(b, c);
            continue;
        }

        switch (c = *p++) {
          case '"': case '\\': case '/':
            msgpack_buffer_write_1(b, c);
            break;
          case 'b':
            msgpack_buffer_write_1(b, '\b');
            break;
          case 'f':
            msgpack_buffer_write_1(b, '\f');
            break;
          case 'n':
            msgpack_buffer_write_1(b, '\n');
            break;
          case 'r':
            msgpack_buffer_write_1(b, '\r');
            break;
          case 't':
            msgpack_buffer_write_1(b, '\t');
            break;
          case 'u':
            v  = (tohex(*p++) << 12);
            v |= (tohex(*p++) <<  8);
            v |= (tohex(*p++) <<  4);
            v |=  tohex(*p++);
bmp:
            if (v <= 0x7F) {
                msgpack_buffer_write_1(b, c);
            }
            else if (v <= 0x7FF) {
                msgpack_buffer_write_2(b, 0xc0|(v>>6), 0x80|(v&0x3F));
            }
            else if (v < 0xD800 || 0xDC00 <= v) {
                msgpack_buffer_write_utf8_3(b, v);
            }
            else {
high_surroagate:
                if (p[0] != '\\' || p[1] != 'u') {
                    msgpack_buffer_write_utf8_3(b, v);
                    continue;
                }
                p += 2;
                v1 = v;
                v  = (tohex(*p++) << 12);
                v |= (tohex(*p++) <<  8);
                v |= (tohex(*p++) <<  4);
                v |=  tohex(*p++);

                if (v < 0xD800 || 0xDFFF < v) {
                    /* lonely high surrogate + valid BMP */
                    msgpack_buffer_write_utf8_3(b, v1);
                    goto bmp;
                }
                else if (v < 0xDC00) {
                    /* high surrogate again */
                    msgpack_buffer_write_utf8_3(b, v1);
                    goto high_surroagate;
                }
                else {
                    /* valid surrogate pair is 4 byte UTF-8 */
                    char buf[4];
                    v = 0x10000 + ((v1 & 0x3FF) << 10) + (v & 0x3FF);
                    buf[0] = 0xF0 |  (v >> 18);
                    buf[1] = 0x80 | ((v >> 12) & 0x3F);
                    buf[2] = 0x80 | ((v >>  6) & 0x3F);
                    buf[3] = 0x80 | ((v      ) & 0x3F);
                    msgpack_buffer_append(b, buf, 4);
                }
            }
            break;
          default:
            UNREACHABLE;
        }
    }
}

void msgpack_packer_write_ir(msgpack_packer_t* pk, struct mpseq *seq)
{
    const char *p = seq->ptr;
    const char *pend = seq->ptr + seq->idx;
    uint32_t len;

    while (p < pend) {
        unsigned char head = (unsigned char)*p++;
        switch (head) {
          case 0xc0:
            msgpack_packer_write_nil(pk);
            break;
          case 0xc2:
            msgpack_packer_write_false(pk);
            break;
          case 0xc3:
            msgpack_packer_write_true(pk);
            break;
          case 0xcb:
            msgpack_packer_write_double(pk, *(double *)p);
            p += sizeof(double);
            break;
          case 0xcf:
            msgpack_packer_write_u64(pk, *(uint64_t *)p);
            p += sizeof(uint64_t);
            break;
          case 0xd3:
            msgpack_packer_write_long_long(pk, *(int64_t *)p);
            p += sizeof(int64_t);
            break;
          case 0xdb:
            len = *(uint32_t*)p;
            msgpack_packer_write_raw_header(pk, len);
            p += sizeof(int32_t);
            msgpack_buffer_append_json_string(PACKER_BUFFER_(pk), *(char **)p, len);
            p += sizeof(char *);
            break;
          case 0xdd:
            msgpack_packer_write_array_header(pk, *(int32_t *)p);
            p += sizeof(int32_t);
            break;
          case 0xdf:
            msgpack_packer_write_map_header(pk, *(int32_t *)p);
            p += sizeof(int32_t);
            break;
          default:
            UNREACHABLE;
        }
    }
}

int pack_from_json(msgpack_packer_t* pk, const char *p, const char *pend)
{
    struct mpseq mpseq = {};
    struct mpseq *seq = &mpseq;
    int err = 0;
    const char *p0 = p;
    mpseq_init(seq);

    while (p < pend) {
        int c;
        uint64_t v;
        const char *q;
        bool float_p = false;
        SCANC(c, p, pend);
        switch (c) {
          case '{':
            mpseq_map_open(seq);
            SCANC(c, p, pend);
            if (c == '}') {
                mpseq_map_close(seq);
                goto terminator;
            }
            goto map_key;
          case '[':
            mpseq_array_open(seq);
            SCANC(c, p, pend);
            if (c == ']') {
                mpseq_array_close(seq);
                goto terminator;
            }
            UNGETC(p);
            continue;
          case '"':
            q = p;
            {
                int64_t r = count_string(&p, pend);
                if (r < 0) mperr(-r);
                mpseq_add_str(seq, (uint32_t)r, q);
            }
            break;
          case 't':
            SCANSTR("rue", p--);
            mpseq_add_true(seq);
            break;
          case 'f':
            SCANSTR("alse", p--);
            mpseq_add_false(seq);
            break;
          case 'n':
            SCANSTR("ull", p--);
            mpseq_add_nil(seq);
            break;
          case '0':
            q = p - 1;
            v = 0;
            GETC(c, p, pend);
            goto subnumber;
          case '-':
            q = p - 1;
            GETC(c, p, pend);
            if (c == '0') {
                v = 0;
                GETC(c, p, pend);
                goto subnumber;
            }
            else if ('1' <= c && c <= '9') {
                v = c & 0xF;
                goto number;
            }
            else {
invalid_number:
                p = q;
                mperr(MPERR_UNEXPECTED_TOKEN);
            }
          case '1': case '2': case '3': case '4': case '5':
          case '6': case '7': case '8': case '9':
            q = p - 1;
            v = c & 0xF;
number:
            GETC(c, p, pend);
#define UINT64_DIV_10 UINT64_C(1844674407370955161)
            while (ISDIGIT(c)) {
                if (v > UINT64_DIV_10 || (v == UINT64_DIV_10 && c > '5')) {
                    float_p = true;
                    do {
                        GETC(c, p, pend);
                    } while (ISDIGIT(c));
                    break;
                }
                v *= 10;
                v += c & 0xF;
                GETC(c, p, pend);
            }
subnumber:
            if (c == '.') {
                float_p = true;
                GETC(c, p, pend);
                if (!ISDIGIT(c)) goto invalid_number;
                do {
                    GETC(c, p, pend);
                } while (ISDIGIT(c));
            }
            if (c == 'e' || c == 'E') {
                float_p = true;
                GETC(c, p, pend);
                if (c == '+' || c == '-') {
                    GETC(c, p, pend);
                }
                if (!ISDIGIT(c)) goto invalid_number;
                do {
                    GETC(c, p, pend);
                } while (ISDIGIT(c));
            }

            if (float_p) {
                double d;
                int len;
                VALUE work;
                char *s;
must_double:
                len = p - q;
                s = ALLOCV(work, len+1);
                MEMCPY(s, q, char, len);
                s[len] = '\0';
                d = rb_cstr_to_dbl(s, false);
                mpseq_add_double(seq, d);
                if (work) ALLOCV_END(work);
            }
            else {
                if (mpseq_add_uint64(seq, v, *q == '-') < 0)
                    goto must_double;
            }
            if (c != -1) UNGETC(p);
            break;
          default:
            mperr(MPERR_UNEXPECTED_TOKEN);
        }

terminator:
        if (mpseq_root_p(seq)) {
            goto finish;
        }

        SCANC(c, p, pend);
        if (mpseq_map_p(seq)) {
            if (c == ',') {
                const char *q;
                int64_t r;
                SCANC(c, p, pend);
map_key:
                if (c != '"') mperr(MPERR_UNEXPECTED_TOKEN);
                q = p;
                r = count_string(&p, pend);
                if (r < 0) mperr(-r);
                mpseq_add_key(seq, (uint32_t)r, q);
                SCANC(c, p, pend);
                if (c != ':') mperr(MPERR_UNEXPECTED_TOKEN);
                continue;
            }
            else if (c == '}') {
                mpseq_map_close(seq);
                goto terminator;
            }
        }
        else if (mpseq_array_p(seq)) {
            if (c == ',')
                continue;
            else if (c == ']') {
                mpseq_array_close(seq);
                goto terminator;
            }
        }
        mperr(MPERR_UNEXPECTED_TOKEN);;
    }
finish:
    SKIPWS(p, pend);
    if (p != pend) {
        err = MPERR_UNUSED_TOKENS;
    }
fail:
    if (!err) {
        msgpack_packer_write_ir(pk, seq);
    }
    mpseq_free(seq);
    if (err) {
        rb_raise(rb_eArgError, "unexpected token at %ld byte", p-p0);
    }
    return err;
}
