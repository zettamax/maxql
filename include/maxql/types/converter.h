#pragma once

#include <maxql/types.h>
#include <maxql/core/strview.h>

typedef enum {
    CONVERT_OK,
    CONVERT_UNKNOWN_LITERAL,
    CONVERT_NOT_A_NUMBER,
    CONVERT_OUT_OF_RANGE,
    CONVERT_TOO_LONG,
    CONVERT_BAD_HEX,
} ConvertResult;

ConvertResult convert_int(StrView, DataValue*);
ConvertResult convert_value(LiteralType type, StrView text, DataValue*);
const char* convert_result_msg(ConvertResult result);
