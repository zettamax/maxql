#include <maxql/engine/dml/analyzer.h>
#include <maxql/types/converter.h>
#include <maxql/types/mapper.h>

bool find_column_index(TableSchema* schema, Ident column_name, uint8_t* column_index)
{
    for (uint8_t i = 0; i < schema->column_count; i++) {
        if (sv_equal_cstr(column_name, schema->columns[i].column_name)) {
            *column_index = i;
            return true;
        }
    }

    return false;
}

Error analyze_select_columns(IdentArray columns, TableSchema* schema, SelectExecPlan* select_plan)
{
    select_plan->column_count = columns.size ? columns.size : schema->column_count;

    if (columns.size == 0)
        for (uint8_t i = 0; i < schema->column_count; i++)
            select_plan->query_column_order[i] = i;
    else
        for (uint8_t i = 0; i < (uint8_t)columns.size; i++) {
            uint8_t column_index;
            if (!find_column_index(schema, columns.data[i], &column_index))
                return error_fmt(ERR_ANALYZER, "Column '%.*s' is not present in table '%s'",
                                 (int)columns.data[i].len, columns.data[i].ptr, schema->table_name);

            select_plan->query_column_order[i] = column_index;
        }

    return error_ok();
}

Error analyze_where_conditions(ConditionArray* where, TableSchema* schema, WhereFilter* where_filter)
{
    for (uint8_t i = 0; i < (uint8_t)where->size; i++) {
        uint8_t column_index;
        if (!find_column_index(schema, where->data[i].column, &column_index))
            return error_fmt(ERR_ANALYZER, "Column '%.*s' is not present in table '%s'",
                             (int)where->data[i].column.len, where->data[i].column.ptr, schema->table_name);

        DataValue col_data_value = {};

        auto result = convert_value(where->data[i].value.type, where->data[i].value.value, &col_data_value);
        if (result != CONVERT_OK)
            return error_fmt(ERR_ANALYZER,
                             "Invalid value for column '%.*s': %s",
                             (int)where->data[i].column.len,
                             where->data[i].column.ptr,
                             convert_result_msg(result));

        Column column = schema->columns[column_index];
        const TypeInfo* col_type_info = column.resolved_type.type_info;
        if (col_type_info->value_type != col_data_value.type) {
            return error_fmt(ERR_ANALYZER, "Column '%s' type (%s) do not match provided value",
                             column.column_name, col_type_info->type_name);
        }

        where_filter->predicates[i] = (WherePredicate){
            column_index,
            col_data_value,
            where->data[i].op,
        };
    }
    where_filter->predicate_count = where->size;

    return error_ok();
}

Error analyze_select(QueryContext* query_ctx)
{
    SelectStmt select = query_ctx->parse_result->stmt.select;
    SelectExecPlan* select_plan = &query_ctx->select;

    TableName table_name;
    sv_to_cstr(select.table, table_name, sizeof(TableName));
    query_ctx->table_meta = database_table_by_name(query_ctx->app_ctx->database, table_name);
    if (query_ctx->table_meta == nullptr)
        return error_fmt(ERR_ANALYZER, "Table '%s' not exist", table_name);

    TableSchema* schema = &query_ctx->table_meta->schema;
    Error error = analyze_select_columns(select.columns, schema, select_plan);
    if (!error_is_ok(error))
        return error;

    error = analyze_where_conditions(&select.where, schema, &select_plan->where);
    if (!error_is_ok(error))
        return error;

    select_plan->ignore_index = select.ignore_index;

    return error_ok();
}

