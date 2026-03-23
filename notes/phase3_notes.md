# CDB Project — Phase 3 Notes
## B-Tree Implementation in C

---

## Table of Contents
1. [Why B-Trees Exist](#1-why-b-trees-exist)
2. [B-Tree Properties](#2-b-tree-properties)
3. [Node Structure](#3-node-structure)
4. [Search](#4-search)
5. [Insert](#5-insert)
6. [Split](#6-split)
7. [Delete](#7-delete)
8. [Borrow & Merge](#8-borrow--merge)
9. [Mistake Log](#9-mistake-log)
10. [Git History](#10-git-history)

---

## 1. Why B-Trees Exist

### The Problem with Binary Search Trees

```
BST worst case (sorted insertion):
1
 \
  2
   \
    3
     \
      4        ← O(n) search — basically a linked list
```

Even a balanced BST has problems for databases:
- Each node holds **1 key** → deep tree → many disk reads
- A disk read fetches a full block (~4KB) but you only use a few bytes

### The B-Tree Solution

```
B-Tree of order 4 (max 3 keys, 4 children per node):

              [10, 20, 30]
           /    |    |    \
        [5]  [15] [25]  [35,40]

Height = 2, covers 8 records
BST height for same data = up to 8
```

**Key insight:** More keys per node = shallower tree = fewer disk reads.

A B-Tree of order 500 with 1 million keys has height ~4.
That means **at most 4 disk reads** to find any record.

---

## 2. B-Tree Properties

For a B-Tree of **order m**:

| Property | Value |
|----------|-------|
| Max keys per node | m - 1 |
| Max children per node | m |
| Min keys per non-root node | ⌈m/2⌉ - 1 |
| All leaves | same depth |

For order 3 (our implementation):
- Max keys = **2**
- Max children = **3**
- Min keys (non-root) = **1**

```
Valid order-3 node examples:

[7]          ← 1 key, valid (minimum)
[7, 15]      ← 2 keys, valid (maximum)
[7, 15, 20]  ← 3 keys, INVALID — must split!
```

---

## 3. Node Structure

```c
#define ORDER       3
#define MAX_KEYS    (ORDER - 1)      // 2
#define MAX_CHILDREN (ORDER)          // 3

typedef struct node {
    int   keys[MAX_KEYS + 1];        // +1 for temporary overflow during split
    struct node *children[MAX_CHILDREN + 1]; // +1 for overflow
    int   key_count;                 // current number of keys
    bool  is_leaf;                   // true if no children
} Node;
```

### Why Fixed Arrays Instead of Pointers

```c
// BAD — two extra mallocs per node, cache unfriendly
typedef struct node {
    int   *keys;           // malloc separately
    struct node **children; // malloc separately
} Node;

// GOOD — everything in one allocation, cache friendly
typedef struct node {
    int   keys[MAX_KEYS + 1];
    struct node *children[MAX_CHILDREN + 1];
} Node;
```

### Why struct node * children, Not struct node children[]

```c
// ILLEGAL — recursive size, compiler can't calculate sizeof
typedef struct node {
    struct node children[3]; // infinite size!
} Node;

// LEGAL — pointer has fixed size (8 bytes on 64-bit)
typedef struct node {
    struct node *children[3]; // each is just an 8-byte pointer
} Node;
```

### create_node

```c
Node *create_node(bool is_leaf) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->is_leaf  = is_leaf;
    node->key_count = 0;
    // zero out keys and children pointers
    memset(node->keys,     0, (MAX_KEYS + 1)     * sizeof(int));
    memset(node->children, 0, (MAX_CHILDREN + 1) * sizeof(Node *));
    return node;
}
```

---

## 4. Search

### Algorithm

```
search(node, key):
  for each key[i] in node:
    if key == keys[i]  → FOUND, return node
    if key  < keys[i]  → recurse into children[i]
  if exhausted all keys → recurse into children[key_count]  (rightmost)
```

### Visual Example — Searching for 15

```
        [10, 20]
       /    |    \
   [5]    [15]   [25, 30]

Step 1: At [10, 20]
  → 15 > 10, move to next key
  → 15 < 20, go to children[1]

Step 2: At [15]
  → 15 == 15, FOUND ✓
```

### Visual Example — Searching for 25

```
Step 1: At [10, 20]
  → 25 > 10, move on
  → 25 > 20, keys exhausted
  → go to children[key_count] = children[2]

Step 2: At [25, 30]
  → 25 == 25, FOUND ✓
```

### Code

```c
Node *search(Node *root, int key) {
    if (root == NULL) return NULL;  // base case — not found

    int index = 0;
    for (index = 0; index < root->key_count; index++) {
        if (key == root->keys[index])
            return root;                          // found in this node
        else if (root->keys[index] > key)
            return search(root->children[index], key); // go left of this key
    }

    // key > all keys in node → go to rightmost child
    return search(root->children[root->key_count], key);
}
```

**Complexity:** O(log n) — tree height × keys per node comparison

---

## 5. Insert

### Two Cases

**Case A — Root is not full:**
```
insert_non_full(root, key)
```

**Case B — Root is full:**
```
1. Create new_root (is_leaf = false)
2. old_root becomes new_root->children[0]
3. split_child(new_root, 0, old_root)
4. insert_non_full(new_root, key)
5. *root = new_root   ← tree grows upward by 1 level
```

### insert_non_full

```c
Node *insert_non_full(Node *node, int key) {
    int pos = 0;

    if (node->is_leaf) {
        // find insertion position (maintain sorted order)
        while (pos < node->key_count && node->keys[pos] < key) pos++;

        // shift keys right to make room
        for (int i = node->key_count - 1; i >= pos; i--)
            node->keys[i + 1] = node->keys[i];

        node->keys[pos] = key;
        node->key_count++;
    } else {
        // find which child to descend into
        int found_idx = get_expected_position(node, key);
        Node *child = node->children[found_idx];

        if (is_full(child)) {
            // child is full — split it before descending
            split_child(node, found_idx, child);
            // after split, median moved up — adjust position if needed
            if (node->keys[found_idx] < key) found_idx++;
        }

        insert_non_full(node->children[found_idx], key);
    }
    return node;
}
```

### insert (entry point)

```c
Node *insert(Node **root, int key) {
    if (*root == NULL) {
        // empty tree — create first node
        Node *node = create_node(true);
        *root = node;
        insert_non_full(*root, key);
        return node;
    }

    if (is_full(*root)) {
        // root is full — tree must grow upward
        Node *new_root = create_node(false);
        new_root->children[0] = *root;      // old root becomes first child
        split_child(new_root, 0, *root);    // split old root
        insert_non_full(new_root, key);     // insert into new structure
        *root = new_root;                   // update root pointer
        return new_root;
    }

    return insert_non_full(*root, key);
}
```

---

## 6. Split

### Why Split?

When a node reaches `MAX_KEYS + 1` keys it violates the B-Tree property.
Split promotes the median key to the parent and creates two half-nodes.

### Visual — Splitting [25, 30] when inserting 35

```
Before:
Parent: [10, 20]          children: [[5], [15], [25,30]]
Insert 35 → [25, 30] is full → split

Step 1: median_idx = 2/2 = 1 → median = 30
Step 2: left  = [25]          (keys before median)
Step 3: right = [35]          (keys after median)
Step 4: promote 30 to parent

After:
Parent: [10, 20, 30]      children: [[5], [15], [25], [35]]
```

### Cascading Root Split

```
Insert into full tree:

Before:             After split + insert 15:
[10, 20]                    [20]
/      \                  /      \
...    [25,30]          [10]     [30]
                        /  \    /   \
                      [5] [15][25] [35]

Tree grew upward by one level — always balanced!
```

### split_child Code

```c
void split_child(Node *parent, int found_idx, Node *full_child) {
    int median_idx     = full_child->key_count / 2;
    int median_element = full_child->keys[median_idx];

    // create left child — gets keys BEFORE median
    int left_size   = median_idx;
    Node *left_child = create_node(full_child->is_leaf);
    for (int i = 0; i < left_size; i++) {
        left_child->keys[i]     = full_child->keys[i];
        left_child->children[i] = full_child->children[i];
    }
    left_child->children[left_size] = full_child->children[left_size];
    left_child->key_count = left_size;

    // create right child — gets keys AFTER median
    int right_size   = full_child->key_count - median_idx - 1;
    Node *right_child = create_node(full_child->is_leaf);
    for (int i = 0; i < right_size; i++) {
        right_child->keys[i]     = full_child->keys[i + median_idx + 1];
        right_child->children[i] = full_child->children[i + median_idx + 1];
    }
    right_child->children[right_size] =
        full_child->children[right_size + median_idx + 1];
    right_child->key_count = right_size;

    // shift parent's keys and children right to make room for median
    for (int i = parent->key_count - 1; i >= found_idx; i--) {
        parent->keys[i + 1]     = parent->keys[i];
        parent->children[i + 2] = parent->children[i + 1];
    }

    // insert median into parent, point to new children
    parent->keys[found_idx]         = median_element;
    parent->children[found_idx]     = left_child;
    parent->children[found_idx + 1] = right_child;
    parent->key_count++;

    free(full_child); // original node replaced by left + right
}
```

**Critical:** Use `free(full_child)` NOT `free_node(full_child)` — the children have already been moved to left/right. Recursive free would double-free them.

---

## 7. Delete

### Three Cases

```
Case 1: Key in LEAF with > MIN_KEYS
→ just remove and shift left

Case 2: Key in INTERNAL NODE, predecessor/successor has extra keys
→ replace key with predecessor (or successor), delete predecessor from leaf

Case 3: Key in INTERNAL NODE, both predecessor and successor at MIN_KEYS
→ merge children, recurse
```

### Case 1 — Leaf Delete

```
Tree: [10] → [5, 7] → delete 7

[5, 7] has 2 keys > MIN_KEYS=1
→ shift left: [5]
→ key_count--
```

### Case 2 — Internal, Borrow from Predecessor

```
Tree:
        [6]
       /   \
    [5]    [7]
Delete 6:
→ inorder predecessor = 5 (rightmost key in left subtree)
→ replace 6 with 5: [5]
→ delete 5 from leaf [5] → leaf becomes []
→ empty leaf triggers merge...
```

### Case 3 — Merge

```
Before merge:
Parent: [6]
Left:   [5]     ← MIN_KEYS
Right:  [7]     ← MIN_KEYS

After merge_children(parent, 0):
Merged: [5, 6, 7]   ← separator key pulled DOWN from parent
Parent: []           ← parent loses a key

If parent was root and is now empty:
→ merged node becomes new root
→ tree shrinks by one level
```

### get_inorder_predecessor / get_inorder_successor

```c
// Rightmost leaf in left subtree
Node *get_inorder_predecessor(Node *node, int found_idx) {
    Node *curr = node->children[found_idx];
    while (!curr->is_leaf)
        curr = curr->children[curr->key_count]; // always go right
    return curr;
}

// Leftmost leaf in right subtree
Node *get_inorder_successor(Node *node, int found_idx) {
    Node *curr = node->children[found_idx + 1];
    while (!curr->is_leaf)
        curr = curr->children[0]; // always go left
    return curr;
}
```

---

## 8. Borrow & Merge

### The Top-Down Proactive Approach

**Problem with bottom-up delete:** After merging a child, the parent may become empty, requiring another fix upward — complex to handle.

**Solution:** Fix nodes on the way **DOWN** before recursing. By the time you reach the target, every node on the path has > MIN_KEYS.

```
Rule: before descending into a child,
if child->key_count == MIN_KEYS → fix it first:
  1. try borrow_from_left
  2. try borrow_from_right
  3. merge if neither works
```

### Borrow from Left Sibling

```
Before:
Parent: [..., 10, ...]
Left:   [7, 9]       ← has extra keys
Child:  [12]         ← MIN_KEYS, about to get smaller

After borrow_from_left:
Parent: [..., 9, ...]   ← 10 replaced by 9
Left:   [7]             ← gave up 9
Child:  [10, 12]        ← received 10 from parent
```

```c
void borrow_from_left(Node *node, int pos) {
    Node *child = node->children[pos];
    Node *left  = node->children[pos - 1];

    // shift child's keys right to make room at front
    for (int i = child->key_count; i > 0; i--)
        child->keys[i] = child->keys[i - 1];
    if (!child->is_leaf)
        for (int i = child->key_count + 1; i > 0; i--)
            child->children[i] = child->children[i - 1];

    // pull parent separator key down into child's front
    child->keys[0] = node->keys[pos - 1];
    child->key_count++;

    // push left sibling's last key up to parent
    node->keys[pos - 1] = left->keys[left->key_count - 1];
    if (!left->is_leaf)
        child->children[0] = left->children[left->key_count];
    left->key_count--;
}
```

### Borrow from Right Sibling

```
Before:
Parent: [..., 10, ...]
Child:  [7]          ← MIN_KEYS
Right:  [12, 15]     ← has extra keys

After borrow_from_right:
Parent: [..., 12, ...]  ← 10 replaced by 12
Child:  [7, 10]         ← received 10 from parent
Right:  [15]            ← gave up 12
```

```c
void borrow_from_right(Node *node, int pos) {
    Node *child = node->children[pos];
    Node *right = node->children[pos + 1];

    // pull parent key down to end of child
    child->keys[child->key_count] = node->keys[pos];
    child->key_count++;

    // push right sibling's first key up to parent
    node->keys[pos] = right->keys[0];

    // shift right sibling's keys left
    for (int i = 0; i < right->key_count; i++)
        right->keys[i] = right->keys[i + 1];

    if (!right->is_leaf) {
        // move right sibling's first child to end of child's children
        Node *dangling = right->children[0];
        for (int i = 0; i < right->key_count; i++)
            right->children[i] = right->children[i + 1];
        child->children[child->key_count] = dangling;
    }
    right->key_count--;
}
```

### merge_children

```
Before:
Parent: [6]
Left:   [5]
Right:  [7]

After merge_children(parent, 0):
Merged: [5, 6, 7]   ← left keys + separator + right keys
Parent: []           ← separator removed, key_count--
```

```c
void merge_children(Node *parent, int found_idx) {
    Node *left_child  = parent->children[found_idx];
    Node *right_child = parent->children[found_idx + 1];
    Node *new_child   = create_node(left_child->is_leaf);

    int left_size  = left_child->key_count;
    int right_size = right_child->key_count;

    // copy left child's keys and children
    for (int i = 0; i < left_size; i++) {
        new_child->keys[i] = left_child->keys[i];
        if (!left_child->is_leaf)
            new_child->children[i] = left_child->children[i];
    }
    if (!left_child->is_leaf)
        new_child->children[left_size] = left_child->children[left_size];

    // pull separator key from parent
    new_child->keys[left_size] = parent->keys[found_idx];

    // copy right child's keys and children
    for (int i = left_size + 1; i < 1 + left_size + right_size; i++) {
        new_child->keys[i] = right_child->keys[i - left_size - 1];
        if (!right_child->is_leaf)
            new_child->children[i] = right_child->children[i - left_size - 1];
    }
    if (!right_child->is_leaf)
        new_child->children[left_size + 1 + right_size] =
            right_child->children[right_size];

    new_child->key_count = left_size + right_size + 1;

    // remove separator from parent, shift left
    for (int i = found_idx; i < parent->key_count - 1; i++) {
        parent->keys[i]         = parent->keys[i + 1];
        parent->children[i + 1] = parent->children[i + 2];
    }
    parent->children[found_idx] = new_child;
    parent->key_count--;

    free(left_child);
    free(right_child);
}
```

### Root Shrink

```c
bool delete(Node **root, int key) {
    if (*root == NULL) return false;

    delete_recursive(*root, key);

    // if root lost all keys (after merge), child becomes new root
    if ((*root)->key_count == 0) {
        Node *old_root = *root;
        *root = (*root)->children[0]; // only child is the merged node
        free(old_root);               // tree shrinks by one level
    }
    return true;
}
```

---

## 9. Mistake Log

### Mistake #1 — Returning Stack Pointer in Recursive Calls
```c
// WRONG — result of recursion discarded
search(root->children[index], key); // return value ignored!

// FIXED
return search(root->children[index], key);
```

### Mistake #2 — Wrong Rightmost Child Index
```c
// WRONG — MAX_CHILDREN-1 is wrong for non-full nodes
return search(root->children[MAX_CHILDREN - 1], key);

// FIXED — use key_count as index to rightmost child
return search(root->children[root->key_count], key);
```

### Mistake #3 — Double-Free in split_child
```c
// WRONG — recursively frees children that were moved to left/right
free_node(full_child);

// FIXED — only free the node itself, children already moved
free(full_child);
```

### Mistake #4 — Recursive Arrays in Struct
```c
// WRONG — infinite size, won't compile
struct node { struct node children[3]; };

// FIXED — pointer array
struct node { struct node *children[3]; };
```

### Mistake #5 — count++ on Eviction (from cache work, applies to splits too)
```c
// WRONG — replacing doesn't increase count
cache->pages[min_idx] = entry;
cache->count++;   // wrong!

// FIXED
cache->pages[min_idx] = entry;
// no count++ — we replaced, not added
```

### Mistake #6 — Freeing Stack-Allocated Struct
```c
// WRONG — node was on stack, not heap
void free_cache(PageCache *cache) {
    free(cache->pages);
    free(cache);   // cache is on stack in main!
}

// FIXED
void free_cache(PageCache *cache) {
    free(cache->pages);
    cache->pages = NULL;
}
```

### Mistake #7 — Leaf Case Falls Through Without Deleting
```c
// WRONG — reaches leaf but exits without deleting
if (child->is_leaf) {
    return; // key exists in leaf but never deleted!
}

// FIXED — remove this check, let recursion reach the leaf
// leaf deletion is handled in the key-found branch
```

### Mistake #8 — Using freed child after merge
```c
// WRONG — child is freed inside merge_children
merge_children(node, pos);
delete_recursive(child, key); // child is freed memory!

// FIXED — use node->children[pos] which points to merged node
merge_children(node, pos);
delete_recursive(node->children[pos], key);
```

### Mistake #9 — Wrong sibling for merge when rightmost child
```c
// WRONG — no right sibling at pos == key_count
merge_children(node, pos);     // tries parent->children[pos+1] = NULL → crash!

// FIXED — check which sibling exists
if (right_sibling)
    merge_children(node, pos);
else
    merge_children(node, pos - 1); // merge with left sibling instead
```

---

## 10. Git History

```
feat:     add BTree Node struct with ORDER, MAX_KEYS, MAX_CHILDREN
feat:     add BTree search with recursive traversal
feat:     B-Tree insert with root split, search, inorder traversal
fix:      avoid double-free in split_child by freeing node only, not children
feat:     B-Tree delete with borrow from sibling and merge
feat:     B-Tree CRUD complete — insert, search, delete with borrow and merge
```

---

## Key Takeaways

1. **B-Trees exist for disk I/O** — fewer levels = fewer disk reads = faster queries.
2. **Trees grow upward** — splits cascade up, root split increases height by 1. Always balanced.
3. **Median moves up on split** — left half stays, right half is new node, median promoted.
4. **Top-down delete** — fix nodes on the way down so you never need to backtrack.
5. **Borrow before merge** — always try to steal from a sibling before merging.
6. **Merge pulls separator down** — parent key comes down into merged node.
7. **Root shrink** — when root becomes empty after merge, its only child is the new root.
8. **`free` vs `free_node`** — in split, children are moved, so only `free` the shell node.
9. **`key_count` is the rightmost child index** — `children[key_count]` is always the rightmost child.
10. **Inorder traversal = sorted output** — if inorder is sorted after every operation, your tree is correct.

---

## B-Tree vs B+ Tree (Preview)

```
B-Tree:                          B+ Tree:
Data in ALL nodes                Data ONLY in leaves
                                 Leaves linked in a chain

        [10]                            [10]
       /    \                          /    \
    [5,data] [15,data]            [10]      [15]
                                 /    \    /    \
                              [5]→[10]→[15]→[20]
                                     ↑
                              linked list of leaves
                              → O(k) range queries
```

---

*Phase 4 → B+ Tree: leaf linking, copied keys, range queries.*
