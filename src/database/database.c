#include <maxql/database/database.h>

#include <maxql/constants.h>
#include <string.h>
#include <maxql/lib/fs.h>
#include <maxql/lib/mem.h>

Error database_load(SchemaLoaderFn schema_loader, Database** database)
{
    FILE* file = nullptr;
    Database* db = nullptr;

    Error error = safe_malloc((void**)&db, sizeof(Database));
    if (!error_is_ok(error))
        goto cleanup;
    da_init(&db->tables);

    error = open_file(TABLE_CATALOG_FILE_NAME,
                      "r",
                      &file,
                      ERR_STORAGE,
                      "Cannot open catalog file");
    if (!error_is_ok(error))
        goto cleanup;

    char line[64];
    while (fgets(line, sizeof(line), file)) {
        TableMeta table_meta = {};

        const char* buffer = strtok(line, "\n");
        if (strlen(buffer) > MAX_TABLE_NAME_SIZE) {
            error = error_fmt(ERR_STORAGE, "Too long table name in catalog");
            goto cleanup;
        }
        COPY_STRING(table_meta.name, buffer);

        error = schema_loader(table_meta.name, &table_meta.schema, &table_meta.index_meta);
        if (!error_is_ok(error))
            goto cleanup;

        da_push(&db->tables, table_meta);
    }
    *database = db;

cleanup:
    if (file != nullptr)
        fclose(file);
    if (!error_is_ok(error)) {
        database_free(db);
        db = nullptr;
    }

    return error;
}

void database_free(Database* database)
{
    if (database != nullptr) {
        if (database->tables.data != nullptr)
            da_free(&database->tables);
        free(database);
    }
}

TableMeta* database_table_by_name(Database* database, TableName name)
{
    for (size_t i = 0; i < database->tables.size; i++)
        if (strcmp(name, database->tables.data[i].name) == 0)
            return &database->tables.data[i];

    return nullptr;
}

Error database_save(Database* database)
{
    FILE* file = nullptr;
    Error error = open_file(TABLE_CATALOG_FILE_NAME,
                            "w",
                            &file,
                            ERR_STORAGE,
                            "Cannot open catalog file");
    if (!error_is_ok(error))
        goto cleanup;

    char buffer[8192];
    size_t offset = 0;
    for (size_t i = 0; i < database->tables.size; i++) {
        offset += snprintf(&buffer[offset],
                           sizeof(TableName),
                           "%s\n",
                           database->tables.data[i].name);
    }
    fprintf(file, "%s", buffer);
    fflush(file);

cleanup:
    if (file != nullptr)
        fclose(file);

    return error;
}

Error database_add_table(Database* database, TableSchema* schema)
{
    TableMeta table_meta = {};
    COPY_STRING(table_meta.name, schema->table_name);
    table_meta.schema = *schema;
    da_push(&database->tables, table_meta);

    Error error = database_save(database);
    if (!error_is_ok(error))
        database->tables.size--;

    return error;
}

Error database_remove_table(Database* database, TableName table_name)
{
    int shift = 0;
    for (size_t i = 0; i < database->tables.size; i++) {
        database->tables.data[i - shift] = database->tables.data[i];
        if (strcmp(database->tables.data[i].name, table_name) == 0) {
            shift = 1;

        }
    }
    if (shift != 0)
        database->tables.size--;

    return database_save(database);
}