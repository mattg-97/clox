#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
    // our repl here has a hard coded line length limit, this is not the norm
    // but is okay for our purposes
    char line[1024];
    for (;;) {
        // print a helper character at the start of each repl line
        printf("> ");
        // get stdin and load into char array
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        // interpret that line
        interpret(line);
    }
}

static char* readFile(const char* path) {
    // open the filej
    FILE* file = fopen(path, "rb");
    // print a logical error response if unable to open the file
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    // seek basically allows us to walk through the entire file, by passing
    // in SEEK END we go to the end of the file
    fseek(file, 0L, SEEK_END);
    // ftell then tells us how many bytes we are from the start of the file
    size_t fileSize = ftell(file);
    // we then rewind back to the beginning of the file
    rewind(file);

    // then we allocate a char buffer of that size
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        // will only happen if OS doesnt have enough memory to read file in
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    // then we read the whole file in a single batch
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}


static void runFile(const char* path) {
    // read in the file
    char* source = readFile(path);
    // interpret the source code
    InterpretResult result = interpret(source);
    // then free the char array (we need to do this because readfile dynamically
    // allocates the memory and then passes ownership to this function)
    free(source);

    // exit codes differ for each error
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    initVM();
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }
    freeVM();
    return 0;
}

/*Chunk chunk;
   initChunk(&chunk);

   // we add the const value itself to the constant pool, which returns the index
   int constant = addConstant(&chunk, 1.2);
   // then we write the instruction and its opcode
   writeChunk(&chunk, OP_CONSTANT, 123);
   // then we write the operand next to it, see chunk.h for a diagram
   writeChunk(&chunk, constant, 123);

   constant = addConstant(&chunk, 3.4);
   writeChunk(&chunk, OP_CONSTANT, 123);
   writeChunk(&chunk, constant, 123);

   writeChunk(&chunk, OP_ADD, 123);

   constant = addConstant(&chunk, 5.6);
   writeChunk(&chunk, OP_CONSTANT, 123);
   writeChunk(&chunk, constant, 123);

   writeChunk(&chunk, OP_DIVIDE, 123);

   writeChunk(&chunk, OP_NEGATE, 123);

   writeChunk(&chunk, OP_RETURN, 123);*/

   // 1 * 2 = 3;
   /*Chunk chunkTwo;
   initChunk(&chunkTwo);
   constant = addConstant(&chunkTwo, 1.0);
   writeChunk(&chunkTwo, OP_CONSTANT, 1);
   writeChunk(&chunkTwo, constant, 1);
   constant = addConstant(&chunkTwo, 2.0);
   writeChunk(&chunkTwo, OP_CONSTANT, 1);
   writeChunk(&chunkTwo, constant, 1);
   writeChunk(&chunkTwo, OP_MULTIPLY, 1);
   constant = addConstant(&chunkTwo, 3.0);
   writeChunk(&chunkTwo, OP_CONSTANT, 1);
   writeChunk(&chunkTwo, constant, 1);
   writeChunk(&chunkTwo, OP_ADD, 1);
   writeChunk(&chunkTwo, OP_RETURN, 123);
   disassembleChunk(&chunkTwo, "chunk two");
   interpret(&chunkTwo);*/
   //disassembleChunk(&chunk, "test chunk");
   //interpret(&chunk);
