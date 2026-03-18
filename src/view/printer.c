#include <maxql/view/printer.h>
#include <stdio.h>
#include <string.h>
#include <maxql/core/utf8.h>

static void print_separator(uint16_t* widths, uint8_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        char buf[300];
        memset(buf, '-', widths[i] + 2);
        buf[widths[i] + 2] = '\0';
        printf("+%s", buf);
    }
    printf("+\n");
}

static void print_header(const char** names, uint16_t* widths, uint8_t count)
{
    for (size_t j = 0; j < count; j++)
        printf("| %-*s ", widths[j], names[j]);
    printf("|\n");
}

static void print_cell(const char* string, uint16_t width, bool right_align)
{
    size_t padding = strlen(string) - utf8_strlen(string);
    if (right_align)
        printf("| %*s ", width + (int)padding, string);
    else
        printf("| %-*s ", width + (int)padding, string);
}

static void print_row(const char** cells, uint16_t* widths, bool* right_aligns, uint8_t count)
{
    for (size_t i = 0; i < count; i++) {
        print_cell(cells[i], widths[i], right_aligns[i]);
    }
    printf("|\n");
}

static void print_empty_result(void)
{
    printf("(empty result)\n");
}

static void print_row_count(TablePrinter* p)
{
    printf("Rows: %zu\n", p->row_count);
}

static void add_row_stream(TablePrinter* p, const char** cells)
{
    if (!p->header_printed) {
        print_separator(p->column_widths, p->column_count);
        print_header(p->column_names, p->column_widths, p->column_count);
        print_separator(p->column_widths, p->column_count);
        p->header_printed = true;
    }

    print_row(cells, p->column_widths, p->right_aligns, p->column_count);
    p->row_count++;
}

static void free_printer(TablePrinter* p)
{
    free(p->column_names); // single block: names + widths + aligns
}

static void finish_stream(TablePrinter* p)
{
    if (p->header_printed) {
        print_separator(p->column_widths, p->column_count);
        print_row_count(p);
    } else
        print_empty_result();

    free_printer(p);
}

static void add_row_buffered(TablePrinter* p, const char** cells)
{
    const char** row = calloc(p->column_count, sizeof(*cells));
    for (size_t i = 0; i < p->column_count; i++) {
        row[i] = strdup(cells[i]);
        uint16_t len = utf8_strlen(cells[i]);
        if (len > p->column_widths[i])
            p->column_widths[i] = len;
    }
    da_push(&p->rows, row);
    p->row_count++;
}

static void finish_buffered(TablePrinter* p)
{
    if (p->rows.size) {
        print_separator(p->column_widths, p->column_count);
        print_header(p->column_names, p->column_widths, p->column_count);
        print_separator(p->column_widths, p->column_count);
        for (size_t i = 0; i < p->rows.size; i++) {
            print_row(p->rows.data[i], p->column_widths, p->right_aligns, p->column_count);
            for (size_t j = 0; j < p->column_count; j++)
                free((void*)p->rows.data[i][j]);
            free(p->rows.data[i]);
        }
        da_free(&p->rows);
        print_separator(p->column_widths, p->column_count);
        print_row_count(p);
    } else
        print_empty_result();

    free_printer(p);
}

static TablePrinter allocate_printer(uint8_t count)
{
    void* block = calloc(count, sizeof(const char*) + sizeof(uint16_t) + sizeof(bool));
    TablePrinter p = { .column_count = count };
    p.column_names = block;
    p.column_widths = (uint16_t*)(p.column_names + count);
    p.right_aligns = (bool*)(p.column_widths + count);

    return p;
}

TablePrinter printer_stream(const char** names, uint16_t* widths, bool* right_aligns, uint8_t count)
{
    TablePrinter p = allocate_printer(count);
    memcpy(p.column_names, names, count * sizeof(*p.column_names));
    memcpy(p.column_widths, widths, count * sizeof(*p.column_widths));
    memcpy(p.right_aligns, right_aligns, count * sizeof(*p.right_aligns));

    for (uint16_t i = 0; i < count; i++)
        if (strlen(p.column_names[i]) > p.column_widths[i])
            p.column_widths[i] = strlen(p.column_names[i]);

    p.add_row = add_row_stream;
    p.finish = finish_stream;

    return p;
}

TablePrinter printer_buffered(const char** names, bool* right_aligns, uint8_t count)
{
    TablePrinter p = allocate_printer(count);
    memcpy(p.column_names, names, count * sizeof(*p.column_names));
    memcpy(p.right_aligns, right_aligns, count * sizeof(*p.right_aligns));

    for (uint16_t i = 0; i < count; i++)
        p.column_widths[i] = strlen(names[i]);

    p.add_row = add_row_buffered;
    p.finish = finish_buffered;

    return p;
}
