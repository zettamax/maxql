#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h> // used by strncpy in COPY_STRING
#include <time.h>

#include <maxql/limits.h>
#include <maxql/core/dyn_array.h>

typedef DynArray(char) StrBuf;

#define COPY_STRING(dest, src) do {           \
    strncpy((dest), (src), sizeof(dest) - 1); \
    (dest)[sizeof(dest) - 1] = '\0';          \
} while(0)

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))

typedef char FileName[MAX_FILE_NAME_SIZE + 1];
typedef char TableName[MAX_TABLE_NAME_SIZE + 1];
typedef char ColumnName[MAX_COLUMN_NAME_SIZE + 1];
typedef char IndexName[MAX_INDEX_NAME_SIZE + 1];
typedef char IndexTypeName[MAX_INDEX_TYPE_NAME_SIZE + 1];
typedef char TypeName[MAX_TYPE_NAME_SIZE + 1];
typedef char ResolvedTypeName[MAX_TYPE_NAME_SIZE + 1];
typedef char VarcharValue[MAX_VARCHAR_VALUE_SIZE + 1];
typedef uint8_t BinaryValue[MAX_BINARY_VALUE_SIZE];

typedef uint64_t FileOffset;

typedef enum {
    INDEX_BTREE,
    INDEX_HASH,
} IndexType;

typedef enum {
    TYPE_INT,
    TYPE_VARCHAR,
    TYPE_CHAR,
    TYPE_BINARY,
    TYPE_FLOAT,
} ColumnType;

typedef enum {
    VALUE_NULL,
    VALUE_INT,
    VALUE_STRING,
    VALUE_BINARY,
    VALUE_FLOAT,
} ValueType;

typedef enum {
    QUERY_UNKNOWN = 0,

    QUERY_CREATE_TABLE,
    QUERY_DROP_TABLE,
    QUERY_CREATE_INDEX,
    QUERY_DROP_INDEX,
    QUERY_TRUNCATE_TABLE,

    QUERY_SELECT,
    QUERY_DELETE,
    QUERY_INSERT,
    QUERY_UPDATE,

    QUERY_SHOW_TABLES,
    QUERY_SHOW_TABLE,
    QUERY_SHOW_INDEX,
    QUERY_SHOW_INDEX_TYPES,
} QueryType;

typedef enum {
    OP_UNKNOWN = 0,
    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_LESS,
    OP_LESS_OR_EQUAL,
    OP_GREATER,
    OP_GREAT_OR_EQUAL,
} ConditionOp;

typedef enum {
    LIT_NUMBER,
    LIT_STRING,
    LIT_HEX,
} LiteralType;

typedef struct {
    ValueType type;
    union {
        int32_t int32;
        float float32;
        VarcharValue string;
        struct {
            BinaryValue bytes;
            uint8_t len;
        } bin;
    } val;
} DataValue;

typedef struct {
    DataValue values[MAX_COLUMNS_IN_SELECT];
} DataRow;

typedef struct {
    DataRow row;
    FileOffset offset;
} StoredRow;

typedef DynArray(StoredRow) DataTable;

typedef struct {
    struct timespec start;
    struct timespec end;
} TimeDelta;
