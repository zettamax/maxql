#include <maxql/types/mapper.h>
#include <maxql/types/key_compare.h>
#include <stdint.h>
#include <stdio.h>

#include <maxql/types.h>
#include <maxql/limits.h>
#include <string.h>

static TypeValidateResult validate_string(const DataValue* value, uint8_t byte_len)
{
    if (strlen(value->val.string) > byte_len)
        return VALIDATE_VALUE_TOO_LONG;
    return VALIDATE_OK;
}

static TypeValidateResult validate_binary(const DataValue* value, uint8_t byte_len)
{
    if (value->val.bin.len > byte_len)
        return VALIDATE_VALUE_TOO_LONG;
    return VALIDATE_OK;
}

static size_t serialize_int(const DataValue* value, uint8_t byte_len, uint8_t* buf)
{
    memcpy(buf, &value->val.int32, byte_len);
    return byte_len;
}

static size_t deserialize_int(const uint8_t* buf, uint8_t byte_len, DataValue* value)
{
    value->type = VALUE_INT;
    memcpy(&value->val.int32, buf, byte_len);
    return byte_len;
}

static size_t serialize_float(const DataValue* value, uint8_t byte_len, uint8_t* buf)
{
    memcpy(buf, &value->val.float32, byte_len);
    return byte_len;
}

static size_t deserialize_float(const uint8_t* buf, uint8_t byte_len, DataValue* value)
{
    value->type = VALUE_FLOAT;
    memcpy(&value->val.float32, buf, byte_len);
    return byte_len;
}

static size_t serialize_varchar(const DataValue* value, [[maybe_unused]] uint8_t byte_len, uint8_t* buf)
{
    uint8_t len = strlen(value->val.string);
    buf[0] = len;
    memcpy(&buf[1], value->val.string, len);
    return len + 1;
}

static size_t deserialize_varchar(const uint8_t* buf, [[maybe_unused]] uint8_t byte_len, DataValue* value)
{
    value->type = VALUE_STRING;
    uint8_t len = buf[0];
    memcpy(value->val.string, &buf[1], len);
    value->val.string[len] = '\0';
    return len + 1;
}

static size_t serialize_char(const DataValue* value, uint8_t byte_len, uint8_t* buf)
{
    uint8_t len = strlen(value->val.string);
    memcpy(buf, value->val.string, len);
    memset(&buf[len], ' ', byte_len - len);
    return byte_len;
}

static size_t deserialize_char(const uint8_t* buf, uint8_t byte_len, DataValue* value)
{
    value->type = VALUE_STRING;
    memcpy(value->val.string, buf, byte_len);
    char* end = value->val.string + byte_len;
    do {
        *end = '\0';
        end--;
    } while (end >= value->val.string && *end == ' ');
    value->val.string[byte_len] = '\0';
    return byte_len;
}

static size_t serialize_binary(const DataValue* value, uint8_t byte_len, uint8_t* buf)
{
    memcpy(buf, value->val.bin.bytes, value->val.bin.len);
    memset(&buf[value->val.bin.len], 0x00, byte_len - value->val.bin.len);
    return byte_len;
}

static size_t deserialize_binary(const uint8_t* buf, uint8_t byte_len, DataValue* value)
{
    value->type = VALUE_BINARY;
    memcpy(value->val.bin.bytes, buf, byte_len);
    value->val.bin.len = byte_len;
    return byte_len;
}

