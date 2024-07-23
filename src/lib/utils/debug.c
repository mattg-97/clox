#include <stdio.h>

#include "debug.h"
#include "value.h"

// This is useful for the programmer to see how we are representing code in our chunks
void disassembleChunk(Chunk* chunk, const char* name) {
    // So we have recogniseable name for our chunks
    printf("== %s ==\n", name);

    // we then go through the bytecode and dissassemble each instrucion, we dont increment
    // offset here, we actually let the disassembleInstrucion function do this. Basically
    // when we call that function it returns the offset of the next instruction. This is 
    // because instructions have different sizes
    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    // this gets the constant index from the subsequent byte in the chunk
    uint8_t constant = chunk->code[offset + 1];
    // we then print it out
    printf("%-16s %4d '", name, constant);
    // we then print the actual value stored at that constant index
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    // disassembleInstruction returns offset + 1 (the beginning of the next instruction)
    // constant returns offset + 2 (1 for opcode, 1 for constant) so we can do the same
    return offset + 2;
}

static int constantLongInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 3];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    // as constant long is a 3 byte opcode
    return offset + 4;
}


static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    // this basically checks if the source code line is the same as the previous one
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    // This gets a single byte from the bytecode at the given offset, which we
    // then switch on. 
    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
        return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        // On the off chance theres a compiler bug, we print that too
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
