#include <maxql/scan/table_scan.h>

#include <stdio.h>
#include <maxql/storage/storage.h>

typedef struct {
    FILE* data_file;
    size_t file_size;
    TableSchema* schema;
} TableScanState;

static TableScanState* init_table_scan_state(TableSchema* schema)
{
    TableScanState* state = calloc(1, sizeof(TableScanState));
    state->data_file = storage_open(schema->table_name);
    state->schema = schema;

    fseeko(state->data_file, 0, SEEK_END);
    state->file_size = ftello(state->data_file);
    fseeko(state->data_file, 0, SEEK_SET);

    return state;
}

void table_scan_close(RowSource* row_source)
{
    TableScanState* state = row_source->state;
    fclose(state->data_file);

    free(row_source->state);
    free(row_source);
}

bool table_scan_next(RowSource* self, StoredRow* out)
{
    memset(out, 0, sizeof(StoredRow));
    TableScanState* state = self->state;

    while (true) {
        off_t offset = ftello(state->data_file);
        if (offset < 0){
            break;
        }

        FileOffset file_offset = (FileOffset)offset;
        if (file_offset >= state->file_size) {
            break;
        }

        if (storage_read_current_row(state->data_file, state->schema, &out->row)) {
            out->offset = offset;
            return true;
        }
    }

    return false;
}

RowSource* table_scan_create(TableSchema* schema)
{
    RowSource* row_source = calloc(1, sizeof(RowSource));
    row_source->close = table_scan_close;
    row_source->next = table_scan_next;
    row_source->state = init_table_scan_state(schema);

    return row_source;
}
