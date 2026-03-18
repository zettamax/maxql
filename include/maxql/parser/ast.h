#pragma once

#include <maxql/core/strview.h>
#include <maxql/types.h>

typedef StrView Ident;

typedef struct {
    LiteralType type;
    StrView value;
} RawValue;

typedef struct {
    Ident column;
    RawValue value;
} Assignment;

typedef struct {
    Assignment* data;
    size_t size;
    size_t capacity;
} AssignmentArray;

typedef struct {
    Ident column;
    ConditionOp op;
    RawValue value;
} Condition;

typedef struct {
    Condition* data;
    size_t size;
    size_t capacity;
} ConditionArray;

typedef struct {
    Ident* data;
    size_t size;
    size_t capacity;
} IdentArray;

typedef struct {
    Ident name;
    StrView len;
    bool has_len;
} ColumnTypeDef;

typedef struct {
    Ident column;
    ColumnTypeDef type;
    bool has_default;
    RawValue default_value;
} ColumnDef;

typedef struct {
    ColumnDef* data;
    size_t size;
    size_t capacity;
} ColumnDefArray;

typedef struct {
    Ident column;
    Ident key_len;
    bool has_key_len;
} IndexKeyPart;

typedef struct {
    IndexKeyPart* data;
    size_t size;
    size_t capacity;
} IndexKeyPartArray;

typedef struct {
    IdentArray columns;
    Ident table;
    bool ignore_index;
    ConditionArray where;
} SelectStmt;

typedef struct {
    Ident table;
    AssignmentArray assignments;
} InsertStmt;

typedef struct {
    Ident table;
    ConditionArray where;
} DeleteStmt;

typedef struct {
    Ident table;
    AssignmentArray assignments;
    ConditionArray where;
} UpdateStmt;

typedef struct {
    Ident table;
    ColumnDefArray column_defs;
} CreateTableStmt;

typedef struct {
    Ident table;
} DropTableStmt;

typedef struct {
    bool is_unique;
    Ident name;
    Ident type;
    Ident table;
    IndexKeyPartArray key_parts;
} CreateIndexStmt;

typedef struct {
    Ident table;
} ShowTableStmt;

typedef struct {
    Ident table;
} ShowIndexStmt;

typedef struct {
    Ident table;
    Ident index;
} DropIndexStmt;

typedef struct {
    Ident table;
} TruncateTableStmt;
