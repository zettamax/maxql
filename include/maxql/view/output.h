#pragma once

#include <maxql/types.h>
#include <maxql/core/error.h>
#include <stddef.h>

void print_error(Error error);
void print_time(TimeDelta td);
void print_index_info(bool used, const char* name);
void print_affected_rows(size_t count);
void print_ok(void);
