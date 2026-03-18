#include <maxql/core/utf8.h>
#include <stdint.h>

uint16_t utf8_strlen(const char* s)
{
    uint16_t count = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80)
            count++;
        s++;
    }
    return count;
}
