#pragma once

#include "maxql/engine/engine.h"

#include <maxql/core/error.h>

Error analyze_select(QueryContext* query_ctx);
Error analyze_insert(QueryContext* query_ctx);
Error analyze_delete(QueryContext* query_ctx);
Error analyze_update(QueryContext* query_ctx);
