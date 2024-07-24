#include <stdio.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {
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

static InterpretResult run() {
//Reads the byte currently pointed at by instruction pointer, then increments
#define READ_BYTE() (*vm.ip++)
// Reads the next byte from bytecode ^, uses that as an index, then looks up the value
// in the constants array
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// This is a creative use of the C pre processor, the outer while loop here is kind of
// strange but basically is a pattern that allows us to write multi line macro statements
// without any strange behaviour occuring
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
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
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;
            case OP_NEGATE: push(-pop()); break;
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
