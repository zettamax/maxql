#pragma once

#include <stddef.h>
#include <maxql/core/strview.h>

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENT,
    TOKEN_NUMBER,
    TOKEN_HEX,
    TOKEN_STRING,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COMMA,
    TOKEN_STAR,
    TOKEN_SEMICOLON,
    TOKEN_NOT,
    TOKEN_EQUALS,
    TOKEN_NOT_EQUALS,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_OR_EQUAL,
    TOKEN_GREAT_OR_EQUAL,
    TOKEN_INVALID,
} TokenType;

typedef struct {
    TokenType type;
    StrView lexeme;
} Token;

typedef struct {
    StrView input;
    size_t pos;
} Lexer;

Lexer lexer_init(StrView input);
Token lexer_next(Lexer* lexer);
