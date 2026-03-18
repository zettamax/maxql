#pragma once

#include <maxql/database/database.h>

typedef struct {
    Database* database;
} AppContext;

extern AppContext g_app_ctx;

void app_context_init(AppContext* app_ctx);
void app_context_free(AppContext* app_ctx);
