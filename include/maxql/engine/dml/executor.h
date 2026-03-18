#pragma once

#include "maxql/engine/engine.h"
#include <maxql/core/error.h>

Error execute_select(QueryContext* query_ctx);
Error execute_insert(QueryContext* query_ctx);
Error execute_delete(QueryContext* query_ctx);
Error execute_update(QueryContext* query_ctx);
