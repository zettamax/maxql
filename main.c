#include <errno.h>
#include <math.h>
#include <maxql/view/output.h>
#include <maxql/core/error.h>
#include <maxql/types.h>
#include <stdio.h>
#include <time.h>
#include <maxql/constants.h>
#include <maxql/cli/repl.h>
#include <maxql/engine/engine.h>
#include <maxql/context/context.h>

void maxql_execute(const char* input_buffer, AppContext* app_ctx)
{
    TimeDelta td;
    clock_gettime(CLOCK_MONOTONIC, &td.start);
    Error error = engine_run(input_buffer, app_ctx);
    clock_gettime(CLOCK_MONOTONIC, &td.end);

    if (error_is_fatal(error))
        handle_fatal_error(error);

    if (!error_is_ok(error))
        print_error(error);
    else
        print_time(td);
}

int main()
{
    app_context_init(&g_app_ctx);

    printf("maxql v%s\n", MAXQL_VERSION);
    repl_run(maxql_execute, &g_app_ctx);

    app_context_free(&g_app_ctx);
    printf(BYE_MSG);
    return 0;
}