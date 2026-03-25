//
// Created by prem on 3/23/26.
//

#include "bplus.h"
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>

int min(int a,int b)
{
    return a<b?a:b;
}

int get_min(BPlusNode * node)
{
    if (node->key_count == 0) return -1;

    return node->keys[0];
}

bool is_full(BPlusNode *node)
{
    return node->key_count == MAX_KEYS;
}

int get_expected_position(BPlusNode *node, int key)
{
    int pos = 0;
    while (pos < node->key_count && node->keys[pos] < key) pos++;

    return pos;
}

Student * search_bplus(BPlusNode * root , int key)
{
    if (root == NULL) return NULL;

    int pos= 0;
    BPlusNode * curr = root;

    while (!curr->is_leaf)
    {
        pos = 0;
        while (pos < curr->key_count && curr->keys[pos] <= key) pos++;
        curr = curr->children[pos];

    }

    for (int i=0;i<curr->key_count;i++)
    {
        if (curr->keys[i] == key)
        {
            return &curr->records[i];
        }
    }

    return NULL;

}

BPlusNode * create_bplus_node(bool is_leaf)
{
    BPlusNode * node = (BPlusNode*)calloc(1 ,sizeof(BPlusNode));
    node->next =NULL;
    node->is_leaf = is_leaf;

    return node;
}

void print_student(Student s)
{
    printf("(id='%d'  , name='%s' , age='%d')\n" , s.id, s.name , s.age);
}

void print_leaf_chain(BPlusNode *root) {
    BPlusNode *curr = root;
    while (!curr->is_leaf)
        curr = curr->children[0];

    printf("Leaf chain: ");
    while (curr != NULL) {
        printf("[");
        for (int i = 0; i < curr->key_count; i++)
            printf("%d ", curr->keys[i]);
        printf("] -> ");
        curr = curr->next;
    }
    printf("NULL\n");
}

void get_students_between(BPlusNode * root , int start_id , int end_id )
{
    int pos;
    BPlusNode * curr = root;
    while (!curr->is_leaf)
    {
        pos = get_expected_position(curr , start_id);
        curr = curr->children[pos];
    }

    pos = get_expected_position(curr , start_id);


    BPlusNode * p = curr;

    for (int i=pos;i<p->key_count;i++)
    {
        if (p->keys[i] > end_id) break;
        printf("Key : %d " , p->keys[i]);
        print_student(p->records[i]);
    }
    p=p->next;
    while (p!=NULL)
    {
        for (int i=0;i<p->key_count;i++)
        {
            if (p->keys[i] > end_id) break;
            printf("Key : %d " , p->keys[i]);
            print_student(p->records[i]);
        }
        p=p->next;
    }

}

void inorder(BPlusNode * node)
{
    if (node ==NULL)
    {
        printf("Empty tree....\n");
        return;
    }

    BPlusNode * curr = node;
    while (!curr->is_leaf)
    {
        curr = curr->children[0];
    }

    BPlusNode * p = curr;
    while (p != NULL)
    {
        for (int i=0;i<p->key_count;i++)
        {
            printf("Key : %d " , p->keys[i]);
            print_student(p->records[i]);
        }

        p = p->next;
    }

    return;
}

void print_bplus_tree(BPlusNode *root, int level) {
    if (!root) return;

    for (int i = root->key_count; i >= 0; i--) {
        if (!root->is_leaf)
            print_bplus_tree(root->children[i], level + 1);

        if (i < root->key_count) {
            for (int j = 0; j < level; j++) printf("    ");
            printf("%d\n", root->keys[i]);
        }
    }
}
void split_leaf(BPlusNode *parent, int found_idx)
{

    // get the leaf which is full
    BPlusNode * full_leaf = parent->children[found_idx];

    // store the next pointer of full_leaf before splitting
    BPlusNode * break_point_list =full_leaf->next;

    // get size of leaf
    int size_of_leaf = full_leaf->key_count;

    // if new element got inserted in full_leaf , size of leaf would be
    int temp_size = size_of_leaf + 1;

    // median idx as per  ideal leaf size
    int median_idx = temp_size/2;

    // median element as per  ideal leaf size
    int median_elem = full_leaf->keys[median_idx];

    // create a new node
    BPlusNode  * right_leaf = create_bplus_node(true);


    // assign size 0
    int right_leaf_size = 0;

    // elements from 'median_idx' to full_lead.key_count get moved to right leaf from full_leaf
    for (int i=median_idx;i<full_leaf->key_count;i++)
    {
        right_leaf->keys[i - median_idx] = full_leaf->keys[i];
        right_leaf->records[i - median_idx] = full_leaf->records[i];
        right_leaf_size++;
    }

    // update new size of full leaf
    full_leaf->key_count = median_idx;

    // assign right ass full_leaf.next (full_leaf ------> right_leaf)
    full_leaf->next = right_leaf;

    // assign what was next of full_leaf before splitting as right_leaf.next (full_leaf ------> right_leaf -------> break_point_list)
    right_leaf->next = break_point_list;

    // insert the median elemet in parent
    int pos = get_expected_position(parent , median_elem);

    // shift keys as per expected position of median
    for (int i=parent->key_count-1;i>=pos;i--) parent->keys[i+1] = parent->keys[i];

    // shift children as per expected position of median
    for (int i=parent->key_count;i>=pos;i--) parent->children[i+1] = parent->children[i];
    parent->keys[pos] = median_elem;

    // update key_count of parent by 1
    parent->key_count++;

    // update key_count of right_leaf by its calculated size
    right_leaf->key_count = right_leaf_size;

    // to be safe
    parent->is_leaf = false;

    // assign children of parent
    parent->children[pos] = full_leaf;
    parent->children[pos + 1] = right_leaf;

}

