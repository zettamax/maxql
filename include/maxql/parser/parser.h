#pragma once

#include <maxql/parser/parser_state.h>
#include <maxql/parser/ast.h>

typedef union {
    SelectStmt select;
    InsertStmt insert;
    DeleteStmt delete;
    UpdateStmt update;
    CreateTableStmt create_table;
    DropTableStmt drop_table;
    CreateIndexStmt create_index;
    ShowTableStmt show_table;
    ShowIndexStmt show_index;
    DropIndexStmt drop_index;
    TruncateTableStmt truncate_table;
} RawStmt;

typedef struct {
    QueryType type;
    RawStmt stmt;
} ParseResult;

ParseResult parse_query(Parser* parser);
void parser_result_free(ParseResult* parse_result);
