//
// Created by prem on 3/22/26.
//

#include <stdio.h>
#include "btree.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Node * parent;
    int found_idx;
}Result;

void borrow_from_right(Node *node, int pos);
void borrow_from_left(Node *node, int pos);


Node * create_node(bool is_leaf)
{
    Node *node = (Node*)calloc(1, sizeof(Node));
    node->is_leaf = is_leaf;
    return node;

}

void free_node(Node * node)
{
    if (node == NULL) return;

    for (int i=0;i<=node->key_count;i++)
    {
        if (node->children[i]!=NULL)
            free_node(node->children[i]);
    }
    node->key_count = 0;
    node->is_leaf =false;
    free(node);
}

int get_expected_position(Node *node ,int key)
{
    int pos = 0;
    while (pos < node->key_count && node->keys[pos] < key) pos++;

    return pos;
}

bool is_full(Node * node)
{
    return node->key_count == MAX_KEYS;
}

void split_child(Node * parent , int found_idx , Node * full_child)
{
    int median_idx = full_child->key_count/2;
    int median_element = full_child->keys[median_idx];

    int left_size = median_idx;
    Node * left_child = create_node(full_child->is_leaf);
    for (int i=0;i<left_size;i++)
    {
        left_child->keys[i] = full_child->keys[i];
        left_child->children[i] = full_child->children[i];
    }
    left_child->children[left_size] = full_child->children[left_size];
    left_child->key_count = left_size;

    int right_size = full_child->key_count - median_idx - 1;
    Node *right_child = create_node(full_child->is_leaf);
    for (int i=0;i<right_size;i++)
    {
        right_child->keys[i] = full_child->keys[i + median_idx + 1];
        right_child->children[i] = full_child->children[i+median_idx+ 1];
    }
    right_child->children[right_size] = full_child->children[right_size + median_idx + 1];

    right_child->key_count = right_size;

    for (int i=parent->key_count - 1;i>=found_idx;i--)
    {
        parent->keys[i+1] = parent->keys[i];
        parent->children[i+2] = parent->children[i+1];
    }

    parent->keys[found_idx] = median_element;

    parent->children[found_idx] = left_child;
    parent->children[found_idx+1] = right_child;
    parent->key_count++;

    free(full_child);
}

Node * insert_non_full(Node *node , int key)
{
    int pos = 0;
    if (node->is_leaf)
    {
        while (pos < node->key_count && node->keys[pos] < key) pos++;

        for (int i=node->key_count - 1;i>=pos;i--)
        {
            node->keys[i+1] = node->keys[i];
            // node->children[i+2] = node->children[i+1];
        }

        node->keys[pos] = key;
        node->key_count++;
    }
    else
    {
        int found_idx = get_expected_position(node, key);

        Node * child = node->children[found_idx];
        if (is_full(child))
        {
            split_child(node , found_idx , child);
            if (node->keys[found_idx] < key ) found_idx++;
        }

        insert_non_full(node->children[found_idx] ,key);

    }

    return node;
}

Node * search(Node * root , int key)
{
    if (root == NULL)
    {
        return NULL;
    }
    int index = 0;
    for (index = 0;index < root->key_count;index++)
    {
        if (key == root->keys[index])
        {
            return root;
        }else if (root->keys[index] > key)
        {
            return search(root->children[index] , key);
        }
    }

    if (index == root->key_count)
    {
        return search(root->children[root->key_count]  , key);
    }

    return NULL;
}

Node * get_inorder_predecessor(Node *node , int found_idx)
{
    Node * curr = node->children[found_idx];

    while (!curr->is_leaf)
    {
        curr = curr->children[curr->key_count];
    }

    return curr;
}

Node * get_inorder_successor(Node *node , int found_idx)

{
    Node * curr = node->children[found_idx + 1];

    while (!curr->is_leaf)
    {
        curr = curr->children[0];
    }

    return curr;

}
Node *insert(Node ** root , int key)
{
    if (*root == NULL)
    {
        Node * node = create_node(true);
        *root = node;
        insert_non_full(*root  , key);
        return node;
    }

    if (is_full(*root))
    {
        Node * new_root = create_node(false);
        new_root->children[0] = *root;
        split_child(new_root , 0, *root);
        insert_non_full(new_root,key);
        *root = new_root;
        return new_root;
    }

    return insert_non_full(*root, key);


}

void print_node(Node *node) {
    if (!node) return;

    printf("[");
    for (int i = 0; i < node->key_count; i++) {
        printf("%d", node->keys[i]);
        if (i != node->key_count - 1) printf(" ");
    }
    printf("]");
}

