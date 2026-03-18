#include <stdint.h>
#include <maxql/serializer/serializer.h>

size_t serialize_row_data(DataValue* values, TableSchema* schema, uint8_t* buf)
{
    size_t offset = 0;
    for (size_t i = 0; i < schema->column_count; i++) {
        ResolvedType* rt = &schema->columns[i].resolved_type;
        offset += rt->type_info->serialize(&values[i], rt->byte_len, &buf[offset]);
    }
    return offset;
}

void deserialize_row_data(uint8_t* buf, TableSchema* schema, DataRow* data_row)
{
    size_t offset = 0;
    for (size_t i = 0; i < schema->column_count; i++) {
        ResolvedType* rt = &schema->columns[i].resolved_type;
        offset += rt->type_info->deserialize(&buf[offset], rt->byte_len, &data_row->values[i]);
    }
}
