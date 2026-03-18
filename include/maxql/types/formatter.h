#pragma once

#include <stddef.h>
#include <maxql/types.h>

void format_int(const DataValue* value, char* buf, size_t buf_size);
void format_string(const DataValue* value, char* buf, size_t buf_size);
void format_binary(const DataValue* value, char* buf, size_t buf_size);
void format_float(const DataValue* value, char* buf, size_t buf_size);

