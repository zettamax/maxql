#pragma once

#include <maxql/schema/schema.h>
#include <maxql/parser/schema_parser.h>

Error index_resolve(SchemaParseResult* parse_result, TableSchema* schema, IndexMeta* out);
void index_serialize(IndexMeta* index_meta, StrBuf* buf, const char* indent);
