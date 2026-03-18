#include <maxql/index/index_set.h>
#include <maxql/types/comparator.h>
#include <maxql/types/presentation.h>
#include <maxql/engine/dml/executor.h>
#include <maxql/scan/index_scan.h>
#include <maxql/scan/row_source.h>
#include <maxql/scan/table_scan.h>
#include <maxql/storage/storage.h>
#include <maxql/view/output.h>
#include <maxql/view/printer.h>

static bool matches_where(StoredRow* row, WhereFilter* where)
{
    for (size_t i = 0; i < where->predicate_count; i++) {
        size_t column_index = where->predicates[i].column_index;
        int cmp_result = compare_value(&row->row.values[column_index], &where->predicates[i].value);
        bool is_matched = compare_matches_op(where->predicates[i].op, cmp_result);
        if (!is_matched)
            return false;
    }

    return true;
}

static void print_row(Column* columns, SelectExecPlan* select_plan, TablePrinter* printer, DataRow* row)
{
    size_t max_buf = 0;
    for (size_t i = 0; i < select_plan->column_count; i++) {
        size_t col_idx = select_plan->query_column_order[i];
        const ResolvedType* rt = &columns[col_idx].resolved_type;
        size_t w = type_presentation(rt->type_info->column_type)->display_width(rt->byte_len) + 1;
        if (w > max_buf) max_buf = w;
    }

    char bufs[select_plan->column_count][max_buf];
    const char* cells[select_plan->column_count];
    for (size_t i = 0; i < select_plan->column_count; i++) {
        size_t column_index = select_plan->query_column_order[i];
        const TypeInfo* type_info = columns[column_index].resolved_type.type_info;
        type_presentation(type_info->column_type)->format_display(&row->values[column_index], bufs[i], max_buf);
        cells[i] = bufs[i];
    }
    printer->add_row(printer, cells);
}

static TablePrinter create_select_printer(SelectExecPlan* select_plan, TableSchema* table_schema)
{
    Column* columns = table_schema->columns;
    uint8_t column_count = select_plan->column_count;
    bool buffered = select_plan->index_decision.use_index;

    const char* names[column_count];
    uint16_t widths[column_count];
    bool right_aligns[column_count];
    for (size_t i = 0; i < column_count; i++) {
        uint8_t column_idx = select_plan->query_column_order[i];
        Column* column = &columns[column_idx];
        ResolvedType* resolved_type = &column->resolved_type;
        const TypeInfo* type_info = resolved_type->type_info;

        names[i] = column->column_name;
        const ColumnPresentation* p = type_presentation(type_info->column_type);
        widths[i] = p->display_width(resolved_type->byte_len);
        right_aligns[i] = p->align_right;
    }

    if (buffered)
        return printer_buffered(names, right_aligns, column_count);

    return printer_stream(names, widths, right_aligns, column_count);
}

RowSource* create_row_source(IndexDecision* index_decision, TableSchema* table_schema)
{
    return index_decision->use_index
               ? index_scan_create(table_schema, index_decision)
               : table_scan_create(table_schema);
}

Error execute_select(QueryContext* query_ctx)
{
    SelectExecPlan* select_plan = &query_ctx->select;
    IndexDecision* index_decision = &select_plan->index_decision;
    TableSchema* table_schema = &query_ctx->table_meta->schema;

    TablePrinter printer = create_select_printer(select_plan, table_schema);
    RowSource* row_source = create_row_source(index_decision, table_schema);

    StoredRow stored_row = {};
    while (row_source->next(row_source, &stored_row))
        if (matches_where(&stored_row, &select_plan->where))
            print_row(table_schema->columns, select_plan, &printer, &stored_row.row);
    printer.finish(&printer);
    row_source->close(row_source);

    print_index_info(index_decision->use_index,
                     index_decision->use_index ? index_decision->index_def->name : nullptr);

    return error_ok();
}

Error execute_insert(QueryContext* query_ctx)
{
    Error error;
    TableMeta* table_meta = query_ctx->table_meta;
    InsertExecPlan* insert_plan = &query_ctx->insert;

    FILE* data_file = nullptr;
    data_file = storage_open(table_meta->name);

    IndexSet index_set = {};
    index_set_load(table_meta, &index_set);

    IndexDefinition* dup_index_def;
    bool found = index_set_has_duplicate(&index_set, insert_plan->values, &dup_index_def);
    if (found) {
        error = error_fmt(ERR_EXECUTOR, "Duplicate value(s) for unique index '%s'", dup_index_def->name);
        goto cleanup;
    }

    FileOffset offset;
    error = storage_insert_row(data_file, insert_plan->values, &table_meta->schema, &offset);
    if (!error_is_ok(error))
        goto cleanup;

    index_set_insert(&index_set, insert_plan->values, offset);
    index_set_save(&index_set);

cleanup:
    index_set_free(&index_set);

    if (data_file)
        storage_close(data_file);

    if (error_is_ok(error))
        print_affected_rows(1);

    return error;
}