Error analyze_insert(QueryContext* query_ctx)
{
    InsertStmt insert = query_ctx->parse_result->stmt.insert;
    InsertExecPlan* plan_insert = &query_ctx->insert;

    TableName table_name;
    sv_to_cstr(insert.table, table_name, sizeof(TableName));
    TableMeta* table_meta = database_table_by_name(query_ctx->app_ctx->database, table_name);
    if (table_meta == nullptr)
        return error_fmt(ERR_ANALYZER, "Table '%s' not exist", table_name);

    query_ctx->table_meta = table_meta;
    AssignmentArray* assignments = &insert.assignments;

    if (assignments->size > table_meta->schema.column_count)
        return error_fmt(ERR_ANALYZER, "Too many assignments for table '%s'", table_name);

    bool assigned_columns[MAX_COLUMNS_PER_TABLE] = {};
    for (uint8_t i = 0; i < (uint8_t)assignments->size; i++) {
        uint8_t column_index;
        if (!find_column_index(&table_meta->schema, assignments->data[i].column, &column_index))
            return error_fmt(ERR_ANALYZER, "Column '%.*s' is not present in table '%s'",
                             (int)assignments->data[i].column.len, assignments->data[i].column.ptr, table_name);

        RawValue* value = &assignments->data[i].value;
        Column* column = &table_meta->schema.columns[column_index];

        auto convert_result = convert_value(value->type, value->value, &plan_insert->values[column_index]);
        if (convert_result != CONVERT_OK)
            return error_fmt(ERR_ANALYZER, "Invalid value for column '%s': %s",
                             column->column_name, convert_result_msg(convert_result));

        auto validate_result = resolved_type_validate(&column->resolved_type, &plan_insert->values[column_index]);
        if (validate_result != VALIDATE_OK)
            return error_fmt(ERR_ANALYZER, "Invalid value for column '%s': %s",
                             column->column_name, validate_type_result_msg(validate_result));

        assigned_columns[column_index] = true;
    }

    for (uint8_t i = 0; i < table_meta->schema.column_count; i++) {
        if (assigned_columns[i])
            continue;

        if (!table_meta->schema.columns[i].has_default)
            return error_fmt(ERR_ANALYZER, "Column '%s' have no default value", table_meta->schema.columns[i].column_name);

        plan_insert->values[i] = table_meta->schema.columns[i].default_value;
    }

    return error_ok();
}

Error analyze_delete(QueryContext* query_ctx)
{
    DeleteStmt delete = query_ctx->parse_result->stmt.delete;
    DeleteExecPlan* delete_plan = &query_ctx->delete;

    TableName table_name;
    sv_to_cstr(delete.table, table_name, sizeof(TableName));
    TableMeta* table_meta = database_table_by_name(query_ctx->app_ctx->database, table_name);
    if (table_meta == nullptr)
        return error_fmt(ERR_ANALYZER, "Table '%s' not exist", table_name);

    query_ctx->table_meta = table_meta;

    return analyze_where_conditions(&delete.where, &table_meta->schema, &delete_plan->where);
}

Error analyze_update(QueryContext* query_ctx)
{
    UpdateStmt update = query_ctx->parse_result->stmt.update;
    UpdateExecPlan* update_plan = &query_ctx->update;
    AssignmentArray* assignments = &update.assignments;

    TableName table_name;
    sv_to_cstr(update.table, table_name, sizeof(TableName));
    TableMeta* table_meta = database_table_by_name(query_ctx->app_ctx->database, table_name);
    if (table_meta == nullptr)
        return error_fmt(ERR_ANALYZER, "Table '%s' not exist", table_name);
    query_ctx->table_meta = table_meta;

    for (uint8_t i = 0; i < (uint8_t)assignments->size; i++) {
        uint8_t column_index;
        if (!find_column_index(&table_meta->schema, assignments->data[i].column, &column_index))
            return error_fmt(ERR_ANALYZER, "Column '%.*s' is not present in table '%s'",
                             (int)assignments->data[i].column.len, assignments->data[i].column.ptr, table_name);

        update_plan->column_positions[i] = column_index;

        auto result = convert_value(assignments->data[i].value.type, assignments->data[i].value.value, &update_plan->values[i]);
        if (result != CONVERT_OK)
            return error_fmt(ERR_ANALYZER, "Invalid value for column '%.*s': %s",
                             (int)assignments->data[i].column.len,
                             assignments->data[i].column.ptr,
                             convert_result_msg(result));
    }
    update_plan->value_count = assignments->size;

    return analyze_where_conditions(&update.where, &table_meta->schema, &update_plan->where);
}