void split_internal(BPlusNode *parent, int found_idx)
{

    BPlusNode * full_child = parent->children[found_idx];

    int size_of_leaf = full_child->key_count;
    int temp_size = size_of_leaf + 1;

    int median_idx = temp_size/2;

    int median_elem = full_child->keys[median_idx];

    BPlusNode  * right_child = create_bplus_node(false);

    int right_leaf_size = 0;

    // elements from 'median_idx' to full_lead.key_count get removed from full_leaf
    for (int i=median_idx + 1;i<full_child->key_count;i++)
    {
        right_child->keys[i - median_idx -1] = full_child->keys[i];
        right_leaf_size++;
    }

    for (int i=median_idx + 1;i<=full_child->key_count;i++)
    {
        right_child->children[i - median_idx -1] = full_child->children[i];
    }
    full_child->key_count = median_idx;

    int pos = get_expected_position(parent , median_elem);

    // insert median element in parent
    for (int i=parent->key_count-1;i>=pos;i--) parent->keys[i+1] = parent->keys[i];
    for (int i=parent->key_count;i>=pos;i--) parent->children[i+1] = parent->children[i];

    parent->keys[pos] = median_elem;
    parent->key_count++;

    right_child->key_count = right_leaf_size;

    parent->children[pos] = full_child;
    parent->children[pos + 1] = right_child;

}

void insert_non_full(BPlusNode * node, int key , Student * record)
{
    int pos = get_expected_position(node , key);

    if (node->is_leaf)
    {
        for (int  i=node->key_count-1;i>=pos;i--)
        {
            node->keys[i+1] = node->keys[i];
            node->records[i+1] = node->records[i];
        }
        node->keys[pos] = key;
        node->records[pos] = *record;
        node->key_count++;

        return;
    }

    BPlusNode * child = node->children[pos];
    if (is_full(child) )
    {
        if (child->is_leaf) split_leaf(node , pos);
        else split_internal(node ,pos);

        pos = get_expected_position(node , key);
        child = node->children[pos];
    }

    insert_non_full(child , key , record);
}

void bplus_insert(BPlusNode **root , int key, Student *record)
{
    if (*root == NULL)
    {
        BPlusNode * new_root  = create_bplus_node(true);
        new_root->keys[0] = key;
        new_root->records[0] = *record;
        new_root->key_count = 1;
        *root = new_root;
        return;
    }

    if (is_full(*root))
    {
        BPlusNode * old_root = *root;
        BPlusNode * new_root = create_bplus_node(false);
        new_root->children[0] = old_root;

        if (old_root->is_leaf)
            split_leaf(new_root, 0);
        else
        {
            split_internal(new_root , 0);
        }

        *root = new_root;

        insert_non_full(*root, key , record);
        return ;
    }

    insert_non_full(*root  , key  , record);
}

int main() {
    BPlusNode *root = NULL;

    // 15 student records
    int ids[]  = {10, 20, 5, 15, 25, 3, 7, 12, 18, 22, 1, 8, 30, 17, 6};
    int ages[] = {20, 22, 19, 21, 23, 18, 20, 22, 21, 19, 17, 20, 25, 21, 19};
    char *names[] = {
        "Alice", "Bob", "Carol", "Dave", "Eve",
        "Frank", "Grace", "Hank", "Ivy", "Jack",
        "Kate", "Leo", "Mia", "Nick", "Olivia"
    };

    int n = 15;
    for (int i = 0; i < n; i++) {
        Student s;
        memset(&s, 0, sizeof(Student));
        s.id  = ids[i];
        s.age = ages[i];
        strcpy(s.name, names[i]);
        bplus_insert(&root, ids[i], &s);
    }

    printf("===== Inorder (all leaves via next ptr) =====\n");
    inorder(root);


    printf("\n");
    int start = 4,end = 30;
    printf("Getting students between id %d & %d : \n" ,start,end);
    get_students_between(root , start , end);

    printf("\n");


    print_leaf_chain(root);


    print_bplus_tree(root, 0);

    printf("\n===== Search =====\n");
    // you write search_bplus — find key in tree, return record
    // test with id=7 (exists) and id=99 (not exists)
    int search_id = 7;
    Student * found = search_bplus(root , search_id);
    if (found == NULL)
    {
        printf("Student with id : %d not found...\n" , search_id);
    }else
    {
        printf("SUCCESS\n");
        print_student(*found);
    }
    return 0;
}
