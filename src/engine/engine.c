#include "maxql/engine/dal/executor.h"
#include "maxql/engine/ddl/executor.h"
#include "maxql/engine/ddl/planner.h"
#include "maxql/engine/ddl/validator.h"
#include "maxql/engine/dml/analyzer.h"
#include "maxql/engine/dml/executor.h"
#include "maxql/engine/dml/planner.h"
#include "maxql/engine/dml/validator.h"

#include <maxql/engine/engine.h>
#include <maxql/core/error.h>
#include <maxql/parser/parser.h>
#include <maxql/context/context.h>
#include <maxql/core/strview.h>

#define STAGES(...) \
    .stages = (Stage[]){__VA_ARGS__}, \
    .count = sizeof((Stage[]){__VA_ARGS__}) / sizeof(Stage)

static PipelineDef pipeline_defs[] = {
    [QUERY_CREATE_TABLE] = {STAGES(
        validate_create_table,
        plan_create_table,
        execute_create_table,
    )},
    [QUERY_DROP_TABLE] = {STAGES(
        validate_drop_table,
        plan_drop_table,
        execute_drop_table,
    )},
    [QUERY_CREATE_INDEX] = {STAGES(
        validate_create_index,
        plan_create_index,
        execute_create_index,
    )},
    [QUERY_DROP_INDEX] = {STAGES(
        validate_drop_index,
        plan_drop_index,
        execute_drop_index,
    )},
    [QUERY_TRUNCATE_TABLE] = {STAGES(
        validate_truncate_table,
        plan_truncate_table,
        execute_truncate_table,
    )},

    [QUERY_SHOW_TABLES] = {STAGES(
        execute_show_tables,
    )},
    [QUERY_SHOW_TABLE] = {STAGES(
        execute_show_table,
    )},
    [QUERY_SHOW_INDEX] = {STAGES(
        execute_show_index,
    )},
    [QUERY_SHOW_INDEX_TYPES] = {STAGES(
        execute_show_index_types,
    )},

    [QUERY_INSERT] = {STAGES(
        validate_insert,
        analyze_insert,
        execute_insert,
    )},
    [QUERY_SELECT] = {STAGES(
        validate_select,
        analyze_select,
        plan_select,
        execute_select,
    )},
    [QUERY_DELETE] = {STAGES(
        validate_delete,
        analyze_delete,
        plan_delete,
        execute_delete,
    )},
    [QUERY_UPDATE] = {STAGES(
        validate_update,
        analyze_update,
        plan_update,
        execute_update,
    )},
};

Error parse_input(const char* input, ParseResult* parse_result)
{
    Parser parser = parser_init(sv_from_cstr(input));
    *parse_result = parse_query(&parser);
    return parser.error;
}

Error pipeline_run(PipelineDef* pipeline_def, QueryContext* query_ctx)
{
    if (pipeline_def->count == 0 || !pipeline_def->stages)
        return error_fmt(ERR_INTERNAL, "No pipeline defined for query type");

    for (uint8_t i = 0; i < pipeline_def->count; i++) {
        Error error = pipeline_def->stages[i](query_ctx);
        if (!error_is_ok(error))
            return error;
    }

    return error_ok();
}

Error engine_run(const char* input, AppContext* app_ctx)
{
    QueryContext query_ctx = {.app_ctx = app_ctx};

    ParseResult parse_result = {};
    Error error = parse_input(input, &parse_result);
    if (!error_is_ok(error))
        goto cleanup;
    query_ctx.parse_result = &parse_result;

    PipelineDef* pipeline_def = &pipeline_defs[query_ctx.parse_result->type];
    error = pipeline_run(pipeline_def, &query_ctx);

cleanup:
    parser_result_free(&parse_result);

    return error;
}
