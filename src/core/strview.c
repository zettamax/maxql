#include <string.h>
#include <maxql/core/strview.h>

StrView sv_from_parts(const char* ptr, size_t len)
{
    return (StrView){ptr, len};
}

StrView sv_from_cstr(const char* ptr)
{
    return (StrView){ptr, strlen(ptr)};
}

bool sv_equal(StrView a, StrView b)
{
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}

bool sv_equal_cstr(StrView str_view, const char* cstr)
{
    return sv_equal(str_view, sv_from_cstr(cstr));
}

bool sv_contains(StrView str_view, char c)
{
    return memchr(str_view.ptr, c, str_view.len) != nullptr;
}

void sv_to_cstr(StrView str_view, char* out, size_t out_size)
{
    if (out_size == 0) return;
    size_t len = str_view.len < out_size - 1 ? str_view.len : out_size - 1;
    memcpy(out, str_view.ptr, len);
    out[len] = '\0';
}
