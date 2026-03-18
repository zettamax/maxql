#pragma once

#include <maxql/types.h>
#include <maxql/index/index_types.h>

static constexpr uint16_t BTREE_NODE_DEFAULT_SIZE = 4096;

void* btree_build_config(IndexDefinition* index_def);
void btree_free_config(void* config);
bool btree_supports_op(ConditionOp op);
void* btree_create(void* config);
void btree_load(void* config, FileName, void** data);
void btree_free(void* data);
void btree_save(void*, FileName);
void btree_insert(void*, const void*, FileOffset);
bool btree_search(void*, const void*, FileOffset*);
void btree_open(FileName file_name, void** data);
bool btree_search_stream(void* config, void* data, const void* key, FileOffset* offset);
bool btree_delete(void*, const void*, FileOffset);
