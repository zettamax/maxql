#include <maxql/schema/schema.h>
#include <string.h>
#include <maxql/index/index_types.h>
#include <maxql/types.h>

bool table_column_pos(TableSchema* schema, ColumnName column_name, uint8_t* column_pos)
{
    for (size_t i = 0; i < schema->column_count; i++)
        if (strcmp(column_name, schema->columns[i].column_name) == 0) {
            *column_pos = i;
            return true;
        }

    return false;
}

IndexDefinition* get_index_def(IndexMeta* index_meta, IndexName index_name)
{
    for (size_t i = 0; i < index_meta->index_count; i++)
        if (strcmp(index_meta->index_defs[i].name, index_name) == 0)
            return &index_meta->index_defs[i];

    return nullptr;
}

void schema_remove_index_def(size_t index_pos, IndexMeta* index_meta)
{
    size_t copy_count = sizeof(IndexDefinition) * (index_meta->index_count - index_pos - 1);
    if (copy_count > 0)
        memmove(&index_meta->index_defs[index_pos],
               &index_meta->index_defs[index_pos + 1],
               copy_count);

    index_meta->index_count--;
}