void print_btree(Node *root, int level) {
    if (!root) return;

    for (int i = root->key_count; i >= 0; i--) {
        if (!root->is_leaf)
            print_btree(root->children[i], level + 1);

        if (i < root->key_count) {
            for (int j = 0; j < level; j++) printf("    ");
            printf("%d\n", root->keys[i]);
        }
    }
}

void inorder(Node *root) {
    if (!root) return;

    for (int i = 0; i < root->key_count; i++) {
        if (!root->is_leaf)
            inorder(root->children[i]);

        printf("%d ", root->keys[i]);
    }

    if (!root->is_leaf)
        inorder(root->children[root->key_count]);
}

void merge_children(Node *parent, int found_idx)
{
    Node *left_child  = parent->children[found_idx];
    Node *right_child = parent->children[found_idx + 1];

    int left_size  = left_child->key_count;
    int right_size = right_child->key_count;

    // pull parent separator key down into left child
    left_child->keys[left_size] = parent->keys[found_idx];

    // copy right child's keys and children into left child
    for (int i = 0; i < right_size; i++)
    {
        left_child->keys[left_size + 1 + i] = right_child->keys[i];
        if (!right_child->is_leaf)
            left_child->children[left_size + 1 + i] = right_child->children[i];
    }
    if (!right_child->is_leaf)
        left_child->children[left_size + 1 + right_size] = right_child->children[right_size];

    left_child->key_count = left_size + 1 + right_size;

    // shift parent keys left
    for (int i = found_idx; i < parent->key_count - 1; i++)
        parent->keys[i] = parent->keys[i + 1];

    // shift parent children left (close the right_child gap)
    for (int i = found_idx + 1; i < parent->key_count; i++)
        parent->children[i] = parent->children[i + 1];

    parent->children[parent->key_count] = NULL;
    parent->key_count--;

    free(right_child);   // right absorbed into left — safe to free
    // left_child stays at parent->children[found_idx] — no change needed
}
void delete_recursive(Node *node , int key)
{
    if (!node) {
        return;
    }

    if (!node->is_leaf && node->key_count == 0)
    {
        delete_recursive(node->children[0], key);
        return;
    }


    int pos = get_expected_position(node , key);

    const int MIN_KEYS = (int)ceil(ORDER/2.0) - 1;

    if ( pos < node->key_count && node->keys[pos] == key)
    {

        if (node->is_leaf)
        {

            for (int i=pos;i<node->key_count-1;i++)
                node->keys[i] = node->keys[i+1];

            node->key_count--;

            return;
        }
        else
        {

            Node * predecessor_node = get_inorder_predecessor(node , pos);

            if (!predecessor_node) return;

            int predecessor =
                predecessor_node->keys[predecessor_node->key_count-1];

            if (predecessor_node->key_count > MIN_KEYS)
            {
                node->keys[pos] = predecessor;
                delete_recursive(predecessor_node , predecessor);
                return;
            }

            Node * successor_node = get_inorder_successor(node , pos);

            if (!successor_node) return;

            int successor = successor_node->keys[0];

            if (successor_node->key_count > MIN_KEYS)
            {
                node->keys[pos] = successor;
                delete_recursive(successor_node , successor);
                return;
            }

            merge_children(node ,pos);

            delete_recursive(node->children[pos] , key);
        }
    }
    else
    {

        Node * child = node->children[pos];

        if (!child) {
            return;
        }


        if (child->key_count == MIN_KEYS)
        {
            Node * left_sibling = (pos > 0) ? node->children[pos-1] : NULL;
            Node * right_sibling = (pos < node->key_count) ? node->children[pos+1] : NULL;

            if (left_sibling && left_sibling->key_count > MIN_KEYS)
            {
                borrow_from_left(node, pos);
            }
            else if (right_sibling && right_sibling->key_count > MIN_KEYS)
            {
                borrow_from_right(node, pos);
            }
            else
            {
                if (right_sibling)
                {
                    merge_children(node, pos);
                }
                else
                {
                    merge_children(node, pos - 1);
                    pos = pos - 1;   // IMPORTANT
                }
            }
        }

        delete_recursive(node->children[pos], key);
        return;

    }

}
bool delete(Node **root , int key){

    if (*root == NULL)
    {
        return false;
    }

    delete_recursive(*root  , key);

    if ((*root)->key_count == 0)
    {
        Node * old_root = *root;

        if (!(*root)->is_leaf)
            *root = (*root)->children[0];
        else
            *root = NULL;

        free(old_root);
    }

    return true;

}
void borrow_from_left(Node *node, int pos) {
    Node *child = node->children[pos];
    Node *left  = node->children[pos - 1];

    // shift child keys right to make room
    for (int i = child->key_count; i > 0; i--)
        child->keys[i] = child->keys[i-1];
    if (!child->is_leaf) {
        for (int i = child->key_count + 1; i > 0; i--)
            child->children[i] = child->children[i-1];
    }

    // pull parent key down into child
    child->keys[0] = node->keys[pos - 1];
    child->key_count++;

    // push left sibling's last key up to parent
    node->keys[pos - 1] = left->keys[left->key_count - 1];
    if (!left->is_leaf)
        child->children[0] = left->children[left->key_count];
    left->key_count--;
}
void borrow_from_right(Node * node, int pos)
{
    Node *child = node->children[pos];
    Node *right  = node->children[pos + 1];


    // pull parent key down into child
    child->keys[child->key_count] = node->keys[pos];
    child->key_count++;

    // push left sibling's last key up to parent
    node->keys[pos] = right->keys[0];

    for (int i = 0;i<right->key_count;i++)
    {
        right->keys[i] = right->keys[i+1];
    }
    if (!right->is_leaf)
    {
        Node *dangling = right->children[0];

        for (int i = 0; i < right->key_count; i++)
            right->children[i] = right->children[i+1];

        child->children[child->key_count] = dangling;
    }
    right->key_count--;
}

