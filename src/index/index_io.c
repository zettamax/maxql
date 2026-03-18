#include <maxql/index/index_io.h>
#include <maxql/index/index.h>
#include <maxql/schema/schema.h>
#include <maxql/types/mapper.h>
#include <maxql/types/converter.h>
#include <maxql/core/strbuf.h>
#include <string.h>
#include <stdio.h>

static Error resolve_index_part(IndexDefinition* idx, uint8_t part_idx,
                                  ColumnName col_name, uint8_t part_len,
                                  TableSchema* schema)
{
    for (size_t k = 0; k < schema->column_count; k++) {
        if (strcmp(schema->columns[k].column_name, col_name) != 0)
            continue;

        Column* col = &schema->columns[k];
        COPY_STRING(idx->columns[part_idx], col_name);
        idx->column_pos[part_idx] = k;
        idx->part_len[part_idx] = part_len;
        idx->part_type[part_idx] = col->resolved_type.type_info->value_type;
        idx->part_compare[part_idx] = col->resolved_type.type_info->compare;
        idx->key_size += part_len;
        return error_ok();
    }
    return error_fmt(ERR_STORAGE, "Index column '%s' not found in schema", col_name);
}

Error index_resolve(SchemaParseResult* parse_result, TableSchema* schema, IndexMeta* out)
{
    out->index_count = 0;
    for (size_t i = 0; i < parse_result->index_defs.size; i++) {
        if (out->index_count >= MAX_TABLE_INDEX_COUNT)
            return error_fmt(ERR_STORAGE, "Too many indexes");

        IndexDef* def = &parse_result->index_defs.data[i];
        IndexDefinition idx = {};

        IndexTypeName type_name;
        sv_to_cstr(def->type, type_name, sizeof(type_name));
        const IndexVTable* vt = index_vtable_by_name(type_name);
        if (!vt)
            return error_fmt(ERR_STORAGE, "Unknown index type '%s'", type_name);
        idx.type = vt->type;
        idx.node_size = vt->default_node_size;
        idx.is_unique = def->is_unique;

        IndexName index_name;
        sv_to_cstr(def->name, index_name, sizeof(index_name));
        COPY_STRING(idx.name, index_name);

        idx.column_count = 0;
        idx.key_size = 0;

        for (size_t j = 0; j < def->key_parts.size; j++) {
            if (idx.column_count >= MAX_INDEX_COLUMN_COUNT)
                return error_fmt(ERR_STORAGE, "Too many key parts in index '%s'", idx.name);

            IndexKeyPart* kp = &def->key_parts.data[j];
            ColumnName col_name;
            sv_to_cstr(kp->column, col_name, sizeof(col_name));

            uint8_t col_pos;
            if (!table_column_pos(schema, col_name, &col_pos))
                return error_fmt(ERR_STORAGE, "Index column '%s' not found in schema", col_name);

            uint8_t part_len;
            if (kp->has_key_len) {
                DataValue len_val;
                ConvertResult cr = convert_int(kp->key_len, &len_val);
                if (cr != CONVERT_OK)
                    return error_fmt(ERR_STORAGE, "Invalid key part length for '%s': %s", col_name, convert_result_msg(cr));
                uint8_t col_byte_len = schema->columns[col_pos].resolved_type.byte_len;
                if (len_val.val.int32 < 1 || len_val.val.int32 > col_byte_len)
                    return error_fmt(ERR_STORAGE, "Key part length out of range for '%s'", col_name);
                part_len = (uint8_t)len_val.val.int32;
            } else {
                part_len = schema->columns[col_pos].resolved_type.byte_len;
            }

            Error err = resolve_index_part(&idx, idx.column_count, col_name, part_len, schema);
            if (!error_is_ok(err))
                return err;
            idx.column_count++;
        }

        out->index_defs[out->index_count++] = idx;
    }
    return error_ok();
}

void index_serialize(IndexMeta* index_meta, StrBuf* buf, const char* indent)
{
    for (uint8_t i = 0; i < index_meta->index_count; i++) {
        IndexDefinition* idx = &index_meta->index_defs[i];
        const IndexVTable* vt = index_vtable(idx->type);

        strbuf_append(buf, indent);
        strbuf_append(buf, vt->type_name);
        strbuf_append(buf, idx->is_unique ? " unique " : " ");
        strbuf_append(buf, idx->name);
        strbuf_append(buf, " (");

        for (uint8_t c = 0; c < idx->column_count; c++) {
            if (c > 0)
                strbuf_append(buf, ", ");
            char part[MAX_COLUMN_NAME_SIZE + 8];
            snprintf(part, sizeof(part), "%s(%u)", idx->columns[c], idx->part_len[c]);
            strbuf_append(buf, part);
        }

        strbuf_append(buf, "),\n");
    }
}
