#include <maxql/core/lexer.h>
#include <stddef.h>
#include <stdint.h>

#include <maxql/core/strview.h>


static bool has_more(Lexer* l)
{
    return l->pos < l->input.len;
}

static char current_char(Lexer* l)
{
    return l->input.ptr[l->pos];
}

static char next_char(Lexer* l)
{
    return l->input.ptr[l->pos + 1];
}

static bool is_ident_start(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

static bool is_ident(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           (c == '_');
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_dot(char c)
{
    return c == '.';
}

static bool is_hex_mark(char c)
{
    return c == 'x';
}

static bool is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static bool is_quote(char c)
{
    return c == '"';
}

static void lexer_skip_whitespace(Lexer* lexer)
{
    while (has_more(lexer) && is_whitespace(current_char(lexer)))
        lexer->pos++;
}

static Token lexer_read_ident(Lexer* lexer)
{
    size_t start = lexer->pos;
    while (has_more(lexer) && is_ident(current_char(lexer)))
        lexer->pos++;

    StrView token = sv_from_parts(lexer->input.ptr + start, lexer->pos - start);
    return (Token){TOKEN_IDENT, token};
}

static Token lexer_read_number(Lexer* lexer)
{
    bool has_dot = false;
    size_t start = lexer->pos;
    while (has_more(lexer) && (is_digit(current_char(lexer)) || is_dot(current_char(lexer)))) {
        if (is_dot(current_char(lexer))) {
            if (has_dot)
                break;
            has_dot = true;
        }
        lexer->pos++;
    }
    StrView token = sv_from_parts(lexer->input.ptr + start, lexer->pos - start);
    return (Token){TOKEN_NUMBER, token};
}

static Token lexer_read_hex(Lexer* lexer)
{
    lexer->pos += 2;
    size_t start = lexer->pos;
    while (has_more(lexer) && is_hex_digit(current_char(lexer)))
        lexer->pos++;

    StrView token = sv_from_parts(lexer->input.ptr + start, lexer->pos - start);
    if (token.len == 0)
        return (Token){TOKEN_INVALID, token};

    return (Token){TOKEN_HEX, token};
}

static Token lexer_read_string(Lexer* lexer)
{
    lexer->pos++; // skip opening quote
    size_t start = lexer->pos;

    while (has_more(lexer) && !is_quote(current_char(lexer)))
        lexer->pos++;

    StrView token = sv_from_parts(lexer->input.ptr + start, lexer->pos - start);
    if (!has_more(lexer) || !is_quote(current_char(lexer)))
        return (Token){TOKEN_INVALID, token};

    lexer->pos++; // skip closing quote
    return (Token){TOKEN_STRING, token};
}

static TokenType lexer_symbol_two_chars(char cc, char nc)
{
    if (cc == '>' && nc == '=') return TOKEN_GREAT_OR_EQUAL;
    if (cc == '<' && nc == '=') return TOKEN_LESS_OR_EQUAL;
    if (cc == '!' && nc == '=') return TOKEN_NOT_EQUALS;

    return TOKEN_INVALID;
}

static TokenType lexer_symbol_type(char c)
{
    switch (c) {
        case '<': return TOKEN_LESS;
        case '>': return TOKEN_GREATER;
        case '(': return TOKEN_LPAREN;
        case ')': return TOKEN_RPAREN;
        case ',': return TOKEN_COMMA;
        case '*': return TOKEN_STAR;
        case ';': return TOKEN_SEMICOLON;
        case '=': return TOKEN_EQUALS;
        default: return TOKEN_INVALID;
    }
}

Token lexer_read_symbol(Lexer* lexer)
{
    if (has_more(lexer))
    {
        TokenType token_type = lexer_symbol_two_chars(current_char(lexer), next_char(lexer));
        if (token_type != TOKEN_INVALID)
        {
            StrView token = sv_from_parts(lexer->input.ptr + lexer->pos, 2);
            lexer->pos += 2;
            return (Token){token_type, token};
        }

    }
    TokenType token_type = lexer_symbol_type(current_char(lexer));
    StrView token = sv_from_parts(lexer->input.ptr + lexer->pos, 1);
    lexer->pos += 1;

    return (Token){token_type, token};
}

Lexer lexer_init(StrView input)
{
    return (Lexer){input, 0};
}

Token lexer_next(Lexer* lexer)
{
    lexer_skip_whitespace(lexer);

    if (!has_more(lexer)) {
        StrView token = sv_from_parts(lexer->input.ptr + lexer->input.len, 0);
        return (Token){TOKEN_EOF, token};
    }

    if (is_ident_start(current_char(lexer)))
        return lexer_read_ident(lexer);

    if (is_digit(current_char(lexer))) {
        if (has_more(lexer) && is_hex_mark(next_char(lexer)))
            return lexer_read_hex(lexer);

        return lexer_read_number(lexer);
    }

    if (is_quote(current_char(lexer)))
        return lexer_read_string(lexer);

    return lexer_read_symbol(lexer);
}
