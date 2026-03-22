//
// Created by prem on 3/22/26.
//

#ifndef CDB_BTREE_H
#define CDB_BTREE_H
#define ORDER 3
#define MAX_CHILDREN (ORDER)
#define MAX_KEYS (ORDER - 1)

#include <stdbool.h>
typedef struct node
{
    int keys[MAX_KEYS];
    struct node * children[MAX_CHILDREN];
    int key_count;
    bool is_leaf;
}Node;
#endif //CDB_BTREE_H