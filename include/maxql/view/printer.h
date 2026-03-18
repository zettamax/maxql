#pragma once

#include <stddef.h>
#include <stdint.h>
#include <maxql/core/dyn_array.h>

typedef DynArray(const char**) PrinterRows;

typedef struct TablePrinter TablePrinter;
typedef struct TablePrinter {
    uint8_t column_count;
    const char** column_names;
    uint16_t* column_widths;
    bool* right_aligns;
    bool header_printed;
    size_t row_count;
    PrinterRows rows;
    void (*add_row)(TablePrinter* p, const char** cells);
    void (*finish)(TablePrinter* p);
} TablePrinter;

TablePrinter printer_stream(const char** names, uint16_t* widths, bool* right_aligns, uint8_t count);
TablePrinter printer_buffered(const char** names, bool* right_aligns, uint8_t count);
