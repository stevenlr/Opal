#include "common.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"

void * xmalloc(uint64_t size)
{
    void * ptr = malloc(size);
    assert(ptr);
    return ptr;
}

void * xrealloc(void * ptr, uint64_t size)
{
    ptr = realloc(ptr, size);
    assert(ptr);
    return ptr;
}

void * _sb_grow_impl(void * b, int32_t increment, uint64_t value_size)
{
    int32_t old_size = (b) ? _sb_raw_cap(b) : 0;
    int32_t min_needed = old_size + increment;
    int32_t new_size = old_size * 2 + 1;

    if (new_size < min_needed)
    {
        new_size = min_needed;
    }

    int32_t * new_raw = xrealloc(b ? _sb_raw(b) : NULL, (uint64_t)new_size * value_size + sizeof(int32_t) * 2);
    if (!b)
    {
        new_raw[0] = 0;
    }

    new_raw[1] = new_size;
    return new_raw + 2;
}

void test_dyn_buf(void)
{
    int64_t * array = NULL;

    assert(sb_len(array) == 0);
    sb_push(array, 45);
    sb_push(array, 12);
    sb_push(array, 49);
    assert(sb_len(array) == 3);
    assert(array[0] == 45);
    assert(array[1] == 12);
    assert(array[2] == 49);
    sb_free(array);
    assert(array == NULL);
    assert(sb_len(array) == 0);
}

typedef struct intern_string_t
{
    uint64_t length;
    const char * str;
} intern_string_t; 

sb_t(intern_string_t) intern_string_list = NULL;

// @Todo Use some kind of memory arena
const char * intern_string_range(const char * first, const char * last)
{
    uint64_t length = last - first + 1;

    for (intern_string_t * it = intern_string_list;
            it != sb_end(intern_string_list);
            ++it)
    {
        if (it->length == length && strncmp(it->str, first, length) == 0)
        {
            return it->str;
        }
    }

    char * new_str = xmalloc(length + 1);
    memcpy(new_str, first, length);
    new_str[length] = '\0';
    sb_push(intern_string_list, (intern_string_t){ length, new_str });
    return new_str;
}

const char * intern_string(const char * str)
{
    return intern_string_range(str, str + strlen(str) - 1);
}

void test_intern_string(void)
{
    char a[] = "my first string";
    char b[] = "my first";
    char c[] = "my first string omg";
    assert(strcmp(a, intern_string(a)) == 0);
    assert(intern_string(a) == intern_string("my first string"));
    assert(intern_string(intern_string(a)) == intern_string(a));
    assert(intern_string(a) != intern_string(b));
    assert(intern_string(a) != intern_string(c));
}

void test_common(void)
{
    test_dyn_buf();
    test_intern_string();
}

