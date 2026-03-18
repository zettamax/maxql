#include <maxql/index/btree.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <maxql/lib/fs.h>
#include <string.h>
#include <sys/types.h>
#include <maxql/core/error.h>
#include <maxql/index/comparator.h>
#include <maxql/index/index_types.h>
#include <maxql/types.h>

typedef struct BtreeNode {
    struct BtreeNode* parent;
    struct BtreeNode** children;
    uint8_t flags;
    uint16_t key_count;
    uint8_t* keys;
    FileOffset* offsets;
} BtreeNode;

typedef struct {
    uint16_t key_size;
    uint16_t node_size;
    KeyComparator comparator;
    ComparatorCompoundCtx comparator_ctx;
} BtreeConfig;

typedef struct {
    uint16_t key_size;
    uint16_t offset_size;
    uint16_t child_size;
    uint16_t max_keys;
    uint16_t min_keys;
    uint16_t mid;
    uint16_t left_count;
    uint16_t right_count;
    uint16_t child_count;
    uint16_t right_start;
    uint16_t flags_offset;
    uint16_t flags_size;
    uint16_t key_count_offset;
    uint16_t key_count_size;
    uint16_t keys_offset;
    uint16_t keys_size;
    uint16_t offsets_offset;
    uint16_t offsets_size;
    uint16_t children_offset;
    uint16_t children_size;
    uint16_t node_size;
} BtreeLayout;

typedef struct {
    BtreeNode* root;
    BtreeLayout layout;
    KeyComparator compare;
    void* ctx;
    BtreeConfig* config;
} Btree;

static constexpr uint8_t BTREE_NODE_FLAGS_SIZE = sizeof((BtreeNode){}.flags);
static constexpr uint8_t BTREE_NODE_VALUE_COUNT_SIZE = sizeof((BtreeNode){}.key_count);

enum BtreeNodeFlags {
    BTREE_NODE_LEAF = 1 << 0,
};

void build_comparator_compound_ctx(IndexDefinition* index_def, ComparatorCompoundCtx* ctx)
{
    for (uint8_t i = 0; i < index_def->column_count; i++) {
        ctx->field_sizes[i] = index_def->part_len[i];
        ctx->field_comparators[i] = index_def->part_compare[i];
    }
    ctx->field_count = index_def->column_count;
}

void* btree_build_config(IndexDefinition* index_def)
{
    BtreeConfig* config = malloc(sizeof(BtreeConfig));
    config->key_size = index_def->key_size;
    config->node_size = index_def->node_size;
    config->comparator = compound_compare;
    build_comparator_compound_ctx(index_def, &config->comparator_ctx);
    return config;
}

void btree_free_config(void* config) { free(config); }

BtreeLayout btree_layout_create(uint16_t node_size, uint16_t key_size)
{
    uint16_t offset_size = sizeof(FileOffset);
    uint16_t child_size = sizeof(BtreeNode*);
    uint16_t dynamic_space = node_size - BTREE_NODE_FLAGS_SIZE - BTREE_NODE_VALUE_COUNT_SIZE;
    uint16_t space_adjusted = dynamic_space - child_size;
    uint16_t max_keys = space_adjusted / (key_size + offset_size + child_size);
    uint16_t min_keys = (max_keys - 1) / 2;
    uint16_t mid = (max_keys + 1) / 2;
    uint16_t left_count = mid;
    uint16_t right_count = max_keys - left_count;
    uint16_t child_count = max_keys + 1;
    uint16_t right_start = left_count + 1;
    uint16_t flags_offset = 0;
    uint16_t flags_size = BTREE_NODE_FLAGS_SIZE;
    uint16_t key_count_offset = flags_size;
    uint16_t key_count_size = BTREE_NODE_VALUE_COUNT_SIZE;
    uint16_t keys_offset = key_count_offset + key_count_size;
    uint16_t keys_size = max_keys * key_size;
    uint16_t offsets_offset = keys_offset + keys_size;
    uint16_t offsets_size = max_keys * offset_size;
    uint16_t children_offset = offsets_offset + offsets_size;
    uint16_t children_size = child_count * child_size;
    uint16_t node_disk_size = node_size;
    return (BtreeLayout){
        key_size,
        offset_size,
        child_size,
        max_keys,
        min_keys,
        mid,
        left_count,
        right_count,
        child_count,
        right_start,
        flags_offset,
        flags_size,
        key_count_offset,
        key_count_size,
        keys_offset,
        keys_size,
        offsets_offset,
        offsets_size,
        children_offset,
        children_size,
        node_disk_size,
    };
}

