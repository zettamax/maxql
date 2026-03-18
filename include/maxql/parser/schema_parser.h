#pragma once

#include <maxql/parser/parser_state.h>
#include <maxql/parser/ast.h>

typedef struct {
    Ident type;
    bool is_unique;
    Ident name;
    IndexKeyPartArray key_parts;
} IndexDef;

typedef struct {
    IndexDef* data;
    size_t size;
    size_t capacity;
} IndexDefArray;

typedef struct {
    ColumnDefArray column_defs;
    IndexDefArray index_defs;
} SchemaParseResult;

SchemaParseResult parse_schema(Parser* parser);
void schema_parse_result_free(SchemaParseResult* result);
