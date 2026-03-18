#include <maxql/storage/storage.h>
#include <stdio.h>
#include <maxql/index/index_set.h>

void index_set_load(TableMeta* table_meta, IndexSet* index_set)
{
    IndexMeta* index_meta = &table_meta->index_meta;
    index_set->table_name = table_meta->name;

    for (uint8_t i = 0; i < index_meta->index_count; i++) {
        IndexDefinition* index_def = &index_meta->index_defs[i];
        index_load(table_meta->name, index_def, &index_set->indexes[i]);
        index_set->index_defs[i] = index_def;
        index_set->count++;
    }
}

void index_set_create(TableMeta* table_meta, IndexSet* index_set)
{
    IndexMeta* index_meta = &table_meta->index_meta;
    index_set->table_name = table_meta->name;

    for (uint8_t i = 0; i < index_meta->index_count; i++) {
        IndexDefinition* index_def = &index_meta->index_defs[i];
        index_create(index_def, &index_set->indexes[i]);
        index_set->index_defs[i] = index_def;
        index_set->count++;
    }
}

bool index_set_has_duplicate(IndexSet* index_set, DataValue values[], IndexDefinition** out_index_def)
{
    for (uint8_t i = 0; i < index_set->count; i++) {
        IndexDefinition* index_def = index_set->index_defs[i];
        if (!index_def->is_unique)
            continue;

        bool found = index_key_exists(&index_set->indexes[i], index_def, values);
        if (found) {
            *out_index_def = index_def;
            return true;
        }
    }

    return false;
}

bool index_set_has_conflict(IndexSet* index_set, DataValue values[], FileOffset offset, IndexDefinition** out_index_def)
{
    for (uint8_t i = 0; i < index_set->count; i++) {
        IndexDefinition* index_def = index_set->index_defs[i];
        if (!index_def->is_unique)
            continue;

        bool found = index_key_conflicts(&index_set->indexes[i], index_def, values, offset);
        if (found) {
            *out_index_def = index_def;
            return true;
        }
    }

    return false;
}

void index_set_insert(IndexSet* index_set, DataValue values[], FileOffset offset)
{
    for (size_t i = 0; i < index_set->count; i++)
        index_insert(&index_set->indexes[i], index_set->index_defs[i], values, offset);
}

void index_set_save(IndexSet* index_set)
{
    for (size_t i = 0; i < index_set->count; i++)
        index_save(index_set->table_name, index_set->index_defs[i], &index_set->indexes[i]);
}

void index_set_free(IndexSet* index_set)
{
    for (size_t i = 0; i < index_set->count; i++)
        index_free(index_set->index_defs[i], &index_set->indexes[i]);
}

bool index_set_delete(IndexSet* index_set, DataValue values[], FileOffset offset)
{
    for (size_t i = 0; i < index_set->count; i++) {
        bool deleted = index_delete(&index_set->indexes[i], index_set->index_defs[i], values, offset);
        if (!deleted)
            return false;
    }

    return true;
}

void index_set_purge(TableName table_name, IndexMeta* index_meta)
{
    for (size_t i = 0; i < index_meta->index_count; i++)
        index_delete_file(table_name, index_meta->index_defs[i].name);
}

Error backfill_index(TableSchema* schema, Index* index, IndexDefinition* index_def)
{
    Error error = error_ok();

    FILE* data_file = storage_open(schema->table_name);
    fseeko(data_file, 0, SEEK_END);
    size_t file_size = ftello(data_file);
    fseeko(data_file, 0, SEEK_SET);
    size_t offset = 0;

    while (offset < file_size) {
        StoredRow stored_row = {};
        if (storage_read_current_row(data_file, schema, &stored_row.row)) {
            if (index_def->is_unique) {
                bool found = index_key_exists(index, index_def, stored_row.row.values);
                if (found) {
                    error = error_fmt(ERR_EXECUTOR, "Duplicate value(s) for unique index '%s'", index_def->name);
                    goto cleanup;
                }
            }
            index_insert(index, index_def, stored_row.row.values, offset);
        }

        offset = ftello(data_file);
    }

    cleanup:
        fclose(data_file);

    return error;
}
