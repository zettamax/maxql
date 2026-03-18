#include <maxql/index/index.h>
#include <maxql/scan/index_scan.h>
#include <stdio.h>
#include <maxql/types.h>
#include <maxql/storage/storage.h>
#include <maxql/schema/schema.h>

typedef struct {
    IndexType index_type;
    TableSchema* schema;
    IndexCondition index_conditions[MAX_INDEX_COLUMN_COUNT];
    size_t condition_count;
    void* index_config;
    void* index_key;
    FILE* index_file;
    FILE* data_file;

    size_t offset;
    bool has_result;
    bool consumed;
} IndexScanState;

IndexScanState* init_index_scan_state(TableSchema* schema, IndexDecision* index_decision)
{
    IndexScanState* state = calloc(1, sizeof(IndexScanState));

    state->schema = schema;
    state->index_type = index_decision->index_def->type;
    state->condition_count = index_decision->condition_count;
    memcpy(state->index_conditions,
           index_decision->index_conditions,
           sizeof(IndexCondition) * state->condition_count);
    state->index_config = index_build_config(index_decision->index_def);
    index_open(schema->table_name, index_decision->index_def, (void**)&state->index_file);
    state->index_key = index_build_key(index_decision->index_def,
                                       index_decision->values,
                                       index_decision->condition_count);
    state->has_result = index_search_stream(state->index_config,
                                            state->index_file,
                                            state->index_key,
                                            index_decision->index_def->type,
                                            &state->offset);
    state->data_file = storage_open(schema->table_name);

    return state;
}

void index_scan_close(RowSource* row_source)
{
    IndexScanState* state = row_source->state;
    fclose(state->index_file);
    fclose(state->data_file);
    index_free_config(state->index_config, state->index_type);
    free(state->index_key);
    free(row_source->state);
    free(row_source);
}

bool index_scan_next(RowSource* self, StoredRow* out)
{
    memset(out, 0, sizeof(StoredRow));
    IndexScanState* state = self->state;

    if (state->consumed || !state->has_result)
        return false;

    state->consumed = true;

    out->offset = state->offset;
    bool found = storage_load_row(state->data_file, state->schema, state->offset, &out->row);

    return found;
}

RowSource* index_scan_create(TableSchema* schema, IndexDecision* selected_index)
{
    RowSource* row_source = calloc(1, sizeof(RowSource));
    row_source->close = index_scan_close;
    row_source->next = index_scan_next;
    row_source->state = init_index_scan_state(schema, selected_index);

    return row_source;
}
