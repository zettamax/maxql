#include <maxql/types/comparator.h>
#include <assert.h>
#include <string.h>
#include <maxql/types.h>

int compare_value(DataValue* a, DataValue* b)
{
    switch (a->type) {
        case VALUE_INT:
            return (a->val.int32 > b->val.int32) - (a->val.int32 < b->val.int32);

        case VALUE_STRING:
            return strcmp(a->val.string, b->val.string);

        case VALUE_BINARY: {
            int len_cmp = (a->val.bin.len > b->val.bin.len) - (a->val.bin.len < b->val.bin.len);
            if (len_cmp != 0)
                return len_cmp;
            return memcmp(a->val.bin.bytes, b->val.bin.bytes, a->val.bin.len);
        }

        case VALUE_FLOAT:
            return (a->val.float32 > b->val.float32) - (a->val.float32 < b->val.float32);

        default: assert(0 && "unknown value type");
    }
}

bool compare_matches_op(ConditionOp op, int cmp_result)
{
    switch (op) {
        case OP_EQUALS: return cmp_result == 0;
        case OP_NOT_EQUALS: return cmp_result != 0;
        case OP_LESS: return cmp_result < 0;
        case OP_LESS_OR_EQUAL: return cmp_result <= 0;
        case OP_GREATER: return  cmp_result > 0;
        case OP_GREAT_OR_EQUAL: return  cmp_result >= 0;
        default: assert(0 && "Unknown operator");
    }
}
