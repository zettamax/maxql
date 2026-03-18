#include <stddef.h>
#include <stdint.h>
#include <maxql/core/dyn_array.h>

void* da_grow(void* data, size_t* capacity, size_t elem_size)
{
    if (*capacity >= (SIZE_MAX / 2) + 1)
        abort();
    size_t new_capacity = *capacity == 0 ? 4 : *capacity * 2;
    if (new_capacity > SIZE_MAX / elem_size)
        abort();

    void* new_data = realloc(data, elem_size * new_capacity);
    if (!new_data)
        abort();

    *capacity = new_capacity;
    return new_data;
}