bool btree_supports_op(ConditionOp op) { return op == OP_EQUALS; }

void* btree_create(void* config)
{
    BtreeConfig* btree_config = config;
    Btree* btree = calloc(1, sizeof(Btree));
    btree->root = nullptr;
    btree->layout = btree_layout_create(btree_config->node_size, btree_config->key_size);
    btree->compare = btree_config->comparator;
    btree->ctx = &btree_config->comparator_ctx;
    btree->config = btree_config;

    return btree;
}

BtreeNode* allocate_node(BtreeLayout* layout)
{
    BtreeNode* node = calloc(1, sizeof(BtreeNode));
    node->keys = calloc(layout->max_keys, layout->key_size);
    node->offsets = calloc(layout->max_keys, layout->offset_size);
    node->children = calloc(layout->child_count, layout->child_size);

    return node;
}

bool flag_is_leaf(uint8_t flags) { return flags & BTREE_NODE_LEAF; }

bool btree_node_is_leaf(BtreeNode* node) { return flag_is_leaf(node->flags); }

BtreeNode* parse_tree(BtreeLayout* layout, uint8_t* binary, BtreeNode* parent, size_t offset)
{
    BtreeNode* node = allocate_node(layout);
    node->parent = parent;

    memcpy(&node->flags, &binary[offset], layout->flags_size);
    memcpy(&node->key_count, &binary[offset + layout->key_count_offset], layout->key_count_size);
    memcpy(node->keys, &binary[offset + layout->keys_offset], layout->keys_size);
    memcpy(node->offsets, &binary[offset + layout->offsets_offset], layout->offsets_size);

    if (!btree_node_is_leaf(node)) {
        size_t children_offset = offset + layout->children_offset;
        for (size_t i = 0; i <= node->key_count; i++) {
            FileOffset child_offset;
            memcpy(&child_offset,
                   &binary[(children_offset + i * sizeof(child_offset))],
                   sizeof(child_offset));
            node->children[i] = parse_tree(layout, binary, node, child_offset);
        }
    }

    return node;
}

void free_node_recurse(BtreeNode* node)
{
    if (!node)
        return;

    if (!btree_node_is_leaf(node)) {
        for (size_t i = 0; i <= node->key_count; i++)
            free_node_recurse(node->children[i]);
    }
    free(node->keys);
    free(node->offsets);
    free(node->children);
    free(node);
}

void free_node(BtreeNode* node)
{
    if (!node)
        return;

    free(node->keys);
    free(node->offsets);
    free(node->children);
    free(node);
}

void btree_free(void* data)
{
    Btree* btree = data;
    free_node_recurse(btree->root);
    free(btree->config);
    free(btree);
}

void btree_load(void* config, FileName filename, void** data)
{
    BtreeConfig* btree_config = config;
    bool error = true;
    Btree* tree = calloc(1, sizeof(Btree));
    tree->layout = btree_layout_create(btree_config->node_size, btree_config->key_size);
    tree->compare = btree_config->comparator;
    tree->ctx = &btree_config->comparator_ctx;
    tree->config = btree_config;

    uint8_t* binary = nullptr;
    FILE* file = nullptr;
    open_file(filename, "r", &file, ERR_STORAGE, "Cannot open index file");

    fseeko(file, 0, SEEK_END);
    off_t file_end_offset = ftello(file);
    if (file_end_offset == 0) {
        tree->root = nullptr;
        *data = tree;
        goto cleanup;
    }
    size_t file_size = (size_t)file_end_offset;

    fseeko(file, 0, SEEK_SET);

    binary = malloc(file_size);
    size_t size_read = fread(binary, 1, file_size, file);

    if (file_size != size_read)
        goto cleanup;

    tree->root = parse_tree(&tree->layout, binary, nullptr, 0);
    *data = tree;
    error = false;

cleanup:
    if (binary)
        free(binary);
    if (file)
        fclose(file);
    if (error)
        btree_free(tree);
}

