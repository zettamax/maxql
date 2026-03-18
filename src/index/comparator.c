#include <maxql/index/comparator.h>

int compound_compare(const void* a, const void* b, const void* ctx)
{
    const uint8_t* ka = a;
    const uint8_t* kb = b;
    const ComparatorCompoundCtx* context = ctx;
    uint16_t offset = 0;
    for (uint8_t i = 0; i < context->field_count; i++)
    {
        int result = context->field_comparators[i](ka + offset, kb + offset, &context->field_sizes[i]);
        if (result != 0)
            return result;
        offset += context->field_sizes[i];
    }

    return 0;
}
