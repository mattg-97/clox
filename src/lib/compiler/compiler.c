#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "value.h"
#include "compiler.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif
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
typedef void (*ParseFn)(bool canAssign);

// this represents a single row in the rules table
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

// forward declarations because why not
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
//static Chunk* currentChunk();
//static void errorAt(Token* token, const char* message);
//static void error(const char* message);
//static void errorAtCurrent(const char* message);
//static void advance();
//static void consume(TokenType type, const char* message);
//static void emitByte(uint8_t byte);
//static void emitBytes(uint8_t byte1, uint8_t byte2);
//static void emitReturn();
//static void endCompiler();
//static ParseRule* getRule(TokenType type);
//static void binary();
//static void expression();
//static void grouping();
//static uint8_t makeConstant(Value value);
//static void emitConstant(Value value);
//static void number();
//static void unary();

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

// turns a value into a constant and adds it to the chunks constant array
static uint8_t makeConstant(Value value) {
    printf("MAKE CONST\n");
    // add the value to the current chunks data region and return its index
    int constant = addConstant(currentChunk(), value);
    // check for error
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk");
        return 0;
    }
    printf("CONST: %d\n", constant);

    // return that constant cast to a byte
    return (uint8_t)constant;
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

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
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
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}


// this function starts at the current token and parses any expression at the given
// precedence level or higher
static void parsePrecedence(Precedence precedence) {
    // first we read the next token
    advance();
    // then look up the corresponding parse rule
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    // if there is no parse rule then there must be a syntax error so we report that and
    // return to the caller
    if (prefixRule == NULL) {
        error("Expect expression");
        return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    // otherwise we call that prefix parse function and let it do its thing
    prefixRule(canAssign);

    // now we look up an infix parser for the next token, and if we find one it means that
    // the prefix expression we already compiled may be an operand for it.
    // But only if the precedence is low enough to permit this infix operation
    // if the precedence is too low, or isnt an infix operator at all, were done and we
    // have parsed all we need to.
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start,
                                         name->length)));
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
      }
  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
      Local* local = &current->locals[i];
      if (local->depth != -1 && local->depth < current->scopeDepth) {
        break;
      }

      if (identifiersEqual(name, &local->name)) {
        error("Already a variable with this name in this scope.");
      }
    }
  addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  declareVariable();
  if (current->scopeDepth > 0) return 0;
  return identifierConstant(&parser.previous);
}

// when this is called the entire first operand and operator will have already been consumed
// eg. 1 + 2 (1 + ) will have been consumed.
// so the compiler will add 1 onto the stack, then 2, then the plus operator
static void binary(bool canAssign) {
    // therefore the operator is the previous token
    TokenType operatorType = parser.previous.type;
    // this is where the precedence takes place, when we parse the right hand operand
    // eg. 2 * 3 + 4 (right hand operand is 3 in this case), we dont need to capture
    // 3 + 4 becasue it is a lower precedence
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return; //unreachable
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
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

static void expressionStatement() {
    printf("EXPRESSION STATEMENT\n");
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression");
    emitByte(OP_POP);
}

static void printStatement() {
    printf("PRINT STATEMENT\n");
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void synchronize() {
    parser.panicMode = false;

      while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
          case TOKEN_CLASS:
          case TOKEN_FUN:
          case TOKEN_VAR:
          case TOKEN_FOR:
          case TOKEN_IF:
          case TOKEN_WHILE:
          case TOKEN_PRINT:
          case TOKEN_RETURN:
            return;

          default:
            ; // Do nothing.
        }

        advance();
      }
}

static void markInitialized() {
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void varDeclaration() {
    printf("VAR_DEC\n");
  uint8_t global = parseVariable("Expect variable name.");

  printf("VAR_PARSED: %d\n", global);


  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  printf("PASSED\n");
  consume(TOKEN_SEMICOLON,
          "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
       declaration();
     }

     consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;
  while (current->localCount > 0 &&
          current->locals[current->localCount - 1].depth >
             current->scopeDepth) {
     emitByte(OP_POP);
     current->localCount--;
   }
}

static void statement() {
    printf("Statement\n");
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();

    } else {
        expressionStatement();
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}



static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void number(bool canAssign) {
    // we assume that the number literal has been consumed and is stored in previous
    // then we use the c std library function to parse it to a double
    double value = strtod(parser.previous.start, NULL);
    // we can now wrap number in a value before storing it in the constant table
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
    // the + 1 and - 2 here trim the quotatin marks and just returns the string value itself
    // it then creates the string object, wraps it in a value and adds it to the constant table
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
        if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
              }
      return i;
    }
  }

  return -1;
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}


static void unary(bool canAssign) {
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
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; //unreachable
    }
}

// we can see here how grouping and unary are fitted into the prefix operators,
// and how the 4 binary operators are in the infix column
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
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

// this function simple returns the rule at a gives index, and is called by binary() to look
// up the precedence of the current operator
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}


bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    // set compiling chunk to chunk parameter
    compilingChunk = chunk;
    // set error flags to false
    parser.hadError = false;
    parser.panicMode = false;
    // primes the scanner
    advance();
    while (!match(TOKEN_EOF)) {
        printf("x\n");
        declaration();
    }
    // wrap things up
    endCompiler();
    // if the parser had no error then compilation was a success so we return true
    return !parser.hadError;
}
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
