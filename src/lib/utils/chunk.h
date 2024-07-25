#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// These are the opcodes for clox, they are all 1 byte, we can then store indexes next to them
// for example
// OP_RETURN                OP_CONSTANT
// [01] <- opcode   opcode->[00][21]<-constant index
// (1 byte)                 (2 bytes)
// each opcodes determines how many operand bytes it has, and also what they mean.
typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_CONSTANT_LONG,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,
} OpCode;

typedef struct {
    // the number of elements in the array that are actually in use
    int count;
    // the number of elements in the array we have allocated
    int capacity;
    // we dont know how big the byte array is before we compile a chunk
    // therefore a pointer is needed, which we will then use to create
    // a dynamic array
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
void writeConstant(Chunk* chunk, Value value, int line);

#endif
