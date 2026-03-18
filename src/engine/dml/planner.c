#include <maxql/engine/dml/planner.h>
#include <maxql/index/index.h>

static WherePredicate* column_covered(uint8_t column_pos, IndexType type, WhereFilter* where)
{
    for (uint8_t i = 0; i < where->predicate_count; i++) {
        WherePredicate* predicate = &where->predicates[i];
        if (column_pos == predicate->column_index) {
            if (index_supports_op(type, predicate->op))
                return &where->predicates[i];
            break;
        }
    }
    return nullptr;
}

static uint8_t score_index(IndexDefinition* index_def, WhereFilter* where)
{
    uint8_t score = 0;
    for (uint8_t i = 0; i < index_def->column_count; i++) {
        if (!column_covered(index_def->column_pos[i], index_def->type, where))
            break;
        score++;
    }
    return score;
}

static void choose_best_index(WhereFilter* where, IndexMeta* index_meta, IndexDefinition** out_index_def)
{
    uint8_t max_score = 0;
    for (uint8_t i = 0; i < index_meta->index_count; i++) {
        IndexDefinition* index_def = &index_meta->index_defs[i];

        uint8_t score = score_index(index_def, where);
        if (index_vtable(index_def->type)->requires_full_match && score != index_def->column_count)
            continue;

        if (score > max_score) {
            max_score = score;
            *out_index_def = index_def;
        }
    }
}

static void fill_best_index(IndexDefinition* index_def, WhereFilter* where, IndexDecision* index_decision)
{
    index_decision->use_index = false;
    if (index_def == nullptr)
        return;

    index_decision->index_def = index_def;
    index_decision->condition_count = 0;
    for (uint8_t i = 0; i < index_def->column_count; i++) {
        uint8_t column_pos = index_def->column_pos[i];
        WherePredicate* predicate = column_covered(column_pos, index_def->type, where);
        if (!predicate)
            break;

        index_decision->condition_count++;
        index_decision->index_conditions[i].op = predicate->op;
        index_decision->index_conditions[i].value = predicate->value;
        index_decision->values[i] = predicate->value;
        index_decision->column_positions[i] = column_pos;
    }

    index_decision->use_index = index_decision->condition_count > 0;
}

static void select_best_index(IndexDecision* index_decision, WhereFilter* where, IndexMeta* index_meta)
{
    IndexDefinition* best_index_def = nullptr;
    choose_best_index(where, index_meta, &best_index_def);
    fill_best_index(best_index_def, where, index_decision);
}

Error plan_select(QueryContext* query_ctx)
{
    SelectExecPlan* select_plan = &query_ctx->select;
    IndexDecision* index_decision = &select_plan->index_decision;
    WhereFilter* where = &select_plan->where;
    IndexMeta* index_meta = &query_ctx->table_meta->index_meta;

    if (!select_plan->ignore_index)
        select_best_index(index_decision, where, index_meta);

    return error_ok();
}

Error plan_delete(QueryContext* query_ctx)
{
    IndexDecision* index_decision = &query_ctx->delete.index_decision;
    WhereFilter* where = &query_ctx->delete.where;
    IndexMeta* index_meta = &query_ctx->table_meta->index_meta;

    select_best_index(index_decision, where, index_meta);

    return error_ok();
}

Error plan_update(QueryContext* query_ctx)
{
    IndexDecision* index_decision = &query_ctx->update.index_decision;
    WhereFilter* where = &query_ctx->update.where;
    IndexMeta* index_meta = &query_ctx->table_meta->index_meta;

    select_best_index(index_decision, where, index_meta);

    return error_ok();
}