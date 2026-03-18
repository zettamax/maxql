#include "maxql/schema/schema_io.h"

#include <maxql/context/context.h>
#include <maxql/index/index.h>

AppContext g_app_ctx;

void app_context_init(AppContext* app_ctx)
{
    index_register_builtin_types();
    const Error error = database_load(schema_load_table, &app_ctx->database);
    if (!error_is_ok(error))
        handle_fatal_error(error);

    // TODO: remove after all schemas migrated to new format
    for (size_t i = 0; i < app_ctx->database->tables.size; i++) {
        TableMeta* tm = &app_ctx->database->tables.data[i];
        schema_save_table(&tm->schema, &tm->index_meta);
    }
}

void app_context_free(AppContext* app_ctx)
{
    if (app_ctx->database) {
        database_free(app_ctx->database);
        app_ctx->database = nullptr;
    }
    index_unregister_all();
}
