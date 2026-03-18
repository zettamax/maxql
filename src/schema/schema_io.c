#include <maxql/index/index_io.h>
#include <maxql/parser/schema_parser.h>
#include <maxql/types/converter.h>
#include <maxql/core/strbuf.h>
#include <maxql/lib/fs.h>
#include <maxql/schema/schema_io.h>
#include <stdlib.h>
#include <stdio.h>

#include "maxql/types/mapper.h"
#include <maxql/types/presentation.h>

#define SCHEMA_LEVEL_INDENT "  "

bool schema_exist_table(char* table_name)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.schema", table_name);
    return check_file_exists(filename);
}

bool schema_remove_table(char* table_name)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.schema", table_name);
    return remove(filename) == 0;
}

static Error schema_validate_column_name(ColumnName column_name)
{
    size_t column_name_len = strlen(column_name);
    if (column_name_len > MAX_COLUMN_NAME_SIZE)
        return error_fmt(ERR_STORAGE, "Column name too long");
    if (column_name_len == 0)
        return error_fmt(ERR_STORAGE, "Column name is empty");
    for (size_t i = 0; i < column_name_len; i++) {
        int is_valid_char = (column_name[i] >= 'a' && column_name[i] <= 'z') || column_name[i] == '_';
        if (!is_valid_char)
            return error_fmt(ERR_STORAGE, "Invalid column name");
    }
    return error_ok();
}

Error schema_load_table(TableName table_name, TableSchema* schema, IndexMeta* index_meta)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.schema", table_name);
    FILE* schema_file = nullptr;
    Error error = open_file(filename, "r", &schema_file, ERR_STORAGE, "Schema file do not exists");
    if (!error_is_ok(error))
        return error;

    fseek(schema_file, 0, SEEK_END);
    long file_size = ftell(schema_file);
    rewind(schema_file);
    char* content = malloc((size_t)file_size + 1);
    fread(content, 1, (size_t)file_size, schema_file);
    content[file_size] = '\0';
    fclose(schema_file);

    Parser parser = parser_init(sv_from_cstr(content));
    SchemaParseResult parse_result = parse_schema(&parser);

    if (!error_is_ok(parser.error)) {
        schema_parse_result_free(&parse_result);
        free(content);
        return parser.error;
    }

    schema->column_count = 0;
    index_meta->index_count = 0;
    COPY_STRING(schema->table_name, table_name);

    for (size_t i = 0; i < parse_result.column_defs.size; i++) {
        if (schema->column_count >= MAX_COLUMNS_PER_TABLE) {
            schema_parse_result_free(&parse_result);
            free(content);
            return error_fmt(ERR_STORAGE, "Too many columns in table");
        }

        ColumnDef* cd = &parse_result.column_defs.data[i];
        Column column = {};

        ColumnName column_name;
        sv_to_cstr(cd->column, column_name, sizeof(column_name));
        Error err = schema_validate_column_name(column_name);
        if (!error_is_ok(err)) {
            schema_parse_result_free(&parse_result);
            free(content);
            return err;
        }
        COPY_STRING(column.column_name, column_name);

        TypeName type_name;
        sv_to_cstr(cd->type.name, type_name, sizeof(type_name));

        int type_len = 0;
        if (cd->type.has_len) {
            DataValue len_val;
            ConvertResult cr = convert_int(cd->type.len, &len_val);
            if (cr != CONVERT_OK) {
                schema_parse_result_free(&parse_result);
                free(content);
                return error_fmt(ERR_STORAGE, "Invalid type length for '%s': %s",
                                 column_name, convert_result_msg(cr));
            }
            if (len_val.val.int32 < 0 || len_val.val.int32 > 255) {
                schema_parse_result_free(&parse_result);
                free(content);
                return error_fmt(ERR_STORAGE, "Type length out of range for '%s'", column_name);
            }
            type_len = len_val.val.int32;
        }

        TypeResolveResult rr = resolve_type(type_name, cd->type.has_len, (uint8_t)type_len, &column.resolved_type);
        if (rr != RESOLVE_OK) {
            schema_parse_result_free(&parse_result);
            free(content);
            return error_fmt(ERR_STORAGE, "Resolving type for '%s.%s': %s",
                             table_name, column_name, resolve_result_msg(rr));
        }

        if (cd->has_default) {
            column.has_default = true;
            ConvertResult cr = convert_value(cd->default_value.type, cd->default_value.value, &column.default_value);
            if (cr != CONVERT_OK) {
                schema_parse_result_free(&parse_result);
                free(content);
                return error_fmt(ERR_STORAGE, "Invalid default for '%s': %s",
                                 column_name, convert_result_msg(cr));
            }
        }

        schema->columns[schema->column_count++] = column;
    }

    Error err = index_resolve(&parse_result, schema, index_meta);
    schema_parse_result_free(&parse_result);
    free(content);
    return err;
}

static void schema_serialize_columns(TableSchema* table_schema, StrBuf* buf, const char* indent)
{
    for (size_t i = 0; i < table_schema->column_count; i++) {
        Column* col = &table_schema->columns[i];
        strbuf_append(buf, indent);
        strbuf_append(buf, col->column_name);
        strbuf_append(buf, " ");
        strbuf_append(buf, col->resolved_type.resolved_type_name);

        if (col->has_default) {
            const ColumnPresentation* p = type_presentation(col->resolved_type.type_info->column_type);
            size_t sz = p->max_literal_size(col->resolved_type.byte_len);
            char tmp[sz];
            p->format_literal(&col->default_value, tmp, sz);
            strbuf_append(buf, " default ");
            strbuf_append(buf, tmp);
        }

        strbuf_append(buf, ",\n");
    }
}

Error schema_save_table(TableSchema* table_schema, IndexMeta* index_meta)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.schema", table_schema->table_name);
    FILE* schema_file = nullptr;
    Error file_error = open_file(filename, "w", &schema_file, ERR_STORAGE, "Failed to create schema file");
    if (!error_is_ok(file_error))
        return file_error;

    StrBuf buf;
    da_init(&buf);

    strbuf_append(&buf, "table (\n");
    strbuf_append(&buf, SCHEMA_LEVEL_INDENT "columns (\n");
    schema_serialize_columns(table_schema, &buf, SCHEMA_LEVEL_INDENT SCHEMA_LEVEL_INDENT);
    strbuf_append(&buf, SCHEMA_LEVEL_INDENT "),\n");

    strbuf_append(&buf, SCHEMA_LEVEL_INDENT "indexes (\n");
    if (index_meta != nullptr)
        index_serialize(index_meta, &buf, SCHEMA_LEVEL_INDENT SCHEMA_LEVEL_INDENT);
    strbuf_append(&buf, SCHEMA_LEVEL_INDENT "),\n");

    strbuf_append(&buf, ")\n");

    fwrite(buf.data, 1, buf.size, schema_file);
    fflush(schema_file);
    fclose(schema_file);
    da_free(&buf);

    return error_ok();
}
