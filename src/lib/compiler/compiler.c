#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

// all of lox's precedence levels in order of lowest to highest, C
// implictly numbers them in ascending order when instantiated in an enum
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

// a simple typedef for a function type that takes no args and returns nothing
typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

// forward declarations because why not
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static Chunk* currentChunk();
static void errorAt(Token* token, const char* message);
static void error(const char* message);
static void errorAtCurrent(const char* message);
static void advance();
static void consume(TokenType type, const char* message);
static void emitByte(uint8_t byte);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitReturn();
static void endCompiler();
static ParseRule* getRule(TokenType type);
static void binary();
static void expression();
static void grouping();
static uint8_t makeConstant(Value value);
static void emitConstant(Value value);
static void number();
static void unary();

static Chunk* currentChunk() {
    return compilingChunk;
}

// this error function is the workhorse of the error handling
static void errorAt(Token* token, const char* message) {
    // INFO: the reason we set this is because if panic mode is already set,
    // we wont print out any more errors. Otherwise we could have a huge list of
    // errors from the rest of the source code but not be able to see where the
    // panic mode occured
    if (parser.panicMode) return;
    // set the parser to panic mode
    parser.panicMode = true;
    // print the line the error occured on
    fprintf(stderr, "[line %d] Error", token->line);

    // check if the type is EOF
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        // if the lexeme is human readable we show it
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    // print the error message
    fprintf(stderr, ": %s\n", message);
    // set the hadError flag to true to indicate a parsing error
    parser.hadError = true;
}

// more commonly the error occurs at the token we just consumed, so this
// function handles that case
static void error(const char* message) {
  errorAt(&parser.previous, message);
}

// if the scanner returns an error then we should actually tell the user
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

// this funciton steps though the token stream
static void advance() {
    // we store the current token for later use
    parser.previous = parser.current;

    for (;;) {
        // uses the scanner to get the current token
        parser.current = scanToken();
        // if its an error then break out of the loop
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

// similar to advance in that it reads the next token, but it also validates that
// the token has the correct type
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    // if the type isnt correct, return an error
    errorAtCurrent(message);
}

// we start by appending a byte to a chunk, it may be an opcode or an operand,
// and sends the previous tokens line information so any errors are associated
// with that line
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// just makes our life easier by allowing us to emit a opcode and an operand
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

// to wrap things up and print a single expression at this stage we add a return
// opcode to the final byte in the chunk
static void endCompiler() {
    emitReturn();
}


static void parsePrecedence(Precedence precedence) {

}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return; //unreachable

    }
}

static void expression() {
    // say we had '-a.b + c' as our expression
    // when we call this function, it will parse the entire expression because
    // + has a higher precendence than assignment. If we were to call it with
    // PREC_UNARY then it would only compile '-a.b' and stop there because addition
    // has lower precedence than unary operators
    parsePrecedence(PREC_ASSIGNMENT);
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// turns a value into a constant and adds it to the chunks constant array
static uint8_t makeConstant(Value value) {
    // add the value to the current chunks data region and return its index
    int constant = addConstant(currentChunk(), value);
    // check for error
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk");
        return 0;
    }

    // return that constant cast to a byte
    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void number() {
    // we assume that the number literal has been consumed and is stored in previous
    // then we use the c std library function to parse it to a double
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

static void unary() {
    // the leading '-' has been consumed and is sitting in the previous token
    TokenType operatorType = parser.previous.type;

    // compile the operand
    // we use PREC_UNARY precedence to permit nested unary expressions like !!doubleNegative
    parsePrecedence(PREC_UNARY);

    // it might seem strange to compile the operand and then emit the negation byte
    // however the order of execution is as follows:
    // 1. We evaluate the operand first, leaving its value on the stack
    // 2. Then we pop that value, negate it, and push the result back on the stack
    // that means the negate instruciton should be pushed last, this is part of
    // the compilers job, parsing the source code and rearranging it into the order
    // that execution happens in our vm

    // emit the operator instruction
    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; //unreachable
    }
}


bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    // set compiling chunk to chunk parameter
    compilingChunk = chunk;
    // set error flags to false
    parser.hadError = false;
    parser.panicMode = false;
    /*  TEMP CODE WHICH ALLOWED US TO DEBUG THE COMPILER BEFORE IMPLEMENTATION
    int line = -1;
    for (;;) {
       Token token = scanToken();
       // if token is on a different line
       if (token.line != line) {
         //print new line number and then set line to token line
         printf("%4d ", token.line);
         line = token.line;
       } else {
           // else just print the | character like we do in vm.run() debug mode
         printf("   | ");
       }
       printf("%2d '%.*s'\n", token.type, token.length, token.start);

       if (token.type == TOKEN_EOF) break;
       }*/
    // primes the scanner
    advance();
    // parse a single expression
    expression();
    // we should now be at the end of the source code so check for token
    consume(TOKEN_EOF, "Expect end of expression");
    // wrap things up
    endCompiler();
    // if the parser had no error then compilation was a success so we return true
    return !parser.hadError;
}
