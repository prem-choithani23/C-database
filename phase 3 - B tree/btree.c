//
// Created by prem on 3/22/26.
//

#include <stdio.h>
#include "btree.h"

#include <stdlib.h>
#include <string.h>

Node * create_node(bool is_leaf)
{
    Node *node = (Node*)malloc(sizeof(Node));
    node->is_leaf = is_leaf;
    memset(node->keys , 0 , MAX_KEYS*sizeof(int));
    memset(node->children , 0 , MAX_CHILDREN*sizeof(Node*));
    node->key_count = 0;

    return node;

}
void free_node(Node * node)
{
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

    free_node(full_child);
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
int main() {
    Node *root = NULL;

    int values[] = {
        10, 20, 5, 6, 12, 30, 7, 17,
        3, 4, 2, 1, 8, 9, 11, 13, 14
    };

    int n = sizeof(values) / sizeof(values[0]);

    for (int i = 0; i < n; i++) {
        insert(&root, values[i]);
    }


    printf("\n===== Sideways Tree =====\n");
    print_btree(root, 0);

    printf("\n===== Inorder (sorted) =====\n");
    inorder(root);
    printf("\n");

    return 0;
}