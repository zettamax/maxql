#pragma once

#include <maxql/engine/dml/types.h>
#include <maxql/engine/ddl/types.h>
#include <maxql/parser/parser.h>
#include <maxql/context/context.h>
#include <maxql/core/error.h>

typedef struct {
    AppContext* app_ctx;
    ParseResult* parse_result;
    TableMeta* table_meta;
    union {
        CreateTablePlan create_table;
        DropTablePlan drop_table;
        CreateIndexPlan create_index;
        DropIndexPlan drop_index;
        TruncateTablePlan truncate_table;

        SelectExecPlan select;
        DeleteExecPlan delete;
        InsertExecPlan insert;
        UpdateExecPlan update;
    };
} QueryContext;

typedef Error (*Stage)(QueryContext*);

typedef struct {
    Stage* stages;
    uint8_t count;
} PipelineDef;

Error engine_run(const char* input, AppContext* app_ctx);
