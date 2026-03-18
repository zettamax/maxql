#pragma once

#include "maxql/engine/engine.h"

#include <maxql/core/error.h>

Error plan_select(QueryContext* query_ctx);
Error plan_delete(QueryContext* query_ctx);
Error plan_update(QueryContext* query_ctx);
