#pragma once

#include <stddef.h>
#include <stdlib.h>

#define DynArray(T) \
    struct \
    { \
        T *data; \
        size_t size; \
        size_t capacity; \
    }

#define da_init(a) \
    do { \
        (a)->data = nullptr; \
        (a)->size = 0; \
        (a)->capacity = 0; \
    } while (0)

#define da_free(a) \
    do { \
        free((a)->data); \
        da_init(a); \
    } while (0)

void* da_grow(void* data, size_t* capacity, size_t elem_size);

#define da_remove(a, index) \
    do { \
        if ((index) < (a)->size) { \
            memmove( \
                &(a)->data[index], \
                &(a)->data[(index) + 1], \
                ((a)->size - (index) - 1) * sizeof(*(a)->data) \
            ); \
            (a)->size--; \
        } \
    } while (0)

#define da_push(a, value) \
    do { \
        if ((a)->size == (a)->capacity) \
        { \
            (a)->data = da_grow((a)->data, &(a)->capacity, sizeof(*(a)->data)); \
        } \
        (a)->data[(a)->size++] = (value); \
    } while (0)
