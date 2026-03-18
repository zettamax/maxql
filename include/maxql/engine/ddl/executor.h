#pragma once

#include "maxql/engine/engine.h"

#include <maxql/core/error.h>

Error execute_create_table(QueryContext* query_ctx);
Error execute_drop_table(QueryContext* query_ctx);
Error execute_create_index(QueryContext* query_ctx);
Error execute_drop_index(QueryContext* query_ctx);
Error execute_truncate_table(QueryContext* query_ctx);
