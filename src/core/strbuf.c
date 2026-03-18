#include <maxql/core/strbuf.h>
#include <string.h>

void strbuf_append(StrBuf* buf, const char* s)
{
    size_t len = strlen(s);
    size_t required = buf->size + len;
    while (buf->capacity < required)
        buf->data = da_grow(buf->data, &buf->capacity, sizeof(char));
    memcpy(buf->data + buf->size, s, len);
    buf->size += len;
}
