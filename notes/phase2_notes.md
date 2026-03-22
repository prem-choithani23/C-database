# CDB Project — Phase 2 Notes
## Memory Mastery & Page Cache

---

## Table of Contents
1. [Heap vs Stack — The Full Picture](#1-heap-vs-stack--the-full-picture)
2. [malloc / calloc / realloc / free](#2-malloc--calloc--realloc--free)
3. [Pointer Arithmetic](#3-pointer-arithmetic)
4. [Struct by Value vs Pointer](#4-struct-by-value-vs-pointer)
5. [void* and Typecasting](#5-void-and-typecasting)
6. [Building the PageCache](#6-building-the-pagecache)
7. [LRU Eviction Policy](#7-lru-eviction-policy)
8. [Separation of Concerns](#8-separation-of-concerns)
9. [Mistake Log](#9-mistake-log)
10. [Git History](#10-git-history)

---

## 1. Heap vs Stack — The Full Picture

### Memory Layout of a C Program

```
High Address
┌─────────────────┐
│   Stack         │  ← local variables, function call frames
│   ↓ grows down  │
├─────────────────┤
│   (free space)  │
├─────────────────┤
│   ↑ grows up    │
│   Heap          │  ← malloc / calloc / realloc
├─────────────────┤
│   BSS           │  ← uninitialised globals
├─────────────────┤
│   Data          │  ← initialised globals, string literals (.rodata)
├─────────────────┤
│   Text          │  ← your compiled code
└─────────────────┘
Low Address
```

### Key Differences

| Property       | Stack                          | Heap                            |
|----------------|--------------------------------|---------------------------------|
| Allocated by   | Compiler (automatic)           | You (`malloc`)                  |
| Freed by       | Compiler (on function return)  | You (`free`)                    |
| Size           | Fixed, small (~1–8MB)          | Large (limited by RAM)          |
| Speed          | Very fast                      | Slower (system call)            |
| Lifetime       | Until function returns         | Until you call `free`           |
| Failure mode   | Stack overflow                 | `malloc` returns `NULL`         |

### Why This Matters for a Database

A page cache holding thousands of records cannot live on the stack — it would overflow immediately. It must live on the heap, dynamically allocated with `malloc`.

---

## 2. malloc / calloc / realloc / free

### malloc

```c
void *malloc(size_t size);
```

Allocates `size` bytes on the heap. Returns `void *` — a pointer to uninitialized memory.  
Returns `NULL` on failure.

```c
Student *cache = malloc(100 * sizeof(Student));
if (cache == NULL) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
}
```

**Always check for NULL.** malloc can fail if the system is out of memory.

### calloc

```c
void *calloc(size_t nmemb, size_t size);
```

Like `malloc`, but **zero-initialises** all bytes. Safer for arrays.

```c
Student *cache = calloc(100, sizeof(Student));
// all bytes are 0 — no garbage
```

### realloc

```c
void *realloc(void *ptr, size_t new_size);
```

Resizes a previously allocated block. May move the data to a new location.  
Returns a new pointer — **always reassign, never use the old pointer after realloc.**

```c
// WRONG
realloc(cache, new_size);        // old pointer may be invalid now!

// CORRECT
cache = realloc(cache, new_size);
if (cache == NULL) { /* handle failure */ }
```

### free

```c
void free(void *ptr);
```

Returns heap memory to the OS. After `free`:
- Set the pointer to `NULL` immediately
- Never dereference a freed pointer — **use-after-free** is undefined behaviour

```c
free(cache->pages);
cache->pages = NULL; // prevents accidental use-after-free
```

### Memory Leak

A memory leak occurs when you `malloc` memory and lose all references to it without calling `free`.

```c
// LEAK — malloc'd PageCache struct is leaked
PageCache *create_cache(int capacity) {
    PageCache *cache = malloc(sizeof(PageCache)); // heap allocation
    cache->pages = malloc(sizeof(PageEntry) * capacity);
    return *cache; // returns a COPY — original pointer lost forever!
}
```

The heap-allocated `PageCache` is never freed. Over time, leaks exhaust available memory.

**Fix:** Don't `malloc` the struct if you're returning by value.

```c
PageCache create_cache(int capacity) {
    PageCache cache;                // stack — fine, returning a copy
    cache.pages = malloc(...);      // only pages needs heap
    cache.capacity = capacity;
    cache.count = 0;
    cache.clock = 0;
    return cache;                   // copy is valid — pages pointer survives
}
```

---

## 3. Pointer Arithmetic

### How It Works

When you add an integer to a pointer, C automatically scales by `sizeof` the pointed-to type.

```c
Student *cache = malloc(100 * sizeof(Student));

// These are identical:
Student fifth = cache[4];
Student fifth = *(cache + 4); // cache + 4 = cache + (4 * sizeof(Student)) bytes
```

`cache + 1` does NOT add 1 byte — it adds `sizeof(Student)` = 60 bytes.

### Visualising the Heap Block

```
cache ──→ [Student 0][Student 1][Student 2]...[Student 99]
           ^           ^           ^
           cache+0     cache+1     cache+2
           offset 0    offset 60   offset 120
```

### Pointer to Array Element

To modify an element in-place without copying, take its address:

```c
// WRONG — modifies a local copy, cache unchanged
PageEntry entry = cache->pages[i];
entry.last_used = clock;          // only updates the copy!

// CORRECT — pointer directly into the cache array
PageEntry *entry = &cache->pages[i];
entry->last_used = clock;         // updates the actual cache entry
```

This was a critical bug in the LRU implementation — the fix was understanding that `=` on a struct makes a copy.

---

## 4. Struct by Value vs Pointer

### Returning a Struct by Value

```c
PageCache create_cache(int capacity) {
    PageCache cache;
    cache.pages = malloc(sizeof(PageEntry) * capacity);
    cache.capacity = capacity;
    cache.count = 0;
    cache.clock = 0;
    return cache; // returns a copy of the struct
}
```

The copy contains `capacity`, `count`, `clock`, and the **value of the `pages` pointer** — which still points to the heap allocation. The heap memory doesn't get copied or freed. This is safe and efficient for small structs.

### When to Return a Pointer Instead

Return `T *` when:
- The struct is very large (copying is expensive)
- The struct must be shared/modified by multiple callers
- You need polymorphism or nullable semantics

### Struct Assignment Copies All Fields

```c
SoftStudent a = get_student();
SoftStudent b = a; // full copy — all fields duplicated
b.age = 99;        // does NOT affect a
```

Since `SoftStudent` contains no pointers (only inline arrays), struct assignment is a safe deep copy. This is why `*student = entry->student` works correctly in `get`.

---

## 5. void* and Typecasting

### Why malloc Returns void*

`malloc` is a general-purpose allocator — it doesn't know what type you'll store in the memory. It returns `void *` ("pointer to anything"), which must be cast to the correct type before use.

```c
// malloc returns void* — we cast to Student*
Student *cache = (Student *)malloc(100 * sizeof(Student));
```

In C (unlike C++), the cast is technically optional since `void *` implicitly converts to any pointer type. But being explicit is good practice — it documents intent and catches type mismatches.

### sizeof With Types vs Variables

```c
malloc(100 * sizeof(Student));   // size of the type
malloc(100 * sizeof(*cache));    // size of what cache points to — safer
```

The second form is safer — if you change the type of `cache`, the `sizeof` automatically adjusts.

---

## 6. Building the PageCache

### Data Structures

```c
// modified_student.h
typedef struct {
    bool is_deleted;
    int  id;
    char name[50];
    int  age;
} SoftStudent; // sizeof = 64

// page_cache.h
typedef struct {
    SoftStudent student;   // the cached record
    int         last_used; // LRU timestamp
} PageEntry;

typedef struct {
    PageEntry *pages;   // heap-allocated array of entries
    int        capacity; // max entries
    int        count;    // current entries
    int        clock;    // monotonically increasing timestamp
} PageCache;
```

### Lifecycle

```c
// Create
PageCache cache = create_cache(10);

// Use
put(&cache, &student);
get(&cache, id, &out, file);

// Destroy
free_cache(&cache);
```

### create_cache

```c
PageCache create_cache(int capacity) {
    PageCache cache;
    cache.capacity = capacity;
    cache.count    = 0;
    cache.clock    = 0;
    cache.pages    = (PageEntry *)malloc(sizeof(PageEntry) * capacity);

    if (cache.pages == NULL) {
        fprintf(stderr, "malloc failed in create_cache\n");
        // caller checks cache.pages for NULL
    }
    return cache;
}
```

### free_cache

```c
void free_cache(PageCache *cache) {
    free(cache->pages);   // only pages was malloc'd
    cache->pages    = NULL;
    cache->count    = 0;
    cache->capacity = 0;
}
```

**Important:** We do NOT `free(cache)` itself — `cache` was declared on the stack in `main`, not heap-allocated.

### put

```c
int put(PageCache *cache, SoftStudent *student) {
    PageEntry entry;
    entry.student   = *student;
    entry.last_used = cache->clock;

    if (is_full(cache)) {
        int min_idx = find_min(cache);     // evict LRU
        cache->pages[min_idx] = entry;
        cache->clock++;
        return 0; // eviction occurred
    }

    cache->pages[cache->count] = entry;
    cache->count++;
    cache->clock++;
    return 1;
}
```

**Key detail:** When the cache is full and you evict, `count` does NOT increase — you're replacing, not adding.

### get

```c
int get(PageCache *cache, int id, SoftStudent *student, FILE *file) {
    // Search cache first
    for (int i = 0; i < cache->count; i++) {
        PageEntry *entry = &cache->pages[i]; // pointer, not copy!
        if (entry->student.id == id) {
            entry->last_used = cache->clock; // update timestamp
            cache->clock++;
            *student = entry->student;
            return 1; // cache hit
        }
    }

    // Cache miss — load from disk
    SoftStudent s;
    read_entry(file, id, &s);
    put(cache, &s);
    *student = s;
    return 1;
}
```

### load_records

```c
void load_records(PageCache *cache, int start_id, int end_id, FILE *file) {
    fseek(file, start_id * sizeof(SoftStudent), SEEK_SET);

    SoftStudent student;
    for (int i = start_id; i <= end_id; i++) {
        fread(&student, sizeof(SoftStudent), 1, file);
        put(cache, &student);
    }
}
```

---

## 7. LRU Eviction Policy

### The Problem

A cache has limited capacity. When it's full and a new record is needed, something must be removed. The question is: **which one?**

**LRU (Least Recently Used):** Evict the record that was accessed least recently — the assumption being that records not accessed recently are less likely to be needed again.

### Clock Timestamp Approach

Every `PageEntry` has a `last_used` field. The `PageCache` has a monotonically increasing `clock` counter.

- On every `put` or `get` hit → stamp the entry with current `clock`, increment `clock`
- On eviction → find the entry with the lowest `last_used` (the LRU entry)

```
Initial state after loading records 0–9:
Slot 0: id=0, last_used=0
Slot 1: id=1, last_used=1
...
Slot 9: id=9, last_used=9
clock = 10

After get(id=5):
Slot 5: id=5, last_used=10  ← updated
clock = 11

After get(id=99) — cache miss, eviction needed:
find_min() → Slot 0 has last_used=0, lowest → evict id=0
Slot 0: id=99, last_used=11 ← new entry
clock = 12
```

### find_min — O(n) Linear Scan

```c
int find_min(PageCache *cache) {
    int min_idx   = 0;
    int min_clock = cache->pages[0].last_used;

    for (int i = 0; i < cache->count; i++) {
        if (cache->pages[i].last_used < min_clock) {
            min_clock = cache->pages[i].last_used;
            min_idx   = i;
        }
    }
    return min_idx;
}
```

**Complexity:** O(n) per eviction. For small caches (10–100 entries) this is fine.  
**Better alternatives:** Min-heap gives O(log n). But premature optimisation is a trap — measure first, optimise second.

### Idempotency in is_full / is_empty

```c
bool is_full(PageCache *cache) {
    return cache->count == cache->capacity;
}

bool is_empty(PageCache *cache) {
    return cache->count == 0;
}
```

Small helper functions that make the code self-documenting and easy to test independently.

---

## 8. Separation of Concerns

### The Hardcoded Filename Problem

**Wrong:**
```c
int get(PageCache *cache, int id, SoftStudent *student) {
    // ...cache miss...
    FILE *file = fopen("soft_students.db", "rb"); // hardcoded!
    read_entry(file, id, &s);
    fclose(file);
}
```

Problems:
- `get` now knows about the filesystem — it shouldn't
- Can't reuse with a different file
- Opens and closes the file on every miss — expensive
- Impossible to test in isolation

**Correct:**
```c
int get(PageCache *cache, int id, SoftStudent *student, FILE *file) {
    // ...cache miss...
    read_entry(file, id, &s); // file is the caller's concern
}
```

The caller (`main`) owns the file handle and passes it in. `get` just does its job.

### General Principle

> A function should do one thing and receive everything it needs as parameters.  
> It should not reach out and grab resources itself.

This is the foundation of **dependency injection** — a concept you'll see everywhere in larger systems.

---

## 9. Mistake Log

### Mistake #1 — malloc'ing the Struct Then Returning by Value (Memory Leak)
**Severity:** Memory leak

```c
// WRONG — leaks the heap-allocated PageCache
PageCache create_cache(int capacity) {
    PageCache *cache = malloc(sizeof(PageCache)); // heap
    cache->pages = malloc(...);
    return *cache; // copy returned, original pointer lost — LEAKED
}

// FIXED — struct on stack, only pages on heap
PageCache create_cache(int capacity) {
    PageCache cache;          // stack
    cache.pages = malloc(...); // heap — pages pointer copied safely
    return cache;
}
```

**Lesson:** If you're returning by value, declare the struct on the stack. Only heap-allocate what needs to outlive the function.

---

### Mistake #2 — Freeing a Stack Pointer
**Severity:** Undefined behaviour / crash

```c
// WRONG
void free_cache(PageCache *cache) {
    free(cache->pages); // correct
    free(cache);        // WRONG — cache lives on the stack in main!
}

// FIXED
void free_cache(PageCache *cache) {
    free(cache->pages);
    cache->pages = NULL;
}
```

**Lesson:** Only `free` what was `malloc`'d. Know where each pointer came from.

---

### Mistake #3 — Updating a Local Copy Instead of the Cache Entry
**Severity:** Silent bug — LRU timestamps never update

```c
// WRONG — entry is a COPY of cache->pages[i]
PageEntry entry = cache->pages[i];
entry.last_used = cache->clock; // updates the copy, not the cache!

// FIXED — entry is a POINTER to cache->pages[i]
PageEntry *entry = &cache->pages[i];
entry->last_used = cache->clock; // updates the actual cache
```

**Lesson:** `T x = array[i]` makes a copy. `T *x = &array[i]` gives you a reference. Know which one you need.

---

### Mistake #4 — Incrementing count on Eviction
**Severity:** count exceeds capacity, buffer overflow

```c
// WRONG — count++ when replacing, not adding
if (is_full(cache)) {
    cache->pages[min_idx] = entry;
    cache->count++; // WRONG — still 10 entries, not 11!
}

// FIXED — count stays the same on eviction
if (is_full(cache)) {
    cache->pages[min_idx] = entry;
    // no count++ — we replaced, not added
}
```

**Lesson:** Track what your counter actually represents. On eviction, you replace an existing slot — the count doesn't change.

---

### Mistake #5 — &s on an Already-Pointer Parameter
**Severity:** Writes to wrong memory location

```c
void read_entry(FILE *file, int id, SoftStudent *s) {
    fseek(file, id * sizeof(SoftStudent), SEEK_SET);
    fread(&s, sizeof(SoftStudent), 1, file); // WRONG — &s is SoftStudent**!
}

// FIXED
void read_entry(FILE *file, int id, SoftStudent *s) {
    fseek(file, id * sizeof(SoftStudent), SEEK_SET);
    fread(s, sizeof(SoftStudent), 1, file); // s is already a pointer
}
```

**Lesson:** If the parameter is already a pointer, pass it directly to `fread`. Adding `&` gives you a pointer-to-pointer — the wrong level of indirection.

---

### Mistake #6 — Cache Miss Returning 0 Even After Successful Load
**Severity:** Logic error — caller thinks record not found

```c
// WRONG
int get(...) {
    // ...cache miss...
    put(cache, &s);
    return 0; // says "not found" even though we loaded it!
}

// FIXED
int get(...) {
    // ...cache miss...
    put(cache, &s);
    *student = s;
    return 1; // record was found and loaded
}
```

**Lesson:** Return values must accurately reflect what happened. A cache miss that successfully loads from disk is still a successful `get`.

---

### Mistake #7 — Hardcoded Filename in get
**Severity:** Design smell — breaks reusability and testability

```c
// WRONG
int get(PageCache *cache, int id, SoftStudent *student) {
    FILE *file = fopen("soft_students.db", "rb"); // hardcoded!
    // ...
}

// FIXED
int get(PageCache *cache, int id, SoftStudent *student, FILE *file) {
    // file is caller's responsibility
}
```

**Lesson:** Functions should receive their dependencies as parameters, not reach out and grab them. This is the separation of concerns principle.

---

## 10. Git History

```
feat:     implement PageCache with put, get, load_records
feat:     LRU eviction policy with clock timestamp in PageCache
fix:      cache miss returns 1 after successful disk load
refactor: remove hardcoded filename from get, accept FILE* param
```

---

## Key Takeaways

1. **Stack for small/temporary, heap for large/persistent** — a cache belongs on the heap.
2. **malloc returns void\*** — C's type system defers to you. Cast explicitly.
3. **Always NULL-check malloc** — failure is rare but catastrophic if ignored.
4. **`T x = array[i]` is a copy, `T *x = &array[i]` is a reference** — know which you need.
5. **Only free what you malloc'd** — freeing stack memory is undefined behaviour.
6. **Set pointers to NULL after free** — prevents silent use-after-free bugs.
7. **count++ only on insertion, not replacement** — think carefully about what your counter means.
8. **LRU with a clock is simple and effective** — O(n) eviction is fine for small caches.
9. **Separation of concerns** — functions receive dependencies, they don't reach out and grab them.
10. **Premature optimisation is a trap** — linear scan before min-heap. Measure first.

---

## What the Cache Looks Like in Memory

```
PageCache (stack in main):
┌─────────────────────────────┐
│ pages    → (heap ptr)       │──→ [PageEntry 0][PageEntry 1]...[PageEntry 9]
│ capacity = 10               │         ↕ each entry = SoftStudent + int
│ count    = 10               │    ┌──────────────────┐
│ clock    = 12               │    │ student.id       │
└─────────────────────────────┘    │ student.name[50] │
                                   │ student.age      │
                                   │ student.is_del   │
                                   │ last_used        │
                                   └──────────────────┘
```

---

*Phase 3 → B-Tree: node structs, recursive insert, split on overflow, search in O(log n).*