size_t btree_to_bin(BtreeLayout* layout, BtreeNode* node, FILE* file, size_t* offset)
{
    size_t current_offset = *offset;
    if (node == nullptr) {
        ftruncate(fileno(file), 0);
        rewind(file);
        return current_offset;
    }

    *offset += layout->node_size;

    size_t children_offsets[layout->child_count];
    memset(&children_offsets, 0, layout->children_size);
    if (!btree_node_is_leaf(node))
        for (size_t i = 0; i <= node->key_count; i++)
            children_offsets[i] = btree_to_bin(layout, node->children[i], file, offset);

    uint8_t buffer[layout->node_size];
    memset(buffer, 0, layout->node_size);
    memcpy(buffer, &node->flags, layout->flags_size);
    memcpy(&buffer[layout->key_count_offset], &node->key_count, layout->key_count_size);
    memcpy(&buffer[layout->keys_offset], node->keys, layout->keys_size);
    memcpy(&buffer[layout->offsets_offset], node->offsets, layout->offsets_size);
    memcpy(&buffer[layout->children_offset], children_offsets, layout->children_size);

    fseeko(file, (long)current_offset, SEEK_SET);
    fwrite(buffer, 1, layout->node_size, file);
    return current_offset;
}

void btree_save(void* data, FileName filename)
{
    Btree* btree = data;

    FILE* file = nullptr;
    open_file(filename, "w", &file, ERR_STORAGE, "Cannot create index file");
    size_t offset = 0;
    btree_to_bin(&btree->layout, btree->root, file, &offset);
    fflush(file);
    fclose(file);
}

void* key_at(uint8_t* keys, uint16_t key_size, uint16_t i) { return &keys[i * key_size]; }

void set_key(uint8_t* keys, uint16_t key_size, uint16_t i, const void* key)
{
    memcpy(key_at(keys, key_size, i), key, key_size);
}

void* node_key(BtreeLayout* layout, BtreeNode* node, uint16_t i)
{
    return key_at(node->keys, layout->key_size, i);
}

void node_set_key(BtreeLayout* layout, BtreeNode* node, uint16_t i, const void* key)
{
    memcpy(key_at(node->keys, layout->key_size, i), key, layout->key_size);
}

BtreeNode* find_leaf(Btree* tree, const void* key)
{
    BtreeLayout* btree_layout = &tree->layout;
    BtreeNode* candidate = tree->root;
    while (!btree_node_is_leaf(candidate)) {
        uint16_t c = candidate->key_count;
        for (uint16_t i = 0; i < candidate->key_count; i++) {
            if (tree->compare(key, node_key(btree_layout, candidate, i), tree->ctx) <= 0) {
                c = i;
                break;
            }
        }
        candidate = candidate->children[c];
    }

    return candidate;
}

uint16_t insert_value(Btree* tree,
                      uint8_t* keys,
                      FileOffset* offsets,
                      uint16_t value_count,
                      const void* key,
                      FileOffset offset)
{
    uint16_t key_size = tree->layout.key_size;
    uint16_t i;
    for (i = 0; i < value_count; i++) {
        if (tree->compare(key, key_at(keys, key_size, i), tree->ctx) <= 0)
            break;
    }

    for (uint16_t j = value_count; j > i; j--) {
        set_key(keys, key_size, j, key_at(keys, key_size, j - 1));
        offsets[j] = offsets[j - 1];
    }
    set_key(keys, key_size, i, key);
    offsets[i] = offset;

    return i;
}

void insert_child(BtreeLayout* layout,
                  BtreeNode* node,
                  BtreeNode** tmp_children,
                  uint16_t idx,
                  BtreeNode* new_child)
{
    memcpy(tmp_children, node->children, layout->child_size * layout->child_count);
    for (uint16_t i = layout->child_count; i > idx; i--)
        tmp_children[i] = tmp_children[i - 1];
    tmp_children[idx] = new_child;
}

