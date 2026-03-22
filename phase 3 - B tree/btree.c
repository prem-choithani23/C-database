//
// Created by prem on 3/22/26.
//

#include <stdio.h>
#include "btree.h"


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

int main(int argc, char* argv[])
{

}
