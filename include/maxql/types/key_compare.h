#pragma once

#include <maxql/types.h>

typedef int (*KeyComparator)(const void* a, const void* b, const void* ctx);

int compare_int_keys(const void* a, const void* b, const void* ctx);
int compare_string_keys(const void* a, const void* b, const void* ctx);
int compare_binary_keys(const void* a, const void* b, const void* ctx);
int compare_float_keys(const void* a, const void* b, const void* ctx);
