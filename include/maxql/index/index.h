#pragma once

#include <maxql/types.h>
#include <maxql/index/index_types.h>

typedef struct {
    IndexType type;
    IndexTypeName type_name;
    char desc[64];
    uint16_t default_node_size;
    bool accepts_non_unique;
    bool requires_full_match;
    void* (*build_config)(IndexDefinition*);
    void (*free_config)(void* config);
    bool (*supports_op)(ConditionOp op);
    void* (*create)(void* config);
    void (*open)(FileName, void** storage);
    void (*load)(void* configs, FileName, void** data);
    void (*free)(void* data);
    void (*save)(void* data, FileName);
    void (*insert)(void* data, const void* index_key, FileOffset);
    bool (*search)(void* data, const void* index_key, FileOffset*);
    bool (*search_stream)(void* config, void* index_file, const void* index_key, FileOffset*);
    bool (*delete)(void* data, const void* index_key, FileOffset);
} IndexVTable;

typedef struct Index {
    const IndexVTable* vtable;
    void* data;
} Index;

void index_register_builtin_types();
void index_unregister_all();
bool index_registry_next(size_t* cursor, const IndexVTable** vt);
const IndexVTable* index_vtable_by_name(const IndexTypeName index_type_name);
const IndexVTable* index_vtable(IndexType index_type);

void* index_build_config(IndexDefinition* index_def);
void index_free_config(void* config, IndexType type);
void* index_build_key(IndexDefinition* index_def, DataValue* values, uint8_t count);
bool index_supports_op(IndexType type, ConditionOp op);
void index_create(IndexDefinition* index_def, Index* index);
void index_delete_file(TableName table_name, IndexName index_name);
void index_open(TableName, IndexDefinition* index_def, void**);
void index_load(TableName table_name, IndexDefinition* index_def, Index* out_index);
void index_free(IndexDefinition* index_def, Index* index);
void index_save(TableName table_name, IndexDefinition* index_def, Index* index);
void index_insert(Index* index, IndexDefinition* index_def, DataValue* values, FileOffset offset);
bool index_key_exists(Index* index, IndexDefinition* index_def, DataValue* values);
bool index_key_conflicts(Index* index, IndexDefinition* index_def, DataValue* values, FileOffset offset);
bool index_search(Index* index, IndexDefinition* index_def, DataValue* value, FileOffset* offset);
bool index_search_stream(void* config, void* index_file, void* index_key, IndexType, FileOffset*);
bool index_delete(Index* index, IndexDefinition* index_def, DataValue values[], FileOffset offset);