bool insert_into_leaf(Btree* tree, BtreeNode* leaf, const void* key, FileOffset offset)
{
    if (leaf->key_count >= tree->layout.max_keys)
        return false;

    insert_value(tree, leaf->keys, leaf->offsets, leaf->key_count, key, offset);
    leaf->key_count++;

    return true;
}

bool insert_into_node(Btree* tree,
                      BtreeNode* parent,
                      BtreeNode* child,
                      const void* key,
                      FileOffset offset)
{
    if (parent->key_count >= tree->layout.max_keys)
        return false;

    uint16_t idx = insert_value(tree,
                                parent->keys,
                                parent->offsets,
                                parent->key_count,
                                key,
                                offset);
    parent->key_count++;
    for (uint16_t i = parent->key_count; i > idx + 1; i--)
        parent->children[i] = parent->children[i - 1];
    parent->children[idx + 1] = child;
    child->parent = parent;

    return true;
}

BtreeNode* create_root(BtreeLayout* layout,
                       BtreeNode* left,
                       BtreeNode* right,
                       const void* key,
                       FileOffset offset)
{
    BtreeNode* root = allocate_node(layout);
    root->parent = nullptr;
    root->children[0] = left;
    root->children[1] = right;
    node_set_key(layout, root, 0, key);
    root->key_count = 1;
    root->offsets[0] = offset;
    root->flags = 0;
    left->parent = root;
    right->parent = root;

    return root;
}

void split_node(Btree* tree,
                BtreeNode* node,
                BtreeNode* right,
                const void* key,
                FileOffset offset,
                uint8_t* tmp_keys,
                FileOffset* tmp_offsets,
                BtreeNode* incoming_child)
{
    BtreeLayout layout = tree->layout;
    memcpy(tmp_keys, node->keys, layout.key_size * layout.max_keys);
    memcpy(tmp_offsets, node->offsets, layout.offset_size * layout.max_keys);
    uint16_t idx = insert_value(tree, tmp_keys, tmp_offsets, layout.max_keys, key, offset);

    node->key_count = layout.left_count;
    memcpy(node->keys, tmp_keys, layout.key_size * layout.left_count);
    memcpy(node->offsets, tmp_offsets, layout.offset_size * layout.left_count);

    right->parent = node->parent;
    right->key_count = layout.right_count;
    right->flags = node->flags;
    memcpy(right->keys,
           key_at(tmp_keys, layout.key_size, layout.right_start),
           layout.key_size * layout.right_count);
    memcpy(right->offsets,
           &tmp_offsets[layout.right_start],
           layout.offset_size * layout.right_count);

    if (!btree_node_is_leaf(right)) {
        BtreeNode* tmp_children[layout.child_count + 1];
        insert_child(&layout, node, tmp_children, idx + 1, incoming_child);
        memcpy(node->children, tmp_children, layout.child_size * (layout.left_count + 1));
        memcpy(right->children,
               &tmp_children[layout.right_start],
               layout.child_size * (layout.right_count + 1));
        for (uint16_t i = 0; i <= layout.right_count; i++)
            right->children[i]->parent = right;
    }
}

BtreeNode* create_leaf(BtreeLayout* layout, const void* key, const FileOffset offset)
{
    BtreeNode* node = allocate_node(layout);
    node->parent = nullptr;
    node_set_key(layout, node, 0, key);
    node->key_count = 1;
    node->offsets[0] = offset;
    node->flags = BTREE_NODE_LEAF;

    return node;
}

