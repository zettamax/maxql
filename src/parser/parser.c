#include <maxql/parser/parser.h>
#include <maxql/parser/primitives.h>
#include <maxql/parser/ast.h>
#include <maxql/core/error.h>

ParseResult parse_select(Parser* parser)
{
    ParseResult result = {.type = QUERY_SELECT};
    SelectStmt stmt = {};
    if (!parser_accept(parser, TOKEN_STAR))
        stmt.columns = parse_ident_list(parser);
    parser_expect_keyword(parser, "from");
    stmt.table = parse_ident(parser);
    stmt.ignore_index = parser_accept_keyword(parser, "ignore_index");
    if (parser_accept_keyword(parser, "where"))
        stmt.where = parse_condition_list(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.select = stmt};

    return result;
}

ParseResult parse_delete(Parser* parser)
{
    ParseResult result = {.type = QUERY_DELETE};
    DeleteStmt stmt = {};
    parser_expect_keyword(parser, "from");
    stmt.table = parse_ident(parser);
    parser_expect_keyword(parser, "where");
    stmt.where = parse_condition_list(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.delete = stmt};

    return result;
}

ParseResult parse_insert(Parser* parser)
{
    ParseResult result = {.type = QUERY_INSERT};
    InsertStmt stmt = {};
    parser_expect_keyword(parser, "into");
    stmt.table = parse_ident(parser);
    parser_expect_keyword(parser, "set");
    stmt.assignments = parse_assignment_list(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.insert = stmt};

    return result;
}

ParseResult parse_update(Parser* parser)
{
    ParseResult result = {.type = QUERY_UPDATE};
    UpdateStmt stmt = {};
    stmt.table = parse_ident(parser);
    parser_expect_keyword(parser, "set");
    stmt.assignments = parse_assignment_list(parser);
    parser_expect_keyword(parser, "where");
    stmt.where = parse_condition_list(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.update = stmt};

    return result;
}

ParseResult parse_create_table(Parser* parser)
{
    ParseResult result = {.type = QUERY_CREATE_TABLE};
    CreateTableStmt stmt = {};
    stmt.table = parse_ident(parser);
    parser_expect(parser, TOKEN_LPAREN);
    stmt.column_defs = parse_column_def_list(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.create_table = stmt};

    return result;
}

ParseResult parse_create_index(Parser* parser, bool is_unique)
{
    ParseResult result = {.type = QUERY_CREATE_INDEX};
    CreateIndexStmt stmt = {};
    stmt.is_unique = is_unique;
    stmt.name = parse_ident(parser);
    if (parser_accept_keyword(parser, "using"))
        stmt.type = parse_ident(parser);
    parser_expect_keyword(parser, "on");
    stmt.table = parse_ident(parser);
    parser_expect(parser, TOKEN_LPAREN);
    stmt.key_parts = parse_index_key_part_list(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.create_index = stmt};

    return result;
}

ParseResult parse_drop_table(Parser* parser)
{
    ParseResult result = {.type = QUERY_DROP_TABLE};
    DropTableStmt stmt = {};
    stmt.table = parse_ident(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.drop_table = stmt};

    return result;
}

ParseResult parse_show_tables(Parser* parser)
{
    ParseResult result = {.type = QUERY_SHOW_TABLES};
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);

    return result;
}

ParseResult parse_show_table(Parser* parser)
{
    ParseResult result = {.type = QUERY_SHOW_TABLE};
    ShowTableStmt stmt = {};
    stmt.table = parse_ident(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.show_table = stmt};

    return result;
}

ParseResult parse_show_index(Parser* parser)
{
    ParseResult result = {.type = QUERY_SHOW_INDEX};
    ShowIndexStmt stmt = {};
    parser_expect_keyword(parser, "on");
    stmt.table = parse_ident(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.show_index = stmt};

    return result;
}

ParseResult parse_drop_index(Parser* parser)
{
    ParseResult result = {.type = QUERY_DROP_INDEX};
    DropIndexStmt stmt = {};
    stmt.index = parse_ident(parser);
    parser_expect_keyword(parser, "on");
    stmt.table = parse_ident(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.drop_index = stmt};

    return result;
}

ParseResult parse_show_index_types(Parser* parser)
{
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);

    return (ParseResult){.type = QUERY_SHOW_INDEX_TYPES};
}

ParseResult parse_truncate_table(Parser* parser)
{
    ParseResult result = {.type = QUERY_TRUNCATE_TABLE};
    TruncateTableStmt stmt = {};
    stmt.table = parse_ident(parser);
    parser_accept(parser, TOKEN_SEMICOLON);
    parser_expect(parser, TOKEN_EOF);
    result.stmt = (RawStmt){.truncate_table = stmt};

    return result;
}

ParseResult parse_query(Parser* parser)
{
    if (parser_check(parser, TOKEN_IDENT)) {
        if (parser_accept_keyword(parser, "select"))
            return parse_select(parser);

        if (parser_accept_keyword(parser, "delete"))
            return parse_delete(parser);

        if (parser_accept_keyword(parser, "insert"))
            return parse_insert(parser);

        if (parser_accept_keyword(parser, "update"))
            return parse_update(parser);

        if (parser_accept_keyword(parser, "create")) {

            if (parser_accept_keyword(parser, "table"))
                return parse_create_table(parser);

            if (parser_accept_keyword(parser, "unique")) {
                parser_expect_keyword(parser, "index");
                return parse_create_index(parser, true);
            }

            if (parser_accept_keyword(parser, "index"))
                return parse_create_index(parser, false);

            parser_set_error(parser, ERR_PARSER, "Unknown create query");
        }

        if (parser_accept_keyword(parser, "drop")) {

            if (parser_accept_keyword(parser, "table"))
                return parse_drop_table(parser);

            if (parser_accept_keyword(parser, "index"))
                return parse_drop_index(parser);

            parser_set_error(parser, ERR_PARSER, "Unknown drop query");
        }

        if (parser_accept_keyword(parser, "truncate")) {
            return parse_truncate_table(parser);
        }

        if (parser_accept_keyword(parser, "show")) {

            if (parser_accept_keyword(parser, "tables"))
                return parse_show_tables(parser);

            if (parser_accept_keyword(parser, "table"))
                return parse_show_table(parser);

            if (parser_accept_keyword(parser, "index")) {

                if (parser_accept_keyword(parser, "types"))
                    return parse_show_index_types(parser);

                return parse_show_index(parser);
            }

            parser_set_error(parser, ERR_PARSER, "Unknown show query");
        }
    }

    parser_set_error(parser, ERR_PARSER, "Expected query type");
    return (ParseResult){};
}

Parser parser_init(StrView input)
{
    Parser p = {};
    p.error = error_ok();
    p.lexer = lexer_init(input);
    parser_next(&p);
    return p;
}

void parser_result_free(ParseResult* parse_result)
{
    switch (parse_result->type) {
        case QUERY_SELECT:
            da_free(&parse_result->stmt.select.columns);
            da_free(&parse_result->stmt.select.where);
            break;
        case QUERY_INSERT:
            da_free(&parse_result->stmt.insert.assignments);
            break;
        case QUERY_DELETE:
            da_free(&parse_result->stmt.delete.where);
            break;
        case QUERY_UPDATE:
            da_free(&parse_result->stmt.update.assignments);
            da_free(&parse_result->stmt.update.where);
            break;
        case QUERY_CREATE_TABLE:
            da_free(&parse_result->stmt.create_table.column_defs);
            break;
        case QUERY_CREATE_INDEX:
            da_free(&parse_result->stmt.create_index.key_parts);
            break;

        default:
            break;
    }
}