#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
} Parser;

Parser parser;

static void errorAt(Token* token, const char* message) {
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}

void compile(const char* source, Chunk* chunk) {
    initScanner(source);
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
     }
    //advance();
    //expression();
    //consume(TOKEN_EOF, "Expect end of expression");
}