BtreeNode* btree_insert_do(Btree* tree, const void* key, FileOffset offset)
{
    BtreeLayout* layout = &tree->layout;
    BtreeNode* root = tree->root;
    if (root == nullptr)
        return create_leaf(layout, key, offset);

    BtreeNode* node = find_leaf(tree, key);
    const bool insert_leaf_success = insert_into_leaf(tree, node, key, offset);
    if (!insert_leaf_success) {
        BtreeNode* right = allocate_node(layout);
        uint8_t tmp_keys[layout->key_size * (layout->max_keys + 1)];
        FileOffset tmp_offsets[layout->max_keys + 1];

        split_node(tree, node, right, key, offset, tmp_keys, tmp_offsets, nullptr);

        if (node->parent == nullptr)
            return create_root(layout,
                               node,
                               right,
                               key_at(tmp_keys, layout->key_size, layout->mid),
                               tmp_offsets[layout->mid]);

        bool insert_node_success = insert_into_node(tree,
                                                    node->parent,
                                                    right,
                                                    key_at(tmp_keys, layout->key_size, layout->mid),
                                                    tmp_offsets[layout->mid]);
        if (insert_node_success)
            return root;

        while (node->parent != nullptr && !insert_node_success) {
            BtreeNode* incoming_child = right;
            right = allocate_node(layout);
            node = node->parent;
            uint8_t key_copy[layout->key_size];
            memcpy(key_copy, key_at(tmp_keys, layout->key_size, layout->mid), layout->key_size);

            split_node(tree,
                       node,
                       right,
                       key_copy,
                       tmp_offsets[layout->mid],
                       tmp_keys,
                       tmp_offsets,
                       incoming_child);

            if (node->parent == nullptr)
                return create_root(layout,
                                   node,
                                   right,
                                   key_at(tmp_keys, layout->key_size, layout->mid),
                                   tmp_offsets[layout->mid]);

            insert_node_success = insert_into_node(tree,
                                                   node->parent,
                                                   right,
                                                   key_at(tmp_keys, layout->key_size, layout->mid),
                                                   tmp_offsets[layout->mid]);
        }
    }

    return root;
}

void btree_insert(void* data, const void* key, FileOffset offset)
{
    Btree* btree = data;
    btree->root = btree_insert_do(btree, key, offset);
}

bool btree_search(void* data, const void* key, FileOffset* offset)
{
    Btree* tree = data;
    BtreeNode* node = tree->root;
    BtreeLayout* layout = &tree->layout;

    if (node == nullptr)
        return false;

    bool is_leaf;
    do {
        uint16_t c = node->key_count;
        for (uint16_t i = 0; i < node->key_count; i++) {
            int compare = tree->compare(key, node_key(layout, node, i), tree->ctx);
            if (compare == 0) {
                *offset = node->offsets[i];
                return true;
            }
            if (compare < 0) {
                c = i;
                break;
            }
        }
        is_leaf = btree_node_is_leaf(node);
        node = node->children[c];
    } while (!is_leaf);

    return false;
}

void btree_open(FileName file_name, void** data)
{
    open_file(file_name, "r", (FILE**)data, ERR_INDEX, "Cannot open index file");
}

bool btree_search_stream(void* config, void* data, const void* key, FileOffset* offset)
{
    BtreeConfig* btree_config = config;
    KeyComparator comparator = btree_config->comparator;
    void* comparator_ctx = &btree_config->comparator_ctx;
    uint16_t key_size = btree_config->key_size;

    BtreeLayout layout = btree_layout_create(btree_config->node_size, btree_config->key_size);
    FILE* btree_file = data;
    off_t node_offset = 0;
    uint8_t* buf = malloc(layout.node_size);
    bool is_leaf;
    bool result = false;
    do {
        fseeko(btree_file, node_offset, SEEK_SET);
        fread(buf, 1, layout.node_size, btree_file);

        is_leaf = flag_is_leaf(buf[0]);
        uint16_t key_count = 0;
        uint8_t keys[layout.keys_size];
        FileOffset offsets[layout.max_keys];
        off_t children[layout.child_count];

        memcpy(&key_count, &buf[layout.key_count_offset], layout.key_count_size);
        memcpy(&keys, &buf[layout.keys_offset], layout.keys_size);
        memcpy(&offsets, &buf[layout.offsets_offset], layout.offsets_size);
        memcpy(&children, &buf[layout.children_offset], layout.children_size);

        uint16_t pick_child = key_count;
        for (uint16_t i = 0; i < key_count; i++) {
            int compare = comparator(key, key_at(keys, key_size, i), comparator_ctx);
            if (compare == 0) {
                *offset = offsets[i];
                result = true;
                goto cleanup;
            }
            if (compare < 0) {
                pick_child = i;
                break;
            }
        }
        node_offset = children[pick_child];
    } while (!is_leaf);

cleanup:
    free(buf);

    return result;
}

