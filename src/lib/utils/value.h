#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj; // this acts almost like the 'base class' for all objects (if C supported classes)
typedef struct ObjString ObjString;

// this is the enum for the different types of values that can be stored in
// the value union, this is used to determine what type of value is stored
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    // this is a tagged union, the type of the value will determine which
    // field is used, this is a common pattern in C for storing multiple
    // types in a single struct
    union {
        bool boolean;
        double number;
        Obj* obj; //objects are always stored on the heap
    } as; // as is a common name for the union in C so we can do as.number etc
} Value;

// these macros are for checking if the type of a value is a certain type
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

// each one of these macros takes a c value of the appropriate type and
// returns a value of the appropriate type, this is a common pattern in C
#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

// these macros are for creating values of the appropriate type
#define BOOL_VAL(value)    ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL            ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)  ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)    ((Value){VAL_OBJ, {.obj = (Obj*)object}})


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

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
