#include "maxql/engine/engine.h"

#include <stdio.h>
#include <maxql/engine/dal/executor.h>
#include <maxql/view/printer.h>
#include <maxql/index/index.h>
#include <maxql/types/presentation.h>

Error execute_show_tables(QueryContext* query_ctx)
{
    Database* database = query_ctx->app_ctx->database;

    const char* names[] = {"Tables"};
    bool aligns[] = {false};
    TablePrinter printer = printer_buffered(names, aligns, 1);

    for (size_t i = 0; i < database->tables.size; i++) {
        const char* cells[] = {database->tables.data[i].name};
        printer.add_row(&printer, cells);
    }
    printer.finish(&printer);

    return error_ok();
}

Error execute_show_table(QueryContext* query_ctx)
{
    ShowTableStmt show_table_stmt = query_ctx->parse_result->stmt.show_table;
    Database* database = query_ctx->app_ctx->database;

    const char* names[] = {"Column", "Type", "Default", "Index",};
    bool aligns[] = {false, false, false, false};
    TablePrinter printer = printer_buffered(names, aligns, 4);

    TableName table_name;
    sv_to_cstr(show_table_stmt.table, table_name, sizeof(table_name));
    TableMeta* table_meta = database_table_by_name(database, table_name);
    if (table_meta == nullptr)
        return error_fmt(ERR_EXECUTOR, "Table '%s' not found", table_name);

    for (size_t i = 0; i < table_meta->schema.column_count; i++) {
        Column column = table_meta->schema.columns[i];

        bool col_has_index = false;
        for (size_t j = 0; j < table_meta->index_meta.index_count; j++)
            if (table_meta->index_meta.index_defs[j].column_pos[0] == i) {
                col_has_index = true;
                break;
            }

        const ColumnPresentation* p = type_presentation(column.resolved_type.type_info->column_type);
        size_t sz = p->display_width(column.resolved_type.byte_len) + 1;
        char default_buf[sz];
        if (column.has_default)
            p->format_display(&column.default_value, default_buf, sz);

        const char* cells[] = {
            column.column_name,
            column.resolved_type.resolved_type_name,
            column.has_default ? default_buf : "-",
            col_has_index ? "Yes" : "No",
        };
        printer.add_row(&printer, cells);
    }
    printer.finish(&printer);

    return error_ok();
}

Error execute_show_index(QueryContext* query_ctx)
{
    ShowIndexStmt show_index_stmt = query_ctx->parse_result->stmt.show_index;
    Database* database = query_ctx->app_ctx->database;

    const char* names[] = {"Index", "Type", "Unique", "Column 1", "Column 2", "Column 3", "Column 4"};
    bool aligns[] = {false, false, false, false, false, false, false};
    TablePrinter printer = printer_buffered(names, aligns, 7);

    TableName table_name;
    sv_to_cstr(show_index_stmt.table, table_name, sizeof(table_name));
    TableMeta* table_meta = database_table_by_name(database, table_name);
    if (table_meta == nullptr)
        return error_fmt(ERR_EXECUTOR, "Table '%s' not found", table_name);

    for (size_t i = 0; i < table_meta->index_meta.index_count; i++) {
        const char* cells[7] = {
            table_meta->index_meta.index_defs[i].name,
            index_vtable(table_meta->index_meta.index_defs[i].type)->type_name,
            table_meta->index_meta.index_defs[i].is_unique ? "Yes" : "No"
        };
        char col_parts[MAX_INDEX_COLUMN_COUNT][64];
        for (size_t j = 0; j < MAX_INDEX_COLUMN_COUNT; j++) {
            if (j < table_meta->index_meta.index_defs[i].column_count) {
                snprintf(col_parts[j], 64, "%s(%u)",
                    table_meta->index_meta.index_defs[i].columns[j],
                    table_meta->index_meta.index_defs[i].part_len[j]);
                cells[j + 3] = col_parts[j];
            } else {
                cells[j + 3] = "-";
            }

        }

        printer.add_row(&printer, cells);
    }
    printer.finish(&printer);

    return error_ok();
}

Error execute_show_index_types([[maybe_unused]] QueryContext* query_ctx)
{
    const char* names[] = {"Type", "Desc"};
    bool aligns[] = {false, false};
    TablePrinter printer = printer_buffered(names, aligns, 2);

    size_t cursor = 0;
    const IndexVTable* vt;
    while (index_registry_next(&cursor, &vt)) {
        const char* cells[2] = {
            vt->type_name,
            vt->desc,
        };
        printer.add_row(&printer, cells);
    }
    printer.finish(&printer);

    return error_ok();
}