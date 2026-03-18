#pragma once

#include <maxql/database/database.h>
#include <maxql/core/error.h>
#include <maxql/index/index.h>
#include <maxql/schema/schema.h>

typedef struct {
    char* table_name;
    Index indexes[MAX_TABLE_INDEX_COUNT];
    IndexDefinition* index_defs[MAX_TABLE_INDEX_COUNT];
    uint8_t count;
} IndexSet;

void index_set_load(TableMeta* table_meta, IndexSet* index_set);
void index_set_create(TableMeta* table_meta, IndexSet* index_set);
bool index_set_has_duplicate(IndexSet* index_set, DataValue* values, IndexDefinition** out_index_def);
bool index_set_has_conflict(IndexSet* index_set, DataValue* values, FileOffset offset, IndexDefinition** out_index_def);
void index_set_insert(IndexSet* index_set, DataValue values[], FileOffset offset);
void index_set_save(IndexSet* index_set);
void index_set_free(IndexSet* index_set);
bool index_set_delete(IndexSet* index_set, DataValue values[], FileOffset offset);
void index_set_purge(TableName table_name, IndexMeta* index_meta);

Error backfill_index(TableSchema* schema, Index* index, IndexDefinition* index_def);
