#pragma once

#include <maxql/core/error.h>

char* safe_strdup(const char* buf);
Error safe_malloc(void** out, size_t size);
