#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

// We define this because most virtual machines have a constant pool at the
// data region, rather then immediately after the opcodes, each chunk will 
// carry with is a list of the values that appear in the program.
//
// This array is dynamic so needs similar methods to the chunks, see chunk.c 
// if the implementation is confusing
typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
