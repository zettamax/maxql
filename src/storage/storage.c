#include <maxql/storage/storage.h>
#include <stdint.h>
#include <string.h>
#include <maxql/core/error.h>
#include <maxql/lib/fs.h>
#include <maxql/serializer/serializer.h>
#include <maxql/schema/schema.h>
#include <maxql/types.h>

FILE* storage_open(TableName table_name)
{
    FileName file_name;
    snprintf(file_name, sizeof(file_name), "%s.data", table_name);
    FILE* data_file;
    open_file(file_name, "r+", &data_file, ERR_STORAGE, "Failed to open data file");

    return data_file;
}

bool storage_exists_table(const char* table_name)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.data", table_name);
    return check_file_exists(filename);
}

bool storage_remove_table(const char* table_name)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.data", table_name);
    return remove(filename) == 0;
}

Error storage_create_table(const char* table_name)
{
    char filename[MAX_FILE_NAME_SIZE];
    snprintf(filename, MAX_FILE_NAME_SIZE, "%s.data", table_name);
    FILE* data_file = nullptr;
    Error file_error = open_file(filename,
                                 "wx",
                                 &data_file,
                                 ERR_STORAGE,
                                 "Failed to create data file");
    if (!error_is_ok(file_error))
        return file_error;
    fflush(data_file);
    fclose(data_file);

    return error_ok();
}

bool storage_read_current_row(FILE* data_file, TableSchema* schema, DataRow* data_row)
{
    uint8_t header_buffer[HEADER_SIZE];
    fread(header_buffer, 1, HEADER_SIZE, data_file);
    uint16_t row_size;
    memcpy(&row_size, &header_buffer[1], ROW_SIZE_FIELD);
    row_size -= HEADER_SIZE;

    if ((header_buffer[0] & 1) == 1) {
        fseeko(data_file, row_size, SEEK_CUR);
        return false;
    }

    uint8_t data_buffer[MAX_ROW_SIZE];
    fread(data_buffer, 1, row_size, data_file);
    deserialize_row_data(data_buffer, schema, data_row);

    return true;
}

bool storage_load_row(FILE* data_file, TableSchema* schema, size_t offset, DataRow* out)
{
    fseeko(data_file, offset, SEEK_SET);

    return storage_read_current_row(data_file, schema, out);
}

Error storage_delete_rows(FILE* data_file, OffsetArray offsets)
{
    for (size_t i = 0; i < offsets.size; i++) {
        fseeko(data_file, (long)offsets.data[i], SEEK_SET);
        uint8_t flags = 1; // deleted
        fwrite(&flags, 1, sizeof(flags), data_file);
    }
    fflush(data_file);

    return error_ok();
}

Error storage_delete_row(FILE* data_file, size_t offset)
{
    fseeko(data_file, (long)offset, SEEK_SET);
    uint8_t flags = 1; // deleted
    fwrite(&flags, 1, sizeof(flags), data_file);
    fflush(data_file);

    return error_ok();
}

Error storage_insert_row(FILE* data_file,
                         DataValue* values,
                         TableSchema* schema,
                         size_t* offset)
{
    uint8_t buf[MAX_ROW_SIZE];
    size_t data_size = serialize_row_data(values, schema, &buf[HEADER_SIZE]);

    buf[0] = 0;
    uint16_t row_size = HEADER_SIZE + data_size;
    memcpy(&buf[ROW_FLAGS_SIZE], &row_size, ROW_SIZE_FIELD);

    fseeko(data_file, 0, SEEK_END);
    *offset = ftello(data_file);
    fwrite(buf, 1, row_size, data_file);
    fflush(data_file);

    return error_ok();
}

void storage_close(FILE* data_file)
{
    if (data_file) {
        fclose(data_file);
    }
}
