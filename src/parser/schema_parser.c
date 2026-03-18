#include <maxql/parser/schema_parser.h>
#include <maxql/parser/primitives.h>

static ColumnDefArray parse_schema_column_def_list(Parser* parser)
{
    ColumnDefArray column_defs;
    da_init(&column_defs);
    while (!parser_check(parser, TOKEN_RPAREN) && error_is_ok(parser->error)) {
        da_push(&column_defs, parse_column_def(parser));
        parser_accept(parser, TOKEN_COMMA);
    }
    return column_defs;
}

static IndexDef parse_index_def(Parser* parser)
{
    IndexDef def = {};
    def.type = parse_ident(parser);
    def.is_unique = parser_accept_keyword(parser, "unique");
    def.name = parse_ident(parser);
    parser_expect(parser, TOKEN_LPAREN);
    def.key_parts = parse_index_key_part_list(parser);
    parser_expect(parser, TOKEN_RPAREN);
    return def;
}

static IndexDefArray parse_index_def_list(Parser* parser)
{
    IndexDefArray defs;
    da_init(&defs);
    while (!parser_check(parser, TOKEN_RPAREN) && error_is_ok(parser->error)) {
        da_push(&defs, parse_index_def(parser));
        parser_accept(parser, TOKEN_COMMA);
    }
    return defs;
}

SchemaParseResult parse_schema(Parser* parser)
{
    SchemaParseResult result = {};

    parser_expect_keyword(parser, "table");
    parser_expect(parser, TOKEN_LPAREN);

    parser_expect_keyword(parser, "columns");
    parser_expect(parser, TOKEN_LPAREN);
    result.column_defs = parse_schema_column_def_list(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_accept(parser, TOKEN_COMMA);

    parser_expect_keyword(parser, "indexes");
    parser_expect(parser, TOKEN_LPAREN);
    result.index_defs = parse_index_def_list(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_accept(parser, TOKEN_COMMA);

    parser_expect(parser, TOKEN_RPAREN);

    return result;
}

void schema_parse_result_free(SchemaParseResult* result)
{
    da_free(&result->column_defs);
    for (size_t i = 0; i < result->index_defs.size; i++)
        da_free(&result->index_defs.data[i].key_parts);
    da_free(&result->index_defs);
}
