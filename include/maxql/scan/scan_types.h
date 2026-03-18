#pragma once

#include <maxql/types.h>
#include <maxql/index/index_types.h>

typedef struct {
    DataValue value;
    ConditionOp op;
} IndexCondition; // todo do i need it

typedef struct {
    IndexDefinition* index_def;
    IndexCondition index_conditions[MAX_WHERE_CONDITIONS];
    DataValue values[MAX_WHERE_CONDITIONS];
    uint8_t column_positions[MAX_WHERE_CONDITIONS];
    uint8_t condition_count;
    bool use_index;
} IndexDecision;
