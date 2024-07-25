#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    // this allows us to pass a variadic number of arguments to this function ...^
    va_list args;
    // add our format arg to the start
    va_start(args, format);
    // then print the arguments to standard error
    vfprintf(stderr, format, args);
    // then we end the va list
    va_end(args);
    fputs("\n", stderr);
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
}

void freeVM() {
    freeObjects();
}

void push(Value value) {
    // this line stores value in the array element at the top of the stack,
    // remember here that stacktop points past the last used element
    *vm.stackTop = value;
    // stackTop (which is a pointer) is then incremented using pointer arithmetic
    vm.stackTop++;
}

Value pop() {
    // opposite to pop, go back one step by decrementing the stack top
    vm.stackTop--;
    // then return that value (dereferenced)
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(OBJ_VAL(result));
}

static InterpretResult run() {
//Reads the byte currently pointed at by instruction pointer, then increments
#define READ_BYTE() (*vm.ip++)
// Reads the next byte from bytecode ^, uses that as an index, then looks up the value
// in the constants array
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// This is a creative use of the C pre processor, the outer while loop here is kind of
// strange but basically is a pattern that allows us to write multi line macro statements
// without any strange behaviour occuring
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("         ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    // This function takes an integer offset, so we need to do some pointer math to convert
    // ip back to its relative offset from the beginning of the bytecode
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                  concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                  double b = AS_NUMBER(pop());
                  double a = AS_NUMBER(pop());
                  push(NUMBER_VAL(a + b));
                } else {
                  runtimeError(
                      "Operands must be two numbers or two strings.");
                  return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                // we can now use the macros to check whether the value on top of the stack
                // is actually a number
                if (!IS_NUMBER(peek(0))) {
                    // if not then we cant perform the negation operation so report a runtime error
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            push(NUMBER_VAL(-AS_NUMBER(pop()))); break;
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

// this interprets the source code
InterpretResult interpret(const char* source) {
    // create a chunk and init it
    Chunk chunk;
    initChunk(&chunk);

    // if compile fails, free the chunk and return an error
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    // set the vm chunk to the created chunk
    vm.chunk = &chunk;
    // set instruction pointer to the first opcode
    vm.ip = vm.chunk->code;

    // interpret the result using the virtual machine
    InterpretResult result = run();

    // free the chunk
    freeChunk(&chunk);
    return result;
}
