#pragma once

#include <maxql/types.h>
#include <maxql/core/dyn_array.h>
#include <maxql/schema/schema.h>
#include <maxql/core/error.h>

typedef Error (*SchemaLoaderFn)(TableName, TableSchema*, IndexMeta*);

typedef struct {
    TableName name;
    TableSchema schema;
    IndexMeta index_meta;
} TableMeta;

typedef struct {
    DynArray(TableMeta) tables;
} Database;

Error database_load(SchemaLoaderFn schema_loader, Database** database);
void database_free(Database* database);
TableMeta* database_table_by_name(Database* database, TableName name);
Error database_save(Database* database);
Error database_add_table(Database* database, TableSchema* schema);
Error database_remove_table(Database* database, TableName table_name);
