#include <stdio.h>
#include <stdlib.h>
#include <maxql/core/error.h>

void handle_fatal_error(const Error error)
{
    fprintf(stderr, "FATAL: %s\n", error.msg);
    exit(EXIT_FAILURE);
}