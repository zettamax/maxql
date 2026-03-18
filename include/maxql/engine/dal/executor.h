#pragma once

#include <maxql/core/error.h>
#include <maxql/engine/engine.h>

Error execute_show_tables(QueryContext* query_ctx);
Error execute_show_table(QueryContext* query_ctx);
Error execute_show_index(QueryContext* query_ctx);
Error execute_show_index_types(QueryContext* query_ctx);