BtreeNode* find_node(Btree* tree, BtreeNode* node, const void* key, uint16_t* pos)
{
    uint16_t key_size = tree->layout.key_size;
    uint16_t child_pos = node->key_count;
    for (uint16_t i = 0; i < node->key_count; i++) {
        int compare = tree->compare(key, key_at(node->keys, key_size, i), tree->ctx);
        if (compare == 0) {
            *pos = i;
            return node;
        }
        if (compare < 0) {
            child_pos = i;
            break;
        }
    }

    if (!btree_node_is_leaf(node))
        return find_node(tree, node->children[child_pos], key, pos);

    return nullptr;
}

BtreeNode* find_predecessor(BtreeNode* node)
{
    while (!btree_node_is_leaf(node))
        node = node->children[node->key_count];

    return node;
}

void remove_from_leaf(Btree* tree, BtreeNode* node, uint16_t pos)
{
    uint16_t key_size = tree->layout.key_size;
    if (btree_node_is_leaf(node)) {
        node->key_count--;
        if (node->key_count > pos) {
            memmove(key_at(node->keys, key_size, pos),
                    key_at(node->keys, key_size, pos + 1),
                    key_size * (node->key_count - pos));
            memmove(node->offsets + pos,
                    node->offsets + pos + 1,
                    tree->layout.offset_size * (node->key_count - pos));
        }
    }
}

void btree_steal_left(Btree* tree,
                      BtreeNode* parent,
                      uint16_t key_index,
                      BtreeNode* child,
                      BtreeNode* left)
{
    BtreeLayout* layout = &tree->layout;

    memmove(child->keys + layout->key_size, child->keys, child->key_count * layout->key_size);
    memmove(child->offsets + 1, child->offsets, child->key_count * layout->offset_size);
    if (!btree_node_is_leaf(child))
        memmove(child->children + 1, child->children, (child->key_count + 1) * layout->child_size);
    child->key_count++;

    void* parent_key = node_key(layout, parent, key_index);
    node_set_key(layout, child, 0, parent_key);
    child->offsets[0] = parent->offsets[key_index];
    if (!btree_node_is_leaf(child)) {
        child->children[0] = left->children[left->key_count];
        child->children[0]->parent = child;
    }

    uint16_t rightmost_index = left->key_count - 1;
    void* rightmost_key = node_key(layout, left, rightmost_index);
    node_set_key(layout, parent, key_index, rightmost_key);
    parent->offsets[key_index] = left->offsets[rightmost_index];
    left->key_count--;
}

void btree_steal_right(Btree* tree,
                       BtreeNode* parent,
                       uint16_t key_index,
                       BtreeNode* child,
                       BtreeNode* right)
{
    BtreeLayout* layout = &tree->layout;

    void* parent_key = node_key(layout, parent, key_index);
    node_set_key(layout, child, child->key_count, parent_key);
    child->offsets[child->key_count] = parent->offsets[key_index];
    if (!btree_node_is_leaf(child)) {
        child->children[child->key_count + 1] = right->children[0];
        child->children[child->key_count + 1]->parent = child;
    }
    child->key_count++;

    void* leftmost_key = node_key(layout, right, 0);
    node_set_key(layout, parent, key_index, leftmost_key);
    parent->offsets[key_index] = right->offsets[0];

    memmove(right->keys, right->keys + layout->key_size, right->key_count * layout->key_size);
    memmove(right->offsets, right->offsets + 1, right->key_count * layout->offset_size);
    if (!btree_node_is_leaf(right))
        memmove(right->children, right->children + 1, (right->key_count + 1) * layout->child_size);
    right->key_count--;
}

