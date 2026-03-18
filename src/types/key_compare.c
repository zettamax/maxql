#include <maxql/types/key_compare.h>

int compare_int_keys(const void* a, const void* b, [[maybe_unused]] const void* ctx)
{
    int va = *(const int32_t*)a;
    int vb = *(const int32_t*)b;
    return (va > vb) - (va < vb);
}

int compare_string_keys(const void* a, const void* b, const void* ctx)
{
    const uint8_t* length = ctx;
    return strncmp(a, b, *length);
}

int compare_binary_keys(const void* a, const void* b, const void* ctx)
{
    const uint8_t* length = ctx;
    return memcmp(a, b, *length);
}

int compare_float_keys(const void* a, const void* b, [[maybe_unused]] const void* ctx)
{
    float va = *(const float*)a;
    float vb = *(const float*)b;
    return (va > vb) - (va < vb);
}
