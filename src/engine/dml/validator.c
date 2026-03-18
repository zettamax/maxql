#include <maxql/engine/dml/validator.h>
#include <maxql/limits.h>

// ReSharper disable once CppDFAUnreachableCode
Error validate_select(QueryContext* query_ctx)
{
    const SelectStmt select = query_ctx->parse_result->stmt.select;

    if (select.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    if (select.columns.size > MAX_COLUMNS_IN_SELECT)
        return error_fmt(ERR_VALIDATOR, "Too many columns");

    for (size_t i = 0; i < select.columns.size; i++)
        if (select.columns.data[i].len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    for (size_t i = 0; i < select.columns.size; i++)
        for (size_t j = i + 1; j < select.columns.size; j++)
            if (sv_equal(select.columns.data[i], select.columns.data[j]))
                return error_fmt(ERR_VALIDATOR, "Duplicate column");

    if (select.where.size > MAX_WHERE_CONDITIONS)
        return error_fmt(ERR_VALIDATOR, "Too many where conditions");

    for (size_t i = 0; i < select.where.size; i++)
        if (select.where.data[i].column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    return error_ok();
}

// ReSharper disable once CppDFAUnreachableCode
Error validate_insert(QueryContext* query_ctx)
{
    const InsertStmt insert = query_ctx->parse_result->stmt.insert;

    if (insert.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    if (insert.assignments.size > MAX_COLUMNS_PER_TABLE)
        return error_fmt(ERR_VALIDATOR, "Too many assignments");

    for (size_t i = 0; i < insert.assignments.size; i++)
        if (insert.assignments.data[i].column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    for (size_t i = 0; i < insert.assignments.size; i++)
        for (size_t j = i + 1; j < insert.assignments.size; j++)
            if (sv_equal(insert.assignments.data[i].column, insert.assignments.data[j].column))
                return error_fmt(ERR_VALIDATOR, "Duplicate column");

    return error_ok();
}

// ReSharper disable once CppDFAUnreachableCode
Error validate_delete(QueryContext* query_ctx)
{
    const DeleteStmt delete = query_ctx->parse_result->stmt.delete;

    if (delete.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    if (delete.where.size > MAX_WHERE_CONDITIONS)
        return error_fmt(ERR_VALIDATOR, "Too many where conditions");

    for (size_t i = 0; i < delete.where.size;i++)
        if (delete.where.data[i].column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    return error_ok();
}

// ReSharper disable once CppDFAUnreachableCode
Error validate_update(QueryContext* query_ctx)
{
    UpdateStmt update = query_ctx->parse_result->stmt.update;

    if (update.table.len > MAX_TABLE_NAME_SIZE)
        return error_fmt(ERR_VALIDATOR, "Table name is too long");

    if (update.assignments.size > MAX_COLUMNS_PER_TABLE)
        return error_fmt(ERR_VALIDATOR, "Too many assignments");

    for (size_t i = 0; i < update.assignments.size; i++)
        if (update.assignments.data[i].column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    for (size_t i = 0; i < update.assignments.size; i++)
        for (size_t j = i + 1; j < update.assignments.size; j++)
            if (sv_equal(update.assignments.data[i].column, update.assignments.data[j].column))
                return error_fmt(ERR_VALIDATOR, "Duplicate column");

    if (update.where.size > MAX_WHERE_CONDITIONS)
        return error_fmt(ERR_VALIDATOR, "Too many where conditions");

    for (size_t i = 0; i < update.where.size;i++)
        if (update.where.data[i].column.len > MAX_COLUMN_NAME_SIZE)
            return error_fmt(ERR_VALIDATOR, "Column name is too long");

    return error_ok();
}