Error execute_delete(QueryContext* query_ctx)
{
    TableMeta* table_meta = query_ctx->table_meta;
    TableSchema* schema = &table_meta->schema;
    IndexDecision* index_decision = &query_ctx->delete.index_decision;

    IndexSet index_set = {};
    index_set_load(table_meta, &index_set);

    RowSource* row_source = create_row_source(index_decision, schema);

    StoredRow stored_row = {};
    OffsetArray offsets;
    da_init(&offsets);
    while (row_source->next(row_source, &stored_row))
        if (matches_where(&stored_row, &query_ctx->delete.where)) {
            da_push(&offsets, stored_row.offset);
            index_set_delete(&index_set, stored_row.row.values, stored_row.offset);
        }
    row_source->close(row_source);

    Error error = error_ok();
    if (offsets.size) {
        FILE* data_file = storage_open(table_meta->name);
        error = storage_delete_rows(data_file, offsets);
        storage_close(data_file);
        if (!error_is_ok(error))
            goto cleanup;

        index_set_save(&index_set);
    }

    print_affected_rows(offsets.size);

cleanup:
    da_free(&offsets);
    index_set_free(&index_set);

    return error;
}

bool fill_update_row(UpdatePair* update_pair, UpdateExecPlan* update)
{
    memcpy(&update_pair->updated, &update_pair->original.row, sizeof(update_pair->updated));

    bool has_changes = false;
    for (int i = 0; i < update->value_count; i++) {
        uint8_t column_pos = update->column_positions[i];
        int result = compare_value(&update->values[i], &update_pair->updated.values[column_pos]);
        if (result != 0) {
            update_pair->updated.values[column_pos] = update->values[i];
            has_changes = true;
        }
    }

    return has_changes;
}

Error index_batch_has_duplicates(UpdatePairArray* pairs, IndexDefinition* index_def)
{
    for (size_t i = 0; i < pairs->size; i++) {
        DataValue* pair_a = pairs->data[i].updated.values;

        for (size_t j = i + 1; j < pairs->size; j++) {
            DataValue* pair_b = pairs->data[j].updated.values;

            bool duplicate = true;
            for (uint8_t k = 0; k < index_def->column_count; k++) {
                uint8_t column_pos = index_def->column_pos[k];
                if (compare_value(&pair_a[column_pos], &pair_b[column_pos]) != 0) {
                    duplicate = false;
                    break;
                }
            }

            if (duplicate) {
                return error_fmt(ERR_INDEX, "Update contains duplicates for unique index '%s'", index_def->name);
            }
        }
    }

    return error_ok();
}

Error batch_check_unique_keys(UpdatePairArray* pairs, IndexSet* index_set)
{
    for (uint8_t i = 0; i < index_set->count; i++) {
        IndexDefinition* index_def = index_set->index_defs[i];

        Error error = index_batch_has_duplicates(pairs, index_def);
        if (!error_is_ok(error))
            return error;
    }

    return error_ok();
}

Error execute_update(QueryContext* query_ctx)
{
    Error error = error_ok();

    TableMeta* table_meta = query_ctx->table_meta;
    TableSchema* table_schema = &table_meta->schema;
    UpdateExecPlan* update = &query_ctx->update;
    IndexDecision* index_decision = &update->index_decision;
    FILE* data_file = nullptr;

    IndexSet index_set = {};
    index_set_load(table_meta, &index_set);

    RowSource* row_source = create_row_source(index_decision, table_schema);

    UpdatePair update_pair = {};
    UpdatePairArray update_pairs;
    da_init(&update_pairs);
    while (row_source->next(row_source, &update_pair.original))
        if (matches_where(&update_pair.original, &update->where)) {
            if (fill_update_row(&update_pair, update))
                da_push(&update_pairs, update_pair);
        }
    row_source->close(row_source);

    for (size_t i = 0; i < update_pairs.size; i++) {
        DataValue* values = update_pairs.data[i].updated.values;
        size_t offset = update_pairs.data[i].original.offset;

        IndexDefinition* conflict_index_def;
        bool found = index_set_has_conflict(&index_set, values, offset, &conflict_index_def);
        if (found) {
            error = error_fmt(ERR_EXECUTOR, "Duplicate value(s) for unique index '%s'", conflict_index_def->name);
            goto cleanup;
        }
    }

    error = batch_check_unique_keys(&update_pairs, &index_set);
    if (!error_is_ok(error))
        goto cleanup;

    data_file = storage_open(table_meta->name);
    for (size_t i = 0; i < update_pairs.size; i++) {
        storage_delete_row(data_file, update_pairs.data[i].original.offset);
        FileOffset offset;
        error = storage_insert_row(data_file,
                                   update_pairs.data[i].updated.values,
                                   &table_meta->schema,
                                   &offset);
        if (!error_is_ok(error))
            goto cleanup;

        index_set_delete(&index_set,
                         update_pairs.data[i].original.row.values,
                         update_pairs.data[i].original.offset);

        index_set_insert(&index_set,
                         update_pairs.data[i].updated.values,
                         offset);
    }

    index_set_save(&index_set);

    print_affected_rows(update_pairs.size);

cleanup:
    if (data_file)
        fclose(data_file);
    if (update_pairs.data != nullptr)
        da_free(&update_pairs);
    index_set_free(&index_set);

    return error;
}