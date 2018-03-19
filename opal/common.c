#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

void * xmalloc(uint64_t size)
{
    void * ptr = malloc(size);
    assert(ptr);
    return ptr;
}
