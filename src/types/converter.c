#include <maxql/types/converter.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

ConvertResult convert_int(StrView int_literal, DataValue* data_value)
{
    char buf[16];
    size_t len = int_literal.len < sizeof(buf) - 1 ? int_literal.len : sizeof(buf) - 1;
    memcpy(buf, int_literal.ptr, len);
    buf[len] = '\0';
    char* end;
    errno = 0;
    long long long_value = strtoll(buf, &end, 10);

    if (errno == ERANGE || long_value < INT32_MIN || long_value > INT32_MAX)
        return CONVERT_OUT_OF_RANGE;

    if (end[0] != '\0')
        return CONVERT_NOT_A_NUMBER;

    *data_value = (DataValue){VALUE_INT, .val.int32 = (int32_t) long_value};

    return CONVERT_OK;
}

ConvertResult convert_float(StrView float_literal, DataValue* data_value)
{
    char buf[32];
    size_t len = float_literal.len < sizeof(buf) - 1 ? float_literal.len : sizeof(buf) - 1;
    memcpy(buf, float_literal.ptr, len);
    buf[len] = '\0';
    char* end;
    errno = 0;
    float float_value = strtof(buf, &end);

    if (errno == ERANGE)
        return CONVERT_OUT_OF_RANGE;

    if (end[0] != '\0')
        return CONVERT_NOT_A_NUMBER;

    *data_value = (DataValue){VALUE_FLOAT, .val.float32 = float_value};

    return CONVERT_OK;
}

ConvertResult convert_number(StrView number_literal, DataValue* data_value)
{
    if (sv_contains(number_literal, '.'))
        return convert_float(number_literal, data_value);

    return convert_int(number_literal, data_value);
}

ConvertResult convert_string(StrView string_literal, DataValue* data_value)
{
    if (string_literal.len > MAX_VARCHAR_VALUE_SIZE)
        return CONVERT_TOO_LONG;

    VarcharValue string_value;
    sv_to_cstr(string_literal, string_value, sizeof(VarcharValue));
    COPY_STRING(data_value->val.string, string_value);

    data_value->type = VALUE_STRING;
    return CONVERT_OK;
}

ConvertResult convert_hex(StrView hex_literal, DataValue* data_value)
{
    if (hex_literal.len > MAX_BINARY_VALUE_SIZE * 2)
        return CONVERT_TOO_LONG;

    size_t hex_idx = 0;
    size_t byte_idx = 0;
    char* end;
    if (hex_literal.len % 2 == 1) {
        hex_idx = 1;
        char byte[3] = {'0', hex_literal.ptr[0], '\0'};
        data_value->val.bin.bytes[byte_idx++] = (uint8_t)strtol(byte, &end, 16);
        if (end[0] != '\0')
            return CONVERT_BAD_HEX;
    }
    for (; hex_idx < hex_literal.len; hex_idx += 2) {
        char byte[3] = {hex_literal.ptr[hex_idx], hex_literal.ptr[hex_idx + 1], '\0'};
        data_value->val.bin.bytes[byte_idx++] = (uint8_t)strtol(byte, &end, 16);
        if (end[0] != '\0')
            return CONVERT_BAD_HEX;
    }
    data_value->val.bin.len = byte_idx;
    data_value->type = VALUE_BINARY;

    return CONVERT_OK;
}

ConvertResult convert_value(LiteralType type, StrView text, DataValue* data_value)
{
    switch (type) {
        case LIT_NUMBER: return convert_number(text, data_value);
        case LIT_STRING: return convert_string(text, data_value);
        case LIT_HEX: return convert_hex(text, data_value);
        default: return CONVERT_UNKNOWN_LITERAL;
    }
}

const char* convert_result_msg(ConvertResult result)
{
    switch (result) {
        case CONVERT_NOT_A_NUMBER: return "not a valid number";
        case CONVERT_OUT_OF_RANGE: return "value out of range";
        case CONVERT_TOO_LONG: return "value too long";
        case CONVERT_BAD_HEX: return "invalid hex literal";
        case CONVERT_UNKNOWN_LITERAL: return "unknown literal type";
        default: return "conversion error";
    }
}
