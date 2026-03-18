#include <maxql/parser/primitives.h>
#include <maxql/parser/parser.h>
#include <maxql/parser/ast.h>
#include <maxql/core/error.h>

void parser_set_error(Parser* parser, ErrorCode code, const char* msg)
{
    if (!error_is_ok(parser->error))
        return;

    parser->error = error_fmt(code, msg);
}

void parser_next(Parser* parser)
{
    parser->current = lexer_next(&parser->lexer);
    if (parser->current.type == TOKEN_INVALID) {
        parser_set_error(parser, ERR_LEXER, "Invalid token");
    }
}

bool parser_check(Parser* parser, TokenType type)
{
    return parser->current.type == type;
}

bool parser_accept(Parser* parser, TokenType type)
{
    bool is_accepted = parser->current.type == type;
    if (is_accepted)
        parser_next(parser);

    return is_accepted;
}

void parser_expect(Parser* parser, TokenType type)
{
    if (!parser_accept(parser, type))
        parser_set_error(parser, ERR_PARSER, "Unexpected token");
}

bool parser_accept_keyword(Parser* parser, const char* keyword)
{
    if (!parser_check(parser, TOKEN_IDENT))
        return false;

    if (!sv_equal_cstr(parser->current.lexeme, keyword))
        return false;

    parser_next(parser);
    return true;
}

void parser_expect_keyword(Parser* parser, const char* keyword)
{
    if (!parser_accept_keyword(parser, keyword))
        parser_set_error(parser, ERR_PARSER, "Expected specific keyword");
}

Ident parse_ident(Parser* parser)
{
    Ident ident = parser->current.lexeme;
    parser_expect(parser, TOKEN_IDENT);
    return ident;
}

IdentArray parse_ident_list(Parser* parser)
{
    IdentArray idents;
    da_init(&idents);
    do {
        da_push(&idents, parse_ident(parser));
    } while (parser_accept(parser, TOKEN_COMMA));

    return idents;
}

StrView parse_number(Parser* parser)
{
    StrView lex = parser->current.lexeme;
    parser_expect(parser, TOKEN_NUMBER);
    return lex;
}

StrView parse_string(Parser* parser)
{
    StrView lex = parser->current.lexeme;
    parser_expect(parser, TOKEN_STRING);
    return lex;
}

StrView parse_hex(Parser* parser)
{
    StrView lex = parser->current.lexeme;
    parser_expect(parser, TOKEN_HEX);
    return lex;
}

RawValue parse_value(Parser* parser)
{
    if (parser_check(parser, TOKEN_NUMBER))
        return (RawValue){LIT_NUMBER, parse_number(parser)};

    if (parser_check(parser, TOKEN_STRING))
        return (RawValue){LIT_STRING, parse_string(parser)};

    if (parser_check(parser, TOKEN_HEX))
        return (RawValue){LIT_HEX, parse_hex(parser)};

    parser_set_error(parser, ERR_PARSER, "Expected number, string or hex literal");
    return (RawValue){};
}

IndexKeyPart parse_index_key_part(Parser* parser)
{
    IndexKeyPart index_key_part = {};
    index_key_part.column = parse_ident(parser);
    if (parser_accept(parser, TOKEN_LPAREN)) {
        index_key_part.has_key_len = true;
        index_key_part.key_len = parse_number(parser);
        parser_expect(parser, TOKEN_RPAREN);
    }

    return index_key_part;
}

IndexKeyPartArray parse_index_key_part_list(Parser* parser)
{
    IndexKeyPartArray parts;
    da_init(&parts);
    do {
        da_push(&parts, parse_index_key_part(parser));
    } while (parser_accept(parser, TOKEN_COMMA));

    return parts;
}

Assignment parse_assignment(Parser* parser)
{
    Ident ident = parse_ident(parser);
    parser_expect(parser, TOKEN_EQUALS);
    RawValue value = parse_value(parser);
    return (Assignment){ident, value};
}

AssignmentArray parse_assignment_list(Parser* parser)
{
    AssignmentArray assignments;
    da_init(&assignments);
    do {
        da_push(&assignments, parse_assignment(parser));
    } while (parser_accept(parser, TOKEN_COMMA));

    return assignments;
}

ConditionOp parse_comparison_op(Parser* parser)
{
    ConditionOp op = OP_UNKNOWN;
    switch (parser->current.type) {
        case TOKEN_EQUALS: op = OP_EQUALS; break;
        case TOKEN_NOT_EQUALS: op = OP_NOT_EQUALS; break;
        case TOKEN_LESS: op = OP_LESS; break;
        case TOKEN_LESS_OR_EQUAL: op = OP_LESS_OR_EQUAL; break;
        case TOKEN_GREATER: op = OP_GREATER; break;
        case TOKEN_GREAT_OR_EQUAL: op = OP_GREAT_OR_EQUAL; break;

        default: parser_set_error(parser, ERR_PARSER, "Expected comparison operator");
    }
    if (op != OP_UNKNOWN)
        parser_next(parser);

    return op;
}

Condition parse_condition(Parser* parser)
{
    Condition cond = {};
    cond.column = parse_ident(parser);
    cond.op = parse_comparison_op(parser);
    cond.value = parse_value(parser);

    return cond;
}

ConditionArray parse_condition_list(Parser* parser)
{
    ConditionArray conditions;
    da_init(&conditions);
    do {
        da_push(&conditions, parse_condition(parser));
    } while (parser_accept_keyword(parser, "and"));

    return conditions;
}

ColumnTypeDef parse_column_type(Parser* parser)
{
    ColumnTypeDef column_type_def = {};
    column_type_def.name = parse_ident(parser);
    if (parser_accept(parser, TOKEN_LPAREN)) {
        column_type_def.len = parse_number(parser);
        column_type_def.has_len = true;
        parser_expect(parser, TOKEN_RPAREN);
    }

    return column_type_def;
}

ColumnDef parse_column_def(Parser* parser)
{
    ColumnDef column_def = {};
    column_def.column = parse_ident(parser);
    column_def.type = parse_column_type(parser);
    if (parser_accept_keyword(parser, "default")) {
        column_def.has_default = true;
        column_def.default_value = parse_value(parser);
    }
    return column_def;
}

ColumnDefArray parse_column_def_list(Parser* parser)
{
    ColumnDefArray column_defs;
    da_init(&column_defs);
    do {
        da_push(&column_defs, parse_column_def(parser));
    } while (parser_accept(parser, TOKEN_COMMA));

    return column_defs;
}