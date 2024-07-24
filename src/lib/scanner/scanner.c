#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    // marks the beginning of the current lexeme (word) being scanned
    const char* start;
    // the current character being looked at
    const char* current;
    // the current line number we are scanning
    int line;
} Scanner;

// scanner variable
Scanner scanner;

// initialize the scanner with sensible defaults
void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

// dereferences scanner.current and checks whether its an EOF char
static bool isAtEnd() {
    return *scanner.current == '\0';
}

// Creates a token from a type using the scanner data
static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    //printf("Token: type = %i, start = %s, length = %i, line = %i\n", token.type, token.start, token.length, token.line);
    return token;
}

// Creats an error token which allows us to print compilation errors
static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

// advances the current char being pointer to
static char advance() {
    // consumes it by making the scanner 'walk' past it
    scanner.current++;
    // returns the 'consumed character'
    return scanner.current[-1];
}

// checks if a character matches an expected one
static bool match(char expected) {
    // return false if we're at the end of a file
    if (isAtEnd()) return false;
    // deref the character and check if it doesnt match expected
    if (*scanner.current != expected) return false;
    // advance current char
    scanner.current++;
    // return true if guards passed
    return true;
}

// exactly the same as advance but doesnt 'consume' the current character by
// stepping over it
static char peek() {
    // returns dereferenced current
    return *scanner.current;
}

// same as peek but one character ahead
static char peekNext() {
    if (isAtEnd()) return '\0';
    // the current char array index 1 is the next char
    return scanner.current[1];
}

// this function allows us to skip whitespace as we dont actually care about any of it
static void skipWhitespace() {
    for (;;) {
        // peek the char
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                // if it's a tab or whitespace just keep going
                advance();
                break;
            case '\n':
                // if its a newline char, increment scanner.line and then advance
                scanner.line++;
                advance();
                break;
            case '/':
                // if its a / and so is the next character then we know its a comment
                if (peekNext() == '/') {
                    //comments go until the end of the line
                    // basically keep advancing until we hit a new line or the end of a file
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token string() {
    // loop until we reach closing quotes or end of file
    while (peek() != '"' && !isAtEnd()) {
        // check if character is a new line then increment
        if (peek() == '\n') scanner.line++;
        // keep going
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string");

    // for the closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static Token number() {
    // keep advancing whilst char is a digit
    while (isDigit(peek())) advance();

    // look for fractional part
    // check if char is . and next char is a digit
    if (peek() == '.' && isDigit(peekNext())) {
        // consume the '.'
        advance();
        // keep advancing whilst digit
        while (isDigit(peek())) advance();
    }
    // return number token
    return makeToken(TOKEN_NUMBER);
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            // we want to be able to have variables such as hello_func
            c == '_';
}

// checks whether the identifier is a keyword
//
// takes start, length, the 'rest' of the keyword and then a token type to return
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    // if lexeme length is the same length as the keyword
    if (scanner.current - scanner.start == start + length
        // the remaining characters must also match
        && memcmp(scanner.start + start, rest, length) == 0) {
        // then we can return the token type
        return type;
    }
    // if it doesnt match a keyword then just return identifier
    return TOKEN_IDENTIFIER;
}

// this basically parses the lexeme for an identifier, which is either a variable name
// or a reserved keyword such as else, or, nil etc.
static TokenType identifierType() {
    // switch on the first letter of the current lexeme
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_OR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
              if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                  case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                  case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
              }
              break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    // if character is a number or a word keep advancing
    while (isAlpha(peek()) || isDigit(peek())) advance();
    // return correct identifier type
    return makeToken(identifierType());
}

// This is where all the work is done.
Token scanToken() {
    // the scanner doesnt care about whitespace, so skip it all
    skipWhitespace();
    // start of lexeme = current character
    scanner.start = scanner.current;
    // if end of file then return token
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    // advance the token
    char c = advance();
    // check for alpha or digit
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    // switch on the characters and return correct tokens
    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }
    //return error if not recognised character
    return errorToken("Unexpected character");
}
