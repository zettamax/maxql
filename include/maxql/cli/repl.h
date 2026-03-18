#pragma once

#include <maxql/context/context.h>

typedef void (*ReplExecuteFn)(const char*, AppContext*);

void repl_run(ReplExecuteFn execute_fn, AppContext* app_ctx);