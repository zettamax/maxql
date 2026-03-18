#pragma once

#include <maxql/core/lexer.h>
#include <maxql/core/error.h>

typedef struct {
    Lexer lexer;
    Token current;
    Error error;
} Parser;

Parser parser_init(StrView input);
