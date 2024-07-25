#ifndef clox_memory_h
#define clox_memory_h

#include <common.h>
#include "object.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// this macro calculates a new capacity based on the current capaciuty
// in order to get the performance we want, it must be scaled based on the
// old size, in this case we are growing by a factor of 2, although 1.5x is
// also used commonly. If the capacity is 0 then we just jump straight to 8
// using amortized analysis we can grow the array in O(1) time when taken over
// a sequence, even though it seems like its O(n)
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// Once we know the desired capacity we can frow the array to that size
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

// This essentially wraps reallocate and passes 0 into the newSize parameter
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void freeObjects();

#endif
