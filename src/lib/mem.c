#include <stdlib.h>
#include <string.h>
#include <maxql/lib/mem.h>
#include <maxql/core/error.h>

char* safe_strdup(const char* buf)
{
    char* dup = strdup(buf);
    if (!dup)
        abort();
    return dup;
}

Error safe_malloc(void** out, const size_t size)
{
    *out = malloc(size);
    if (*out == nullptr)
        return error_fatal_fmt(ERR_MEMORY, "Cannot allocate memory");
    return error_ok();
}
