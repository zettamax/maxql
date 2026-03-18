#pragma once

#include <stdint.h>
#include <maxql/schema/schema.h>

size_t serialize_row_data(DataValue* values, TableSchema* schema, uint8_t* buf);
void deserialize_row_data(uint8_t* buf, TableSchema* schema, DataRow* data_row);
