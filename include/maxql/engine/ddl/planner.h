#pragma once

#include "maxql/engine/engine.h"

#include <maxql/core/error.h>

Error plan_create_table(QueryContext* query_ctx);
Error plan_drop_table(QueryContext* query_ctx);
Error plan_create_index(QueryContext* query_ctx);
Error plan_drop_index(QueryContext* query_ctx);
Error plan_truncate_table(QueryContext* query_ctx);
