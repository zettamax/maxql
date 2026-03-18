#pragma once

#include <maxql/types.h>

int compare_value(DataValue* a, DataValue* b);
bool compare_matches_op(ConditionOp op, int cmp_result);
