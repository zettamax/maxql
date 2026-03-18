#pragma once

#include <stdio.h>
#include <maxql/types.h>
#include <maxql/core/error.h>
#include <maxql/schema/schema.h>
#include <maxql/core/dyn_array.h>
#include <maxql/limits.h>

#define ROW_FLAGS_SIZE 1
#define ROW_SIZE_FIELD 2
#define HEADER_SIZE (ROW_FLAGS_SIZE + ROW_SIZE_FIELD)
#define MAX_DATA_SIZE (MAX_COLUMNS_PER_TABLE * (1 + MAX_VARCHAR_VALUE_SIZE))
#define MAX_ROW_SIZE (MAX_DATA_SIZE + HEADER_SIZE)

typedef DynArray(size_t) OffsetArray;

FILE* storage_open(TableName table_name);
bool storage_exists_table(const char* table_name);
bool storage_remove_table(const char* table_name);
Error storage_create_table(const char* table_name);
bool storage_read_current_row(FILE*, TableSchema*, DataRow*);
bool storage_load_row(FILE*, TableSchema*, size_t offset, DataRow*);
Error storage_delete_rows(FILE*, OffsetArray);
Error storage_delete_row(FILE* data_file, size_t offset);
Error storage_insert_row(FILE*, DataValue*, TableSchema*, size_t*);
void storage_close(FILE*);
