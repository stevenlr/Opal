#pragma once

#include <stdint.h>

void * xmalloc(uint64_t size);
void * xremalloc(uint64_t size);

void * _sb_grow_impl(void * b, int32_t increment, uint64_t value_size);

#define _sb_raw(b) ((int32_t *)(b) - 2)
#define _sb_raw_len(b) _sb_raw(b)[0]
#define _sb_raw_cap(b) _sb_raw(b)[1]
#define _sb_need_grow(b, n) ((b) == 0 || _sb_raw_len(b) + (n) >= _sb_raw_cap(b))
#define _sb_grow(b, n) (*((void **)&(b)) = _sb_grow_impl((b), (n), sizeof(*(b))))
#define _sb_maybe_grow(b, n) (_sb_need_grow(b, n) ? _sb_grow(b, n) : 0)

#define sb_len(b) ((b) ? _sb_raw_len(b) : 0)
#define sb_end(b) ((b) ? (b) + _sb_raw_len(b) : 0)
#define sb_push(b, ...) (_sb_maybe_grow(b, 1), (b)[_sb_raw_len(b)++] = (__VA_ARGS__))
#define sb_free(b) ((b) ? free(_sb_raw(b)), (b) = NULL : 0)

const char * intern_string(const char * str);

void test_common();

