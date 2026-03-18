#pragma once

#include <maxql/types.h>
#include <maxql/types/types.h>

typedef struct {
    ColumnType column_type;
    bool align_right;
    uint8_t (*display_width)(uint8_t byte_len);
    size_t  (*max_literal_size)(uint8_t byte_len);
    void (*format_display)(const DataValue* value, char* buf, size_t buf_size);
    void (*format_literal)(const DataValue* value, char* buf, size_t buf_size);
} ColumnPresentation;

const ColumnPresentation* type_presentation(ColumnType column_type);
