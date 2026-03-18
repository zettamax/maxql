#pragma once

#include "maxql/database/database.h"

#include <maxql/schema/schema.h>

typedef struct {
    TableSchema schema;
} CreateTablePlan;

typedef struct {
    TableMeta* table_meta;
} DropTablePlan;

typedef struct {
    TableMeta* table_meta;
    IndexName index_name;
    IndexType index_type;
    uint16_t node_size;
    uint8_t column_pos[MAX_INDEX_COLUMN_COUNT];
    uint8_t key_parts_len[MAX_INDEX_COLUMN_COUNT];
    uint8_t column_count;
    IndexDefinition index_def;
} CreateIndexPlan;

typedef struct {
    TableMeta* table_meta;
    size_t index_pos;
    IndexName index_name;
} DropIndexPlan;

typedef struct {
    TableMeta* table_meta;
} TruncateTablePlan;
