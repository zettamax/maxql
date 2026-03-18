#include <maxql/engine/ddl/planner.h>
#include <maxql/types/converter.h>
#include <maxql/core/error.h>
#include <stdint.h>
#include <string.h>
#include <maxql/types.h>
#include <maxql/defaults.h>
#include <maxql/schema/schema.h>
#include <maxql/parser/parser.h>
#include <maxql/index/index.h>
#include <maxql/types/mapper.h>

Error plan_create_table(QueryContext* query_ctx)
{
    CreateTablePlan* create_table = &query_ctx->create_table;
    CreateTableStmt create_table_stmt = query_ctx->parse_result->stmt.create_table;

    TableName table_name;
    sv_to_cstr(create_table_stmt.table, table_name, sizeof(table_name));
    COPY_STRING(create_table->schema.table_name, table_name);
    if (database_table_by_name(query_ctx->app_ctx->database, table_name))
        return error_fmt(ERR_PLANNER, "Table '%s' already exists", table_name);

    ColumnDefArray column_def_array = create_table_stmt.column_defs;
    for (size_t i = 0; i < (uint8_t)column_def_array.size; i++) {
        Column column = {};

        ColumnName column_name;
        sv_to_cstr(column_def_array.data[i].column, column_name, sizeof(column_name));
        COPY_STRING(column.column_name, column_name);

        TypeName type_name;
        sv_to_cstr(column_def_array.data[i].type.name, type_name, sizeof(type_name));

        int32_t column_type_length = 0;
        if (column_def_array.data[i].type.has_len) {
            DataValue col_length_data_value;
            ConvertResult result = convert_int(column_def_array.data[i].type.len, &col_length_data_value);
            if (result != CONVERT_OK)
                return error_fmt(ERR_PLANNER,
                                 "Invalid value for column '%s': %s",
                                 column_name,
                                 convert_result_msg(result));

            column_type_length = col_length_data_value.val.int32;
            if (column_type_length < 0 || column_type_length > 255)
                return error_fmt(ERR_PLANNER, "Column '%s' length is out of range", column_name);
        }

        auto resolve_type_result = resolve_type(type_name,
                                                column_def_array.data[i].type.has_len,
                                                (uint8_t)column_type_length,
                                                &column.resolved_type);
        if (resolve_type_result != RESOLVE_OK)
            return error_fmt(ERR_PLANNER, "Column '%s': %s '%s'",
                             column_name, resolve_result_msg(resolve_type_result), type_name);

        if (column_def_array.data[i].has_default) {
            DataValue column_default_value = {};

            auto convert_result = convert_value(column_def_array.data[i].default_value.type,
                                                column_def_array.data[i].default_value.value,
                                                &column_default_value);
            if (convert_result != CONVERT_OK)
                return error_fmt(ERR_PLANNER, "Invalid value for column '%s': %s",
                                 column_name, convert_result_msg(convert_result));

            auto validate_type_result = resolved_type_validate(&column.resolved_type, &column_default_value);
            if (validate_type_result != VALIDATE_OK)
                return error_fmt(ERR_PLANNER, "Invalid default column '%s' value: %s",
                                 column_name, validate_type_result_msg(validate_type_result));

            column.has_default = true;
            column.default_value = column_default_value;
        }

        create_table->schema.columns[create_table->schema.column_count++] = column;
    }

    return error_ok();
}

Error plan_drop_table(QueryContext* query_ctx)
{
    auto database = query_ctx->app_ctx->database;
    auto drop_table = &query_ctx->drop_table;
    auto drop_table_stmt = query_ctx->parse_result->stmt.drop_table;

    TableName table_name;
    sv_to_cstr(drop_table_stmt.table, table_name, sizeof(table_name));
    drop_table->table_meta = database_table_by_name(database, table_name);
    if (!drop_table->table_meta)
        return error_fmt(ERR_PLANNER, "Table '%s' not found", table_name);

    return error_ok();
}

static Error key_part_length(IndexKeyPart* part, uint8_t schema_size, ColumnName column_name, uint8_t* out_len)
{
    if (part->has_key_len) {
        DataValue key_len_value;

        auto convert_result = convert_int(part->key_len, &key_len_value);
        if (convert_result != CONVERT_OK)
            return error_fmt(ERR_PLANNER, "Incorrect key part length for column '%s': %s",
                             column_name, convert_result_msg(convert_result));

        if (key_len_value.val.int32 < 1)
            return error_fmt(ERR_PLANNER, "Key part length must be positive for column '%s'", column_name);

        if (key_len_value.val.int32 > schema_size)
            return error_fmt(ERR_PLANNER, "Key part length is too big for column '%s'", column_name);

        *out_len = key_len_value.val.int32;
    } else
        *out_len = schema_size;

    return error_ok();
}

