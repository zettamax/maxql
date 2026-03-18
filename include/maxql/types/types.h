#pragma once

#include <stddef.h>
#include <maxql/types.h>
#include <maxql/types/key_compare.h>

typedef enum {
    RESOLVE_OK = 0,
    RESOLVE_UNKNOWN_TYPE,
    RESOLVE_LENGTH_NOT_EXPECTED,
    RESOLVE_LENGTH_REQUIRED,
    RESOLVE_LENGTH_TOO_BIG,
    RESOLVE_LENGTH_TOO_SMALL,
    RESOLVE_MISCONFIGURED,
} TypeResolveResult;

typedef enum {
    VALIDATE_OK = 0,
    VALIDATE_TYPE_MISMATCH,
    VALIDATE_VALUE_TOO_LONG,
} TypeValidateResult;

typedef struct {
    ColumnType column_type;
    ValueType value_type;
    TypeName type_name;
    bool expects_len;
    size_t min_len;
    size_t max_len;
    uint8_t fixed_size;
    KeyComparator compare;
    TypeValidateResult (*validate)(const DataValue* value, uint8_t byte_len);
    size_t (*serialize)(const DataValue* value, uint8_t byte_len, uint8_t* buf);
    size_t (*deserialize)(const uint8_t* buf, uint8_t byte_len, DataValue* value);
} TypeInfo;

typedef struct {
    const TypeInfo* type_info;
    uint8_t byte_len;
    ResolvedTypeName resolved_type_name;
} ResolvedType;
