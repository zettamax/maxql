#include <assert.h>
#include <maxql/index/index.h>
#include <maxql/index/btree.h>
#include <maxql/types.h>
#include <string.h>
#include <maxql/lib/fs.h>

static DynArray(IndexVTable) index_registry;
static const IndexVTable builtin_indexes[] = {
    {
        .type = INDEX_BTREE,
        .type_name = "btree",
        .desc = "Simple index with unique-only keys",
        .default_node_size = BTREE_NODE_DEFAULT_SIZE,
        .accepts_non_unique = false,
        .requires_full_match = true,
        btree_build_config,
        btree_free_config,
        btree_supports_op,
        btree_create,
        btree_open,
        btree_load,
        btree_free,
        btree_save,
        btree_insert,
        btree_search,
        btree_search_stream,
        btree_delete,
    }
};

bool register_index_type(const IndexVTable index)
{
    if (index_vtable_by_name(index.type_name))
        return false;

    if (index_vtable(index.type))
        return false;

    da_push(&index_registry, index);
    return true;
}

void index_register_builtin_types()
{
    da_init(&index_registry);
    for (size_t i = 0; i < ARRAY_LEN(builtin_indexes); i++)
        register_index_type(builtin_indexes[i]);
}

void index_unregister_all()
{
    da_free(&index_registry);
}

bool index_registry_next(size_t* cursor, const IndexVTable** vt)
{
    if (*cursor < index_registry.size) {
        *vt = &index_registry.data[(*cursor)++];
        return true;
    }

    return false;
}

const IndexVTable* index_vtable_by_name(const IndexTypeName index_type_name)
{
    for (size_t i = 0; i < index_registry.size; i++)
        if (strcmp(index_registry.data[i].type_name, index_type_name) == 0)
            return &index_registry.data[i];

    return nullptr;
}

const IndexVTable* index_vtable(IndexType index_type)
{
    for (size_t i = 0; i < index_registry.size; i++)
        if (index_registry.data[i].type == index_type)
            return &index_registry.data[i];

    return nullptr;
}

void index_format_file_name(TableName table_name, IndexName index_name, FileName* file_name)
{
    snprintf(*file_name, sizeof(*file_name), "%s-%s.idx", table_name, index_name);
}

void* index_build_config(IndexDefinition* index_def)
{
    return index_vtable(index_def->type)->build_config(index_def);
}

void index_free_config(void* config, IndexType type)
{
    index_vtable(type)->free_config(config);
}

bool index_supports_op(IndexType type, ConditionOp op)
{
    return index_vtable(type)->supports_op(op);
}

void index_create(IndexDefinition* index_def, Index* index)
{
    index->vtable = index_vtable(index_def->type);
    void* config = index->vtable->build_config(index_def);
    index->data = index->vtable->create(config);
}

void index_delete_file(TableName table_name, IndexName index_name)
{
    FileName file_name;
    index_format_file_name(table_name, index_name, &file_name);
    delete_file(file_name, ERR_STORAGE, "Cannot delete index file");
}

void index_open(TableName table_name, IndexDefinition* index_def, void** storage)
{
    FileName file_name;
    index_format_file_name(table_name, index_def->name, &file_name);
    index_vtable(index_def->type)->open(file_name, storage);
}

void index_load(TableName table_name, IndexDefinition* index_def, Index* out_index)
{
    FileName file_name;
    index_format_file_name(table_name, index_def->name, &file_name);
    out_index->vtable = index_vtable(index_def->type);
    void* config = out_index->vtable->build_config(index_def);
    out_index->vtable->load(config, file_name, &out_index->data);
}

void index_free(IndexDefinition* index_def, Index* index)
{
    index_vtable(index_def->type)->free(index->data);
}

void index_save(TableName table_name, IndexDefinition* index_def, Index* index)
{
    FileName file_name;
    index_format_file_name(table_name, index_def->name, &file_name);
    index->vtable->save(index->data, file_name);
}

void* data_value_raw_ptr(DataValue* value)
{
    switch (value->type) {
        case VALUE_INT: return &value->val.int32;
        case VALUE_STRING: return value->val.string;
        case VALUE_BINARY: return value->val.bin.bytes;
        case VALUE_FLOAT: return &value->val.float32;
        default: assert(0 && "Unknown type");
    }
}

void prepare_values(IndexDefinition* index_def, DataValue values[], DataValue out_values[])
{
    for (uint8_t i = 0; i < index_def->column_count; i++) {
        out_values[i] = values[index_def->column_pos[i]];
    }
}

void prepare_index_key(IndexDefinition* index_def, DataValue* values, uint8_t count, uint8_t* index_key_out)
{
    memset(index_key_out, 0, index_def->key_size);
    uint16_t offset = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        DataValue* value = &values[i];
        const void* raw = data_value_raw_ptr(value);
        size_t actual_size;
        switch (value->type) {
            case VALUE_BINARY: actual_size = value->val.bin.len; break;
            case VALUE_STRING: actual_size = strlen(value->val.string); break;
            default: actual_size = index_def->part_len[i]; break;
        }
        if (actual_size > index_def->part_len[i])
            actual_size = index_def->part_len[i];
        memcpy(index_key_out + offset, raw, actual_size);
        offset += index_def->part_len[i];
    }
}

void* index_build_key(IndexDefinition* index_def, DataValue* values, uint8_t count)
{
    void* key = calloc(1, index_def->key_size);
    prepare_index_key(index_def, values, count, key);
    return key;
}

void index_insert(Index* index, IndexDefinition* index_def, DataValue values[], FileOffset offset)
{
    DataValue index_values[index_def->column_count];
    prepare_values(index_def, values, index_values);

    uint8_t index_key[index_def->key_size];
    prepare_index_key(index_def, index_values, index_def->column_count, index_key);

    index->vtable->insert(index->data, index_key, offset);
}

bool index_key_exists(Index* index, IndexDefinition* index_def, DataValue* values)
{
    FileOffset found_offset;
    return index_search(index, index_def, values, &found_offset);
}

bool index_key_conflicts(Index* index, IndexDefinition* index_def, DataValue* values, FileOffset offset)
{
    FileOffset found_offset;
    bool found = index_search(index, index_def, values, &found_offset);
    return found && offset != found_offset;
}

bool index_search(Index* index, IndexDefinition* index_def, DataValue* values, FileOffset* offset)
{
    DataValue index_values[index_def->column_count];
    prepare_values(index_def, values, index_values);

    uint8_t index_key[index_def->key_size];
    prepare_index_key(index_def, index_values, index_def->column_count, index_key);

    const IndexVTable* vtable = index_vtable(index_def->type);
    bool found = vtable->search(index->data, index_key, offset);

    return found;
}

bool index_search_stream(void* config,
                         void* index_file,
                         void* index_key,
                         IndexType index_type,
                         FileOffset* offset)
{
    const IndexVTable* vtable = index_vtable(index_type);
    return vtable->search_stream(config, index_file, index_key, offset);
}

bool index_delete(Index* index, IndexDefinition* index_def, DataValue values[], FileOffset offset)
{
    DataValue index_values[index_def->column_count];
    prepare_values(index_def, values, index_values);

    uint8_t key[index_def->key_size];
    prepare_index_key(index_def, index_values, index_def->column_count, key);

    return index->vtable->delete(index->data, key, offset);
}
