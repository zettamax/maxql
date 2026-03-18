#pragma once

#include "maxql/engine/engine.h"

#include <maxql/core/error.h>

Error validate_select(QueryContext* query_ctx);
Error validate_insert(QueryContext* query_ctx);
Error validate_delete(QueryContext* query_ctx);
Error validate_update(QueryContext* query_ctx);
