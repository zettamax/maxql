#pragma once

#include <maxql/schema/schema.h>
#include <maxql/types.h>
#include <maxql/core/error.h>

bool schema_exist_table(char* table_name);
bool schema_remove_table(char* table_name);
Error schema_load_table(TableName table_name, TableSchema* schema, IndexMeta* index_meta);
Error schema_save_table(TableSchema* table_schema, IndexMeta* index_meta);