static const TypeInfo type_mapping[] = {
    [TYPE_INT] = {
        .column_type   = TYPE_INT,
        .value_type    = VALUE_INT,
        .type_name     = "int",
        .expects_len   = false,
        .fixed_size    = sizeof(int32_t),
        .compare       = compare_int_keys,
        .serialize     = serialize_int,
        .deserialize   = deserialize_int,
    },
    [TYPE_VARCHAR] = {
        .column_type   = TYPE_VARCHAR,
        .value_type    = VALUE_STRING,
        .type_name     = "varchar",
        .expects_len   = true,
        .min_len       = 1,
        .max_len       = MAX_VARCHAR_VALUE_SIZE,
        .compare       = compare_string_keys,
        .validate      = validate_string,
        .serialize     = serialize_varchar,
        .deserialize   = deserialize_varchar,
    },
    [TYPE_CHAR] = {
        .column_type   = TYPE_CHAR,
        .value_type    = VALUE_STRING,
        .type_name     = "char",
        .expects_len   = true,
        .min_len       = 1,
        .max_len       = MAX_CHAR_VALUE_SIZE,
        .compare       = compare_string_keys,
        .validate      = validate_string,
        .serialize     = serialize_char,
        .deserialize   = deserialize_char,
    },
    [TYPE_BINARY] = {
        .column_type   = TYPE_BINARY,
        .value_type    = VALUE_BINARY,
        .type_name     = "binary",
        .expects_len   = true,
        .min_len       = 1,
        .max_len       = MAX_BINARY_VALUE_SIZE,
        .compare       = compare_binary_keys,
        .validate      = validate_binary,
        .serialize     = serialize_binary,
        .deserialize   = deserialize_binary,
    },
    [TYPE_FLOAT] = {
        .column_type   = TYPE_FLOAT,
        .value_type    = VALUE_FLOAT,
        .type_name     = "float",
        .expects_len   = false,
        .fixed_size    = sizeof(float),
        .compare       = compare_float_keys,
        .serialize     = serialize_float,
        .deserialize   = deserialize_float,
    },
};

const TypeInfo* type_by_name(TypeName type_name)
{
    for (size_t i = 0; i < ARRAY_LEN(type_mapping); i++)
        if (strcmp(type_name, type_mapping[i].type_name) == 0)
            return &type_mapping[i];

    return nullptr;
}

TypeResolveResult resolve_type(TypeName type_name, bool has_len, uint8_t len, ResolvedType* out)
{
    const TypeInfo* type = type_by_name(type_name);
    if (type == nullptr)
        return RESOLVE_UNKNOWN_TYPE;

    if (!type->expects_len && has_len)
        return RESOLVE_LENGTH_NOT_EXPECTED;

    if (type->expects_len && !has_len)
        return RESOLVE_LENGTH_REQUIRED;

    if (has_len) {
        if (len > type->max_len)
            return RESOLVE_LENGTH_TOO_BIG;

        if (len < type->min_len)
            return RESOLVE_LENGTH_TOO_SMALL;

        out->byte_len = len;
    } else {
        if (type->fixed_size == 0)
            return RESOLVE_MISCONFIGURED;

        out->byte_len = type->fixed_size;
    }
    out->type_info = type;

    if (!type->expects_len)
        COPY_STRING(out->resolved_type_name, type_name);
    else
        snprintf(out->resolved_type_name,
                 sizeof(out->resolved_type_name),
                 "%s(%i)",
                 type_name,
                 out->byte_len);

    return RESOLVE_OK;
}

const char* resolve_result_msg(TypeResolveResult result)
{
    switch (result) {
        case RESOLVE_UNKNOWN_TYPE: return "unknown type";
        case RESOLVE_LENGTH_NOT_EXPECTED: return "type does not accept length";
        case RESOLVE_LENGTH_REQUIRED: return "type requires length";
        case RESOLVE_LENGTH_TOO_BIG: return "length exceeds maximum";
        case RESOLVE_LENGTH_TOO_SMALL: return "length below minimum";
        case RESOLVE_MISCONFIGURED: return "misconfigured type";
        default: return "type resolution error";
    }
}

TypeValidateResult resolved_type_validate(const ResolvedType* rt, const DataValue* value) {
    if (value->type != rt->type_info->value_type)
        return VALIDATE_TYPE_MISMATCH;
    if (rt->type_info->validate)
        return rt->type_info->validate(value, rt->byte_len);
    return VALIDATE_OK;
}

const char* validate_type_result_msg(TypeValidateResult result)
{
    switch (result) {
        case VALIDATE_TYPE_MISMATCH: return "data type mismatch";
        case VALIDATE_VALUE_TOO_LONG: return "data too long for column";
        default: return "validate type error";
    }
}

