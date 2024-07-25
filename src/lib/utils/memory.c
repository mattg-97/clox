#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

// Although this returns a void* we are using the GROW_ARRAY macro to cast it back to a
// chosen pointer type.
//
// This function allows us to do all of our memory allocations in one place, here is what
// happens for each case:
// oldSize      |newSize        |Operation
// 0            |Non-Zero       |Allocate a new block (malloc)
// Non-zero     |0              |Free allocation
// Non-zero     |< OldSize      |Shrink existing allocation
// Non-zero     |> OldSize      |Grow existing allocation
//
// The reason we want to do this in one spot will become evident when we implement a GC
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    // if newSize is 0 then free the memory
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    // realloc conveniently 'becomes' malloc if allocating for the first time
    void* result = realloc(pointer, newSize);
    // for the rare case we run out of memory...
    if (result == NULL) exit(1);
    return result;
}

static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}