void btree_merge(Btree* tree, BtreeNode* parent, uint16_t left_index)
{
    BtreeLayout* layout = &tree->layout;

    uint16_t right_index = left_index + 1;
    BtreeNode* left = parent->children[left_index];
    BtreeNode* right = parent->children[right_index];

    node_set_key(layout, left, left->key_count, node_key(layout, parent, left_index));
    left->offsets[left->key_count] = parent->offsets[left_index];
    left->key_count++;

    memcpy(node_key(layout, left, left->key_count),
           node_key(layout, right, 0),
           right->key_count * layout->key_size);
    memcpy(&left->offsets[left->key_count], right->offsets, right->key_count * layout->offset_size);
    if (!btree_node_is_leaf(left)) {
        uint16_t left_child_count = left->key_count + 1;
        uint16_t right_child_count = right->key_count + 1;
        memcpy(&left->children[left_child_count],
               right->children,
               right_child_count * layout->child_size);
        for (uint16_t i = left_child_count; i < left_child_count + right_child_count; i++)
            left->children[i]->parent = left;
    }
    left->key_count += right->key_count;

    uint16_t parent_shift_count = parent->key_count - right_index;
    if (parent_shift_count > 0) {
        memmove(node_key(layout, parent, left_index),
                node_key(layout, parent, right_index),
                parent_shift_count * layout->key_size);
        memmove(&parent->offsets[left_index],
                &parent->offsets[right_index],
                parent_shift_count * layout->offset_size);
        memmove(&parent->children[right_index],
                &parent->children[right_index + 1],
                parent_shift_count * layout->child_size);
    }
    parent->key_count--;

    free_node(right);
}

void delete_recursive(Btree* tree, BtreeNode* node, const void* key);

void ensure_min_keys(Btree* tree, BtreeNode* parent, uint16_t child_index)
{
    if (parent->children[child_index]->key_count > tree->layout.min_keys)
        return;

    if (child_index > 0)
        if (parent->children[child_index - 1]->key_count > tree->layout.min_keys) {
            btree_steal_left(tree,
                         parent,
                         child_index - 1,
                         parent->children[child_index],
                         parent->children[child_index - 1]);
            return;
        }

    if (child_index < parent->key_count)
        if (parent->children[child_index + 1]->key_count > tree->layout.min_keys) {
            btree_steal_right(tree,
                              parent,
                              child_index,
                              parent->children[child_index],
                              parent->children[child_index + 1]);
            return;
        }

    uint16_t left_index = child_index > 0 ? child_index - 1 : child_index;
    btree_merge(tree, parent, left_index);
}

bool node_key_position(Btree* tree, BtreeNode* node, const void* key, uint16_t* pos)
{
    BtreeLayout* layout = &tree->layout;
    uint16_t insert_pos = node->key_count;
    for (uint16_t i = 0; i < node->key_count; i++) {
        int compare = tree->compare(key, node_key(layout, node, i), tree->ctx);
        if (compare == 0) {
            *pos = i;
            return true;
        }
        if (compare < 0) {
            insert_pos = i;
            break;
        }
    }
    *pos = insert_pos;
    return false;
}

void delete_recursive(Btree* tree, BtreeNode* node, const void* key)
{
    BtreeLayout* layout = &tree->layout;
    uint16_t pos;
    if (node_key_position(tree, node, key, &pos)) {
        if (btree_node_is_leaf(node)) {
            remove_from_leaf(tree, node, pos);
            return;
        }

        ensure_min_keys(tree, node, pos);
        node_key_position(tree, node, key, &pos);

        BtreeNode* child = node->children[pos];
        uint8_t tmp_key[layout->key_size];
        memcpy(&tmp_key, node_key(layout, child, child->key_count - 1), layout->key_size);
        node_set_key(layout, node, pos, tmp_key);
        node->offsets[pos] = child->offsets[child->key_count - 1];

        delete_recursive(tree, child, tmp_key);
    } else {
        ensure_min_keys(tree, node, pos);
        node_key_position(tree, node, key, &pos);

        delete_recursive(tree, node->children[pos], key);
    }
}

bool btree_delete(void* data, const void* key, [[maybe_unused]] FileOffset offset)
{
    Btree* tree = data;
    FileOffset found_offset;
    if (!btree_search(data, key, &found_offset))
        return false;

    delete_recursive(tree, tree->root, key);

    if (tree->root->key_count == 0 && !btree_node_is_leaf(tree->root)) {
        BtreeNode* tmp = tree->root;
        tree->root = tree->root->children[0];
        tree->root->parent = nullptr;
        free_node(tmp);
    }

    return true;
}