int main() {
    Node *root = NULL;

    // ── INSERT ────────────────────────────────────────────────────────────
    // Mix of ascending, descending, and interleaved values to stress splits
    int values[] = {
        50, 20, 70, 10, 30, 60, 80,
        5,  15, 25, 35, 55, 65, 75, 85,
        1,  3,  7,  12, 17, 22, 27, 32,
        37, 40, 45, 52, 57, 62, 67, 72,
        77, 82, 87, 90, 95, 100
    };
    int n = sizeof(values) / sizeof(values[0]);

    for (int i = 0; i < n; i++) insert(&root, values[i]);

    printf("After all inserts:\n");
    inorder(root);
    printf("\n\n");

    // ── DELETE: leaf nodes (simple cases) ─────────────────────────────────
    int leaf_deletes[] = {1, 3, 100, 95, 90};
    for (int i = 0; i < 5; i++) {
        delete(&root, leaf_deletes[i]);
        printf("After delete %3d: ", leaf_deletes[i]);
        inorder(root); printf("\n");
    }

    // ── DELETE: internal nodes (forces predecessor/successor borrow) ───────
    int internal_deletes[] = {50, 20, 70};
    for (int i = 0; i < 3; i++) {
        delete(&root, internal_deletes[i]);
        printf("After delete %3d: ", internal_deletes[i]);
        inorder(root); printf("\n");
    }

    // ── DELETE: trigger merges ─────────────────────────────────────────────
    int merge_deletes[] = {5, 7, 10, 12, 15, 17};
    for (int i = 0; i < 6; i++) {
        delete(&root, merge_deletes[i]);
        printf("After delete %3d: ", merge_deletes[i]);
        inorder(root); printf("\n");
    }

    // ── DELETE: cascade merges up to root ─────────────────────────────────
    int cascade_deletes[] = {22, 25, 27, 30, 32, 35};
    for (int i = 0; i < 6; i++) {
        delete(&root, cascade_deletes[i]);
        printf("After delete %3d: ", cascade_deletes[i]);
        inorder(root); printf("\n");
    }

    // ── DELETE: non-existent keys (should not crash) ───────────────────────
    printf("\n-- Non-existent deletes --\n");
    delete(&root, 999);
    delete(&root, 0);
    delete(&root, 50);   // already deleted
    printf("(no crash = good)\n\n");

    // ── DELETE: everything remaining ──────────────────────────────────────
    printf("-- Draining tree --\n");
    int remaining[] = {37, 40, 45, 52, 55, 57, 60, 62, 65, 67,
                       72, 75, 77, 80, 82, 85, 87};
    for (int i = 0; i < 17; i++) {
        delete(&root, remaining[i]);
        printf("After delete %3d: ", remaining[i]);
        inorder(root); printf("\n");
    }

    printf("\nFinal inorder (should be empty): ");
    inorder(root);
    printf("(empty)\n");

    printf("Root is %s\n", root == NULL ? "NULL ✓" : "NOT NULL ✗");

    free_node(root);
    return 0;
}