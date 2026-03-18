#include <maxql/engine/ddl/executor.h>
#include <maxql/index/index_set.h>
#include <maxql/view/output.h>
#include <maxql/schema/schema_io.h>
#include <maxql/storage/storage.h>
#include <maxql/index/index.h>

Error execute_create_table(QueryContext* query_ctx)
{
    TableSchema* table_schema = &query_ctx->create_table.schema;
    Database* database = query_ctx->app_ctx->database;

    Error error = schema_save_table(table_schema, nullptr);
    if (!error_is_ok(error))
        return error;

    error = storage_create_table(table_schema->table_name);
    if (!error_is_ok(error))
        return error;

    error = database_add_table(database, table_schema);

    if (!error_is_ok(error))
        return error;

    print_ok();
    return error_ok();
}

Error execute_drop_table(QueryContext* query_ctx)
{
    DropTablePlan* drop_table = &query_ctx->drop_table;
    TableMeta* table_meta = drop_table->table_meta;
    Database* database = query_ctx->app_ctx->database;

    index_set_purge(table_meta->name, &table_meta->index_meta);

    if (!schema_remove_table(table_meta->name))
        return error_fmt(ERR_EXECUTOR, "Cannot delete schema file");

    if (!storage_remove_table(table_meta->name))
        return error_fmt(ERR_EXECUTOR, "Cannot delete data file");

    Error error = database_remove_table(database, table_meta->name);
    if (!error_is_ok(error))
        return error;

    print_ok();
    return error_ok();
}

Error execute_create_index(QueryContext* query_ctx)
{
    TableMeta* table_meta = query_ctx->create_index.table_meta;
    TableSchema* table_schema = &table_meta->schema;
    IndexMeta* index_meta = &table_meta->index_meta;
    IndexDefinition* index_def = &query_ctx->create_index.index_def;

    Index index;
    index_create(index_def, &index);

    Error error = backfill_index(table_schema, &index, index_def);
    if (!error_is_ok(error)) {
        index_free(index_def, &index);
        return error;
    }

    index_save(table_meta->name, index_def, &index);
    index_free(index_def, &index);

    index_meta->index_defs[index_meta->index_count++] = *index_def;
    error = schema_save_table(table_schema, index_meta);
    if (!error_is_ok(error))
        return error;

    print_ok();
    return error_ok();
}

Error execute_drop_index(QueryContext* query_ctx)
{
    DropIndexPlan* drop_index = &query_ctx->drop_index;
    IndexMeta* index_meta = &drop_index->table_meta->index_meta;
    TableSchema* table_schema = &drop_index->table_meta->schema;

    schema_remove_index_def(drop_index->index_pos, index_meta);
    Error error = schema_save_table(table_schema, index_meta);
    if (!error_is_ok(error))
        return error;

    index_delete_file(table_schema->table_name, drop_index->index_name);

    print_ok();
    return error_ok();
}

Error execute_truncate_table(QueryContext* query_ctx)
{
    TableMeta* table_meta = query_ctx->truncate_table.table_meta;

    if (!storage_remove_table(table_meta->name))
        return error_fmt(ERR_STORAGE, "Cannot delete data file");

    Error error = storage_create_table(table_meta->name);
    if (!error_is_ok(error))
        return error;

    index_set_purge(table_meta->name, &table_meta->index_meta);
    IndexSet index_set = {};
    index_set_create(table_meta, &index_set);
    index_set_save(&index_set);
    index_set_free(&index_set);

    print_ok();
    return error_ok();
}
