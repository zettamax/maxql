#include <maxql/types/formatter.h>
#include <stdio.h>

void format_int(const DataValue* value, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%i", value->val.int32);
}

void format_string(const DataValue* value, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%s", value->val.string);
}

void format_binary(const DataValue* value, char* buf, size_t buf_size)
{
    size_t offset = snprintf(buf, buf_size, "0x");
    for (size_t i = 0; i < value->val.bin.len; i++)
        offset += snprintf(&buf[offset], buf_size - offset, "%02x", value->val.bin.bytes[i]);
}

void format_float(const DataValue* value, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%.7g", value->val.float32);
}
