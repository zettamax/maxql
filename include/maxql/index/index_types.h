#pragma once

#include <maxql/index/comparator.h>

typedef struct IndexDefinition {
    IndexName name;
    IndexType type;
    uint16_t node_size;
    uint16_t key_size;
    bool is_unique;
    uint8_t column_count;
    ColumnName columns[MAX_INDEX_COLUMN_COUNT];
    uint8_t column_pos[MAX_INDEX_COLUMN_COUNT];
    uint8_t part_len[MAX_INDEX_COLUMN_COUNT];
    ValueType part_type[MAX_INDEX_COLUMN_COUNT];
    KeyComparator part_compare[MAX_INDEX_COLUMN_COUNT];
} IndexDefinition;
