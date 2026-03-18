#pragma once

#include <maxql/types.h>

typedef struct RowSource RowSource;

typedef struct RowSource {
    void (*close)(RowSource*);
    bool (*next)(RowSource*, StoredRow*);
    void* state;
} RowSource;
