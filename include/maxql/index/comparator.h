#pragma once

#include <maxql/types/key_compare.h>

typedef struct {
    uint8_t field_count;
    uint8_t field_sizes[MAX_INDEX_COLUMN_COUNT];
    KeyComparator field_comparators[MAX_INDEX_COLUMN_COUNT];
} ComparatorCompoundCtx;

int compound_compare(const void* a, const void* b, const void* ctx);
