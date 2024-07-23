#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    // we need to check that the array has enough capacity for a new byte
    if (chunk->capacity < chunk->count + 1) {
        // if it doesnt then we should grow the array to make room
        // get the old capacity
        int oldCapacity = chunk->capacity;
        // grow the capacity using the macro in memory.h
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        // then we use the grow array macro to grow the opcodes array to the new capacity
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        // same with the lines array
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }
    // write byte param to the opcode at the current array position
    chunk->code[chunk->count] = byte;
    // write line param to the line at the current array position
    chunk->lines[chunk->count] = line;
    // increment the count
    chunk->count++;
}

// Uses our memory macros to free the opcode and line arrays
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    // We then call initChunk to leave it in a well-defined, empty state
    initChunk(chunk);
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void writeConstant(Chunk* chunk, Value value, int line) {
    int constantIndex = addConstant(chunk, value);

    if (chunk->capacity < chunk->count + 4) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }
    // write byte param to the opcode at the current array position
    chunk->code[chunk->count] = OP_CONSTANT_LONG;
    chunk->lines[chunk->count] = line;
    // increment the count
    chunk->count++;

    chunk->code[chunk->count] = (constantIndex >> 16) & 0xFF;
    chunk->lines[chunk->count] = line;
    chunk->count++;

    chunk->code[chunk->count] = (constantIndex >> 8) & 0xFF;
    chunk->lines[chunk->count] = line;
    chunk->count++;

    chunk->code[chunk->count] = constantIndex & 0xFF;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}