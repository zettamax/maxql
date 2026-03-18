#pragma once

#include <maxql/types.h>
#include <maxql/index/index_types.h>
#include <maxql/types/types.h>
#include <maxql/core/error.h>

typedef struct {
    ColumnName column_name;
    ResolvedType resolved_type;
    bool has_default;
    DataValue default_value;
} Column;

typedef struct {
    TableName table_name;
    Column columns[MAX_COLUMNS_PER_TABLE];
    uint8_t column_count;
} TableSchema;

typedef struct {
    IndexDefinition index_defs[MAX_TABLE_INDEX_COUNT];
    uint8_t index_count;
} IndexMeta;

bool table_column_pos(TableSchema* schema, ColumnName column_name, uint8_t* column_pos);
IndexDefinition* get_index_def(IndexMeta* index_meta, IndexName index_name);
void schema_remove_index_def(size_t index_pos, IndexMeta* index_meta);
