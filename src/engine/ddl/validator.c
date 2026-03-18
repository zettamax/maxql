#include <maxql/engine/ddl/validator.h>
#include <maxql/parser/ast.h>
#include <maxql/parser/parser.h>
#include <maxql/limits.h>
#include <maxql/types/mapper.h>

// ReSharper disable once CppDFAUnreachableCode
Error validate_create_table(QueryContext* query_ctx)
{
    auto create_table_stmt = query_ctx->parse_result->stmt.create_table;
    const ColumnDefArray column_defs = create_table_stmt.column_defs;

    if (column_defs.size > MAX_COLUMNS_PER_TABLE)
        return error_fmt(ERR_VALIDATOR, "Too many columns definitions for table");

    for (size_t i = 0; i < (uint8_t)column_defs.size; i++) {
        ColumnDef* column_def = &column_defs.data[i];
        for (size_t j = i + 1; j < column_defs.size; j++)
            if (sv_equal(column_def->column, column_defs.data[j].column))
                return error_fmt(ERR_VALIDATOR, "Duplicate column name in definition");

        if (column_def->column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

        if (column_def->type.name.len > MAX_TYPE_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column type is too long");

        TypeName tmp_name;
        sv_to_cstr(column_def->type.name, tmp_name, sizeof(tmp_name));
        if (type_by_name(tmp_name) == nullptr)
            return error_fmt(ERR_VALIDATOR, "Unknown column type");
    }
    if (create_table_stmt.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    return error_ok();
}

Error validate_drop_table(QueryContext* query_ctx)
{
    auto drop_table_stmt = query_ctx->parse_result->stmt.drop_table;

    if (drop_table_stmt.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    return error_ok();
}

// ReSharper disable once CppDFAUnreachableCode
Error validate_create_index(QueryContext* query_ctx)
{
    auto create_index_stmt = query_ctx->parse_result->stmt.create_index;

    if (create_index_stmt.name.len > MAX_INDEX_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Index name is too long");

    if (create_index_stmt.type.len > MAX_INDEX_TYPE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Index type is too long");

    if (create_index_stmt.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    IndexKeyPartArray key_parts = create_index_stmt.key_parts;
    if (key_parts.size > MAX_INDEX_COLUMN_COUNT)
        return error_fmt(ERR_VALIDATOR, "Too many key parts provided");

    for (size_t i = 0; i < key_parts.size; i++)
        if (key_parts.data[i].column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    for (size_t i = 0; i < key_parts.size; i++)
        for (size_t j = i + 1; j < key_parts.size; j++)
            if (sv_equal(key_parts.data[i].column, key_parts.data[j].column))
                return error_fmt(ERR_VALIDATOR, "Index must not have duplicate columns");

    return error_ok();
}

// ReSharper disable once CppDFAUnreachableCode
Error validate_drop_index(QueryContext* query_ctx)
{
    auto drop_index_stmt = query_ctx->parse_result->stmt.drop_index;

    if (drop_index_stmt.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    if (drop_index_stmt.index.len > MAX_INDEX_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Index name is too long");

    return error_ok();
}

Error validate_truncate_table(QueryContext* query_ctx)
{
    auto truncate_table_stmt = query_ctx->parse_result->stmt.truncate_table;

    if (truncate_table_stmt.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    return error_ok();
}
