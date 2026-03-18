#pragma once

#include <maxql/limits.h>
#include <maxql/scan/scan_types.h>
#include <maxql/core/dyn_array.h>
#include <stdint.h>

typedef struct {
    uint8_t column_index;
    DataValue value;
    ConditionOp op;
} WherePredicate;

typedef struct {
    WherePredicate predicates[MAX_WHERE_CONDITIONS];
    uint8_t predicate_count;
} WhereFilter;

typedef struct {
    uint8_t column_count;
    uint8_t query_column_order[MAX_COLUMNS_IN_SELECT];
    WhereFilter where;
    IndexDecision index_decision;
    bool ignore_index;
} SelectExecPlan;

typedef struct {
    WhereFilter where;
    IndexDecision index_decision;
} DeleteExecPlan;

typedef struct {
    DataValue values[MAX_COLUMNS_PER_TABLE];
} InsertExecPlan;

typedef struct {
    DataValue values[MAX_COLUMNS_PER_TABLE];
    uint8_t column_positions[MAX_COLUMNS_PER_TABLE];
    uint8_t value_count;
    WhereFilter where;
    IndexDecision index_decision;
} UpdateExecPlan;

typedef struct {
    StoredRow original;
    DataRow updated;
} UpdatePair;

typedef DynArray(UpdatePair) UpdatePairArray;
