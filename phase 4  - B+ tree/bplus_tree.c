//
// Created by prem on 3/23/26.
//

#include "bplus.h"
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

void borrow_from_left_leaf(BPlusNode *parent, int idx);
void borrow_from_right_leaf(BPlusNode *parent, int idx);
void borrow_from_left_internal(BPlusNode *parent, int idx);
void borrow_from_right_internal(BPlusNode *parent, int idx);
void merge_leaves(BPlusNode *parent, int idx);
void merge_internal(BPlusNode *parent, int idx);
void bplus_delete(BPlusNode **root, int key);
void bplus_delete_recursive(BPlusNode *node, int key);
void bplus_delete(BPlusNode **root, int key);

int min(int a,int b)
{
    return a<b?a:b;
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

    if (root == NULL)
        return;


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

    int median_elem;
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
    median_elem = full_leaf->keys[median_idx];

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

    // after moving keys to right_leaf
    right_leaf->key_count = right_leaf_size;

    // 🔥 FIX
    median_elem = right_leaf->keys[0];

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

// -----------------------------------
//       DELETE OPERATION FUNCTIONS
// -----------------------------------
/*
* Take left and right leaf around parent->keys[idx]
* Copy right leaf's keys and records into left leaf
* Fix next pointer: left->next = right->next
* Remove parent->keys[idx] from parent, shift keys and children
* free(right)
*/
void update_routing_key(BPlusNode *node, int pos)
{
    if (!node) return;

    // ensure valid bounds
    if (pos <= 0 || pos > node->key_count)
        return;

    BPlusNode *child = node->children[pos];

    // 🚨 critical null check
    if (child == NULL || child->key_count == 0)
        return;

    node->keys[pos - 1] = child->keys[0];
}
void delete_shift_keys_and_children_from(BPlusNode *node, int idx)
{
    // shift keys
    for (int i = idx; i < node->key_count - 1; i++)
        node->keys[i] = node->keys[i + 1];

    // shift children (REMOVE child[idx+1])
    for (int i = idx + 1; i <= node->key_count; i++)
        node->children[i] = node->children[i + 1];

    node->children[node->key_count] = NULL;

    node->key_count--;
}

void delete_shift_keys_and_record_from(BPlusNode * node  , int idx)
{
    for (int i=idx;i<node->key_count-1;i++)
    {
        node->keys[i] = node->keys[i+1];
        node->records[i] = node->records[i+1];

    }
    node->key_count--;
}

void delete_shift_keys_from(BPlusNode * node  , int idx)
{
    for (int i=idx;i<node->key_count-1;i++)
    {
        node->keys[i] = node->keys[i+1];
    }
    node->key_count--;
}

void add_shift_keys_and_children_from(BPlusNode * node  , int idx)
{
    for (int i=node->key_count - 1;i>= idx;i--)
    {
        node->keys[i + 1] = node->keys[i];
    }
    for (int i=node->key_count;i>= idx;i--)
    {
        node->children[i + 1] = node->children[i];
    }
    node->key_count++;
}

int find_child_index(BPlusNode *node, int key)
{
    int pos = 0;
    while (pos < node->key_count && node->keys[pos] <= key)
        pos++;
    return pos;
}

void add_shift_keys_and_records_from(BPlusNode * node  , int idx)
{
    for (int i=node->key_count - 1;i>= idx;i--)
    {
        node->keys[i + 1] = node->keys[i];
        node->records[i + 1] = node->records[i];
    }
    node->key_count++;
}

void merge_leaves(BPlusNode *parent, int idx)
{
    BPlusNode * left_leaf = parent->children[idx];
    BPlusNode * right_leaf = parent->children[idx + 1];

    int left_size = left_leaf->key_count;
    int right_size = right_leaf->key_count;

    BPlusNode * break_point_chain  = right_leaf->next;

    // Copy right leaf's keys and records into left leaf
    for (int i=0;i<right_size;i++)
    {
        left_leaf->keys[i + left_size] = right_leaf->keys[i];
        left_leaf->records[i+left_size] = right_leaf->records[i];
    }

    left_leaf->key_count += right_size;

    // Fix next pointer: left->next = right->next
    left_leaf->next = break_point_chain;

    // Remove parent->keys[idx] from parent, shift keys and children
    delete_shift_keys_and_children_from(parent , idx);

    free(right_leaf);
    parent->children[parent->key_count + 1] = NULL;
}

/*
* No next pointer
* The separator key from parent comes down into the merged node (unlike leaf merge where it's just deleted)
* Children pointers must be copied too
 */
void merge_internal(BPlusNode *parent, int idx)
{
    BPlusNode * left_child = parent->children[idx];
    BPlusNode * right_child = parent->children[idx + 1];

    int separator_key = parent->keys[idx];

    int left_size = left_child->key_count;
    int right_size = right_child->key_count;

    // insert separator key in left child
    left_child->keys[left_size] = separator_key;
    left_size++;

    // Copy right leaf's keys into left leaf
    for (int i=0;i<right_size;i++)
    {
        left_child->keys[i + left_size ] = right_child->keys[i];
    }
    for (int i=0;i<=right_size;i++)
    {
        left_child->children[i + left_size ] = right_child->children[i];
    }
    left_child->key_count += right_size + 1;

    // Remove parent->keys[idx] from parent, shift keys and children
    delete_shift_keys_and_children_from(parent , idx);

    free(right_child);
}

// Borrow from left means:
//
// Take the last key+record from left sibling
// Shift all keys+records in current leaf right by 1
// Insert borrowed key+record at index 0
// Update parent routing key at idx-1 to new first key of current leaf
void borrow_from_left_leaf(BPlusNode *parent, int idx)
{
    BPlusNode * current_node = parent->children[idx];
    BPlusNode * left_sibling = parent->children[idx - 1];

    int left_size = left_sibling->key_count;

    int shift_key = left_sibling->keys[left_size - 1];
    Student shift_record = left_sibling->records[left_size - 1];

    left_sibling->key_count--;

    add_shift_keys_and_records_from(current_node , 0);

    current_node->keys[0] = shift_key;
    current_node->records[0] = shift_record;

    // 🔥 FIX: update correct parent key
    parent->keys[idx - 1] = current_node->keys[0];
}

void borrow_from_right_leaf(BPlusNode *parent, int idx)
{
    BPlusNode * right_sibling = parent->children[idx + 1];
    BPlusNode * current_node = parent->children[idx];


    int shift_key = right_sibling->keys[0];
    Student shift_record = right_sibling->records[0];

    delete_shift_keys_and_record_from(right_sibling , 0);

    int curr_size = current_node->key_count;

    current_node->keys[curr_size] = shift_key;
    current_node->records[curr_size] = shift_record;
    current_node->key_count++;

    if (right_sibling->key_count > 0)
        parent->keys[idx] = right_sibling->keys[0];
}

void borrow_from_left_internal(BPlusNode *parent, int idx)
{
    BPlusNode * current_node = parent->children[idx];
    BPlusNode * left_sibling = parent->children[idx - 1];

    int left_size = left_sibling->key_count;

    int shift_key = left_sibling->keys[left_size - 1];
    BPlusNode * shift_child = left_sibling->children[left_size];

    left_sibling->key_count--;

    add_shift_keys_and_children_from(current_node , 0);

    current_node->keys[0] = parent->keys[idx - 1];
    current_node->children[0] = shift_child;

    // 🔥 FIX
    parent->keys[idx - 1] = shift_key;
}
void borrow_from_right_internal(BPlusNode *parent, int idx)
{
    BPlusNode * right_sibling = parent->children[idx + 1];
    BPlusNode * current_node = parent->children[idx];

    int shift_key = right_sibling->keys[0];
    BPlusNode * shift_child = right_sibling->children[0];

    // shift keys
    for (int i = 0; i < right_sibling->key_count - 1; i++)
        right_sibling->keys[i] = right_sibling->keys[i + 1];

    // shift children
    for (int i = 0; i < right_sibling->key_count; i++)
        right_sibling->children[i] = right_sibling->children[i + 1];

    right_sibling->children[right_sibling->key_count] = NULL;
    right_sibling->key_count--;

    int curr_size = current_node->key_count;

    current_node->keys[curr_size] = parent->keys[idx];
    current_node->children[curr_size + 1] = shift_child;
    current_node->key_count++;

    parent->keys[idx] = shift_key;
}

void bplus_delete_recursive(BPlusNode *node, int key)
{
    printf("[DELETE] node=%p key=%d key_count=%d is_leaf=%d\n",
       node, key, node->key_count, node->is_leaf);

    const int MIN_KEY = (ORDER - 1)/2;
    int pos = find_child_index(node , key);
    printf("[POS] pos=%d key_count=%d\n", pos, node->key_count);
    printf("[CHILD] child=%p\n", node->children[pos]);

    BPlusNode * child = node->children[pos];

    if (node->is_leaf)
    {
        printf("Leaf hai...\n");
        int i = 0;
        while (i < node->key_count && node->keys[i] < key) i++;

        if (i == node->key_count || node->keys[i] != key)
        {
            printf("Key %d not found\n", key);
            return;
        }

        delete_shift_keys_and_record_from(node, i);

        return;
    }

    if (child->is_leaf)
    {
        if (child->key_count > MIN_KEY)
        {
            bplus_delete_recursive(child , key);

            //update routing key
            // int new_pos = find_child_index(node, key);
            // update_routing_key(node , new_pos);

            return;
        }
        else
        {
            BPlusNode * left_sibling = pos == 0 ? NULL : node->children[pos - 1];
            BPlusNode * right_sibling = pos == node->key_count ? NULL : node->children[pos  + 1];

            if (left_sibling && left_sibling->key_count > MIN_KEY)
            {
                borrow_from_left_leaf(node, pos);

                int new_pos = find_child_index(node, key);
                bplus_delete_recursive(node->children[new_pos], key);

                // update_routing_key(node, new_pos);
                return;
            }
            else if (right_sibling && right_sibling->key_count > MIN_KEY)
            {
                borrow_from_right_leaf(node , pos);

                int new_pos = find_child_index(node, key);
                bplus_delete_recursive(node->children[new_pos], key);

                // update_routing_key(node, new_pos);
                return;
            }



            if (right_sibling)  merge_leaves(node , pos);
            else merge_leaves(node , pos - 1);

            int new_pos =find_child_index(node ,key);

            bplus_delete_recursive(node->children[new_pos], key);
            // update_routing_key(node , new_pos);
            return;
        }
    }
    else
    {
        if (child->key_count > MIN_KEY)
        {
            bplus_delete_recursive(child , key);

            int new_pos = find_child_index(node, key);

            // update routing key if needed
            // update_routing_key(node , new_pos);

        }
        else
        {
            BPlusNode * left_sibling = pos == 0 ? NULL : node->children[pos - 1];
            BPlusNode * right_sibling = pos == node->key_count ? NULL : node->children[pos  + 1];

            if (left_sibling && left_sibling->key_count > MIN_KEY)
            {
                borrow_from_left_internal(node, pos);

                int new_pos = find_child_index(node, key);
                bplus_delete_recursive(node->children[new_pos], key);


                // update_routing_key(node , new_pos);
                return;
            }

            if (right_sibling && right_sibling->key_count > MIN_KEY)
            {
                borrow_from_right_internal(node , pos);

                int new_pos = find_child_index(node, key);
                bplus_delete_recursive(node->children[new_pos], key);

                // update_routing_key(node , new_pos);
                return;
            }

            if (right_sibling)
                merge_internal(node , pos);
            else
                merge_internal(node , pos - 1);

            int new_pos =find_child_index(node ,key);

            // recalculate pos (tree structure might be changed )
            bplus_delete_recursive(node->children[new_pos], key);

            // update_routing_key(node , new_pos);
            return;
        }
    }
}

void bplus_delete(BPlusNode **root, int key)
{
    BPlusNode * curr_root = *root;
    if (curr_root == NULL )
    {
        printf("Tree already empty...\n");
        return;
    }


    bplus_delete_recursive(curr_root,key);

    if (curr_root->key_count == 0)
    {
        if (curr_root->is_leaf)
        {
            free(curr_root);
            *root = NULL;
        }
        else
        {
            *root = curr_root->children[0];
            free(curr_root);
        }
    }
}

// int main() {
//     BPlusNode *root = NULL;
//
//     // 20 unsorted student records
//     int ids[] = {42, 5, 18, 33, 7, 50, 2, 27, 39, 11,
//                  25, 1, 45, 30, 22, 9, 14, 6, 35, 48};
//
//     int ages[] = {21, 19, 22, 20, 18, 23, 17, 21, 22, 20,
//                   19, 18, 24, 23, 21, 20, 19, 18, 22, 25};
//
//     char *names[] = {
//         "A", "B", "C", "D", "E",
//         "F", "G", "H", "I", "J",
//         "K", "L", "M", "N", "O",
//         "P", "Q", "R", "S", "T"
//     };
//
//     int n = 20;
//
//     printf("===== INSERTIONS =====\n");
//     for (int i = 0; i < n; i++) {
//         Student s;
//         memset(&s, 0, sizeof(Student));
//         s.id  = ids[i];
//         s.age = ages[i];
//         strcpy(s.name, names[i]);
//
//         printf("Inserting %d\n", ids[i]);
//         bplus_insert(&root, ids[i], &s);
//     }
//
//     printf("\n===== TREE AFTER INSERTIONS =====\n");
//     inorder(root);
//     printf("\n");
//     print_leaf_chain(root);
//
//     printf("\n===== DELETIONS =====\n");
//
//     // Deletion order (different from insertion)
//     int delete_ids[] = {18, 5, 42, 1, 50, 7, 27, 2, 39, 11,
//                         25, 45, 30, 22, 9, 14, 6, 35, 48, 33};
//
//     for (int i = 0; i < n; i++) {
//         printf("\nDeleting %d\n", delete_ids[i]);
//         bplus_delete(&root, delete_ids[i]);
//
//         printf("Tree after deleting %d:\n", delete_ids[i]);
//         inorder(root);
//         printf("\n");
//         print_leaf_chain(root);
//     }
//
//     printf("\n===== FINAL TREE =====\n");
//     if (root == NULL) {
//         printf("Tree is empty.\n");
//     } else {
//         inorder(root);
//         printf("\n");
//         print_leaf_chain(root);
//     }
//
//     return 0;
// }

int main() {
    BPlusNode *root = NULL;

    // =========================
    // TEST CASE 1: INSERTIONS
    // =========================
    int ids[] = {
        15, 3, 8, 22, 7, 10, 18, 5, 1, 12,
        30, 25, 40, 35, 28, 33, 45, 50, 2, 6
    };

    int n = sizeof(ids) / sizeof(ids[0]);

    printf("===== INSERTIONS =====\n");
    for (int i = 0; i < n; i++) {
        Student s;
        memset(&s, 0, sizeof(Student));

        s.id = ids[i];
        s.age = 18 + (ids[i] % 10);   // random-ish age
        sprintf(s.name, "S%d", ids[i]);

        printf("Inserting %d\n", ids[i]);
        bplus_insert(&root, ids[i], &s);
    }

    printf("\n===== TREE AFTER INSERTIONS =====\n");
    inorder(root);
    printf("\n");
    print_leaf_chain(root);

    // =========================
    // TEST CASE 1: DELETIONS
    // =========================
    int delete_ids[] = {
        8, 22, 1, 50, 3, 7, 10, 15,
        18, 5, 2, 6,
        12, 25, 30,
        28, 33, 35,
        40, 45
    };

    printf("\n===== DELETIONS =====\n");

    for (int i = 0; i < n; i++) {
        printf("\nDeleting %d\n", delete_ids[i]);

        bplus_delete(&root, delete_ids[i]);

        printf("Tree after deleting %d:\n", delete_ids[i]);
        inorder(root);
        printf("\n");
        print_leaf_chain(root);

        // 🚨 Safety check
        if (root == NULL && i != n - 1) {
            printf("ERROR: Tree became NULL too early\n");
            return 0;
        }
    }

    // =========================
    // FINAL CHECK
    // =========================
    printf("\n===== FINAL TREE =====\n");

    if (root == NULL) {
        printf("CORRECT: Tree is empty\n");
    } else {
        printf("INVALID: Tree is NOT empty\n");
        inorder(root);
        printf("\n");
        print_leaf_chain(root);
    }

    return 0;
}