Error plan_create_index(QueryContext* query_ctx)
{
    CreateIndexPlan* create_index = &query_ctx->create_index;
    IndexDefinition* index_def = &create_index->index_def;
    CreateIndexStmt create_index_stmt = query_ctx->parse_result->stmt.create_index;

    TableName table_name;
    sv_to_cstr(create_index_stmt.table, table_name, sizeof(table_name));
    create_index->table_meta = database_table_by_name(query_ctx->app_ctx->database, table_name);
    if (!create_index->table_meta)
        return error_fmt(ERR_PLANNER, "Table '%s' not found", table_name);

    IndexMeta* index_meta = &create_index->table_meta->index_meta;
    if (index_meta->index_count >= MAX_TABLE_INDEX_COUNT)
        return error_fmt(ERR_PLANNER, "Table '%s' already has %d indexes (max %d)",
                         table_name, index_meta->index_count, MAX_TABLE_INDEX_COUNT);

    IndexName index_name;
    sv_to_cstr(create_index_stmt.name, index_name, sizeof(index_name));
    IndexDefinition* index = get_index_def(index_meta, index_name);
    if (index)
        return error_fmt(ERR_INDEX, "Index '%s' already exists on table '%s'", index_name, table_name);
    COPY_STRING(index_def->name, index_name);

    const IndexVTable* vtable;
    if ((uint8_t)create_index_stmt.type.len > 0) {
        IndexTypeName index_type_name;
        sv_to_cstr(create_index_stmt.type, index_type_name, sizeof(index_type_name));
        vtable = index_vtable_by_name(index_type_name);
        if (!vtable)
            return error_fmt(ERR_PLANNER, "Unknown index type: '%s'", index_type_name);
    } else {
        vtable = index_vtable(INDEX_DEFAULT_TYPE);
    }

    if (!create_index_stmt.is_unique && !vtable->accepts_non_unique)
        return error_fmt(ERR_PLANNER, "Index type '%s' must be declared unique", vtable->type_name);

    index_def->type = vtable->type;
    index_def->node_size = vtable->default_node_size;
    index_def->is_unique = create_index_stmt.is_unique;

    TableSchema* schema = &create_index->table_meta->schema;
    IndexKeyPartArray key_parts = create_index_stmt.key_parts;
    for (size_t i = 0; i < (uint8_t)key_parts.size; i++)
    {
        uint8_t column_pos;
        ColumnName column_name;
        sv_to_cstr(key_parts.data[i].column, column_name, sizeof(column_name));
        if (!table_column_pos(schema, column_name, &column_pos))
            return error_fmt(ERR_PLANNER, "Column '%s' is not present in table '%s'", column_name, schema->table_name);

        ResolvedType* resolved_type = &schema->columns[column_pos].resolved_type;
        uint8_t key_part_len;
        Error error = key_part_length(&key_parts.data[i], resolved_type->byte_len, column_name, &key_part_len);
        if (!error_is_ok(error))
            return error;

        index_def->column_pos[i] = column_pos;
        COPY_STRING(index_def->columns[i], column_name);
        index_def->part_len[i] = key_part_len;
        index_def->part_type[i] = resolved_type->type_info->value_type;
        index_def->part_compare[i] = resolved_type->type_info->compare;
        index_def->key_size += key_part_len;
    }
    index_def->column_count = key_parts.size;

    return error_ok();
}

Error plan_drop_index(QueryContext* query_ctx)
{
    auto drop_index = &query_ctx->drop_index;
    auto drop_index_stmt = query_ctx->parse_result->stmt.drop_index;

    TableName table_name;
    sv_to_cstr(drop_index_stmt.table, table_name, sizeof(table_name));
    drop_index->table_meta = database_table_by_name(query_ctx->app_ctx->database, table_name);
    if (!drop_index->table_meta)
        return error_fmt(ERR_PLANNER, "Table '%s' not found", table_name);


    IndexMeta* index_meta = &drop_index->table_meta->index_meta;
    IndexName index_name;
    sv_to_cstr(drop_index_stmt.index, index_name, sizeof(index_name));
    IndexDefinition* index_def = get_index_def(index_meta, index_name);
    if (!index_def)
        return error_fmt(ERR_PLANNER, "Index '%s' is not present in table '%s'", index_name, table_name);

    COPY_STRING(drop_index->index_name, index_name);
    drop_index->index_pos = index_def - index_meta->index_defs;

    return error_ok();
}

Error plan_truncate_table(QueryContext* query_ctx)
{
    auto database = query_ctx->app_ctx->database;
    auto truncate_table = &query_ctx->truncate_table;
    auto truncate_table_stmt = query_ctx->parse_result->stmt.truncate_table;

    TableName table_name;
    sv_to_cstr(truncate_table_stmt.table, table_name, sizeof(table_name));
    truncate_table->table_meta = database_table_by_name(database, table_name);
    if (!truncate_table->table_meta)
        return error_fmt(ERR_PLANNER, "Table '%s' not found", table_name);

    return error_ok();
}
