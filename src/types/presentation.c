#include <maxql/types/presentation.h>
#include <maxql/types/formatter.h>
#include <stdio.h>

static void format_literal_string(const DataValue* value, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "\"%s\"", value->val.string);
}

static uint8_t display_width_int([[maybe_unused]] uint8_t byte_len)    { return 11; }
static uint8_t display_width_string(uint8_t byte_len)                  { return byte_len < 4 ? 4 : byte_len; }
static uint8_t display_width_binary(uint8_t byte_len)                  { return byte_len * 2 + 2; }
static uint8_t display_width_float([[maybe_unused]] uint8_t byte_len)  { return 15; }

static size_t max_literal_size_int([[maybe_unused]] uint8_t byte_len)    { return 12; }       // -2147483648 + null
static size_t max_literal_size_string(uint8_t byte_len)                  { return byte_len + 3; }   // "..." + null
static size_t max_literal_size_binary(uint8_t byte_len)                  { return byte_len * 2 + 3; } // 0x... + null
static size_t max_literal_size_float([[maybe_unused]] uint8_t byte_len)  { return 15; }       // %.7g + null

static const ColumnPresentation presentation_mapping[] = {
    [TYPE_INT] = {
        .column_type      = TYPE_INT,
        .align_right      = true,
        .display_width    = display_width_int,
        .max_literal_size = max_literal_size_int,
        .format_display   = format_int,
        .format_literal   = format_int,
    },
    [TYPE_VARCHAR] = {
        .column_type      = TYPE_VARCHAR,
        .align_right      = false,
        .display_width    = display_width_string,
        .max_literal_size = max_literal_size_string,
        .format_display   = format_string,
        .format_literal   = format_literal_string,
    },
    [TYPE_CHAR] = {
        .column_type      = TYPE_CHAR,
        .align_right      = false,
        .display_width    = display_width_string,
        .max_literal_size = max_literal_size_string,
        .format_display   = format_string,
        .format_literal   = format_literal_string,
    },
    [TYPE_BINARY] = {
        .column_type      = TYPE_BINARY,
        .align_right      = false,
        .display_width    = display_width_binary,
        .max_literal_size = max_literal_size_binary,
        .format_display   = format_binary,
        .format_literal   = format_binary,
    },
    [TYPE_FLOAT] = {
        .column_type      = TYPE_FLOAT,
        .align_right      = true,
        .display_width    = display_width_float,
        .max_literal_size = max_literal_size_float,
        .format_display   = format_float,
        .format_literal   = format_float,
    },
};

const ColumnPresentation* type_presentation(ColumnType column_type)
{
    return &presentation_mapping[column_type];
}
