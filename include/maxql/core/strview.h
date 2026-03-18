#pragma once

#include <stddef.h>

typedef struct {
    const char* ptr;
    size_t len;
} StrView;

StrView sv_from_cstr(const char* s);
StrView sv_from_parts(const char* ptr, size_t len);

bool sv_equal(StrView a, StrView b);
bool sv_equal_cstr(StrView a, const char* s);
bool sv_contains(StrView str_view, char c);

void sv_to_cstr(StrView str_view, char* out, size_t out_size);
