#pragma once

#include "maxql/engine/engine.h"

#include <maxql/core/error.h>

Error validate_create_table(QueryContext* query_ctx);
Error validate_drop_table(QueryContext* query_ctx);
Error validate_create_index(QueryContext* query_ctx);
Error validate_drop_index(QueryContext* query_ctx);
Error validate_truncate_table(QueryContext* query_ctx);
