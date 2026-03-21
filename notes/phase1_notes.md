# CDB Project — Phase 1 Notes
## Binary Record Store in C

---

## Table of Contents
1. [Structs & Memory Layout](#1-structs--memory-layout)
2. [Binary File I/O](#2-binary-file-io)
3. [Random Access with fseek](#3-random-access-with-fseek)
4. [In-Place Updates](#4-in-place-updates)
5. [Stack vs Heap Memory](#5-stack-vs-heap-memory)
6. [Strings in C](#6-strings-in-c)
7. [Hex Dump & Raw Bytes](#7-hex-dump--raw-bytes)
8. [Packed Structs & Layout Corruption](#8-packed-structs--layout-corruption)
9. [Linear Scan vs Random Access](#9-linear-scan-vs-random-access)
10. [Soft Delete — Tombstone Pattern](#10-soft-delete--tombstone-pattern)
11. [Mistake Log](#11-mistake-log)
12. [Git History](#12-git-history)

---

## 1. Structs & Memory Layout

### What is Struct Padding?

The C compiler does **not** pack struct fields tightly. It inserts invisible padding bytes between fields to ensure each field starts at a memory address that is a multiple of its own size. This is called **alignment**.

Why? Because modern CPUs read memory in aligned chunks. An unaligned read requires two memory fetches instead of one — slower, and on some architectures, a hard fault.

**Rule:** A field of size `N` must start at an offset that is a multiple of `N`.

### Calculating sizeof Manually

```c
struct Student {
    int  id;       // 4 bytes, offset 0
    char name[50]; // 50 bytes, offset 4
    int  age;      // 4 bytes — needs offset multiple of 4
};
```

After `id(4) + name(50)` = offset 54.  
Is 54 a multiple of 4? **No.**  
Next multiple of 4 after 54 = **56**.  
So 2 bytes of padding are inserted.

```
Offset 0  : [id       ] 4 bytes
Offset 4  : [name     ] 50 bytes
Offset 54 : [padding  ] 2 bytes  ← compiler inserted silently
Offset 56 : [age      ] 4 bytes
Total     : 60 bytes
```

`sizeof(struct Student) = 60`, not 58.

### Tail Padding

The total struct size must be a multiple of its **largest member's alignment**.  
This ensures arrays of structs work correctly — every element must be properly aligned.

```c
struct Student {
    int  id;    // 4 bytes
    int  age;   // 4 bytes
    char name[50]; // 50 bytes
};
// 4 + 4 + 50 = 58 → not a multiple of 4 → padded to 60
```

Both orderings give `sizeof = 60` here. But field ordering **can** save memory:

```c
// 24 bytes — wasteful
struct Bad {
    char   a;  // 1 byte + 7 padding
    double b;  // 8 bytes
    char   c;  // 1 byte + 7 padding
};

// 16 bytes — optimal
struct Good {
    double b;  // 8 bytes
    char   a;  // 1 byte
    char   c;  // 1 byte + 6 tail padding
};
```

**General rule:** Sort fields in descending order of their alignment size.

### Verifying with Code

```c
#include <stdio.h>

typedef struct {
    int  id;
    char name[50];
    int  age;
} Student;

int main() {
    printf("sizeof(Student) = %zu\n", sizeof(Student)); // 60
    return 0;
}
```

---

## 2. Binary File I/O

### File Modes

| Mode   | Read | Write | Create | Truncate | Binary |
|--------|------|-------|--------|----------|--------|
| `"w"`  | ✗    | ✓     | ✓      | ✓        | ✗      |
| `"wb"` | ✗    | ✓     | ✓      | ✓        | ✓      |
| `"r"`  | ✓    | ✗     | ✗      | ✗        | ✗      |
| `"rb"` | ✓    | ✗     | ✗      | ✗        | ✓      |
| `"rb+"`| ✓    | ✓     | ✗      | ✗        | ✓      |

**Always use binary modes (`b`) for struct data.** Text mode on Windows silently translates `\n` to `\r\n`, corrupting binary data.

`"rb+"` is what a database engine uses — read and write without truncating existing data.

### fwrite

```c
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
```

- `ptr`   — pointer to the data to write
- `size`  — size of each item in bytes
- `nmemb` — number of items to write
- `stream`— file pointer

Returns number of items successfully written.

### fread

```c
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
```

Same parameter order. Returns number of items successfully read.

### Writing 100 Records

```c
void insert_rows(FILE *file) {
    int num_items = 100;

    for (int i = 0; i < num_items; i++) {
        Student student;
        memset(&student, 0, sizeof(Student)); // zero out padding bytes

        student.id = i;
        student.age = 15 + rand() % 30;

        char id_str[4];
        sprintf(id_str, "%d", i);

        char name[11] = "Student_";
        strcat(name, id_str);
        strcpy(student.name, name);

        int written = fwrite(&student, sizeof(Student), 1, file);
        if (written == 0) {
            printf("Write failed at record %d\n", i);
        }
    }
}
```

**Important:** Always `memset` your struct to zero before writing.  
Without it, the padding bytes contain random stack garbage — this leaks data and makes your hex dump unreadable.

### Always Check fopen

```c
FILE *file = fopen("students.db", "wb");
if (file == NULL) {
    printf("File opening failed\n");
    return 1; // use 1 for error, not 0
}
// ...
fclose(file); // always close
```

---

## 3. Random Access with fseek

### Why Fixed-Size Records Enable O(1) Access

Because every record is exactly `sizeof(Student) = 60` bytes, the byte offset of record `i` is always:

```
offset = i * sizeof(Student)
```

No scanning, no searching. Direct jump to any record.

### fseek

```c
int fseek(FILE *stream, long offset, int whence);
```

- `stream` — file pointer
- `offset` — byte offset (relative to `whence`)
- `whence`  — reference point:
  - `SEEK_SET` — from start of file
  - `SEEK_CUR` — from current position
  - `SEEK_END` — from end of file

Returns 0 on success, non-zero on failure.

### Reading Record #42

```c
void read_record(FILE *file, int index, Student *out) {
    fseek(file, index * sizeof(Student), SEEK_SET);
    fread(out, sizeof(Student), 1, file);
}

// Usage
Student s;
read_record(file, 42, &s);
printf("ID: %d, Name: %s, Age: %d\n", s.id, s.name, s.age);
```

**File pointer behaviour after fread:**  
After `fread`, the file pointer automatically advances by the number of bytes read. So calling `read_record(file, 0, &s)` followed by `read_record(file, 1, &s)` works without seeking for the second call — but it's safer to always seek explicitly.

---

## 4. In-Place Updates

### The Problem with fread → modify → fwrite

```c
// WRONG — file pointer drifts
fseek(file, index * sizeof(Student), SEEK_SET);
fread(&s, sizeof(Student), 1, file);   // pointer now at index+1
// modify s...
fwrite(&s, sizeof(Student), 1, file);  // writes to WRONG position!
```

After `fread`, the file pointer is at the **next** record. Writing there corrupts the wrong record.

### Correct Approach — Seek Again Before Writing

```c
void update_record(FILE *file, int index, Student *updated) {
    fseek(file, index * sizeof(Student), SEEK_SET);
    fwrite(updated, sizeof(Student), 1, file);
}
```

No need to read first if you have the full updated struct. Just seek and overwrite.

### Why "rb+" Mode

`"rb+"` opens the file for both reading and writing without truncating.  
`"wb"` would wipe the entire file. `"rb"` is read-only.

---

## 5. Stack vs Heap Memory

### Where Variables Live

| Location | Allocated by     | Freed by           | Lifetime                  |
|----------|------------------|--------------------|---------------------------|
| Stack    | Compiler         | Compiler (auto)    | Until function returns    |
| Heap     | `malloc`/`calloc`| `free` (manual)    | Until you call `free`     |

### The Dangling Pointer Bug

```c
// WRONG — returns pointer to dead stack memory
char *int_to_string(int n) {
    char buffer[20];          // lives on the stack
    sprintf(buffer, "%d", n);
    return buffer;            // stack frame dies here — pointer is dangling!
}
```

This is **undefined behaviour**. It may "work" sometimes because the stack memory hasn't been overwritten yet — making it even more dangerous to debug.

### Fix 1 — Pass Buffer by Reference (Preferred)

```c
void int_to_string(int n, char *buffer) {
    sprintf(buffer, "%d", n);
    // caller owns and manages the buffer
}

// Usage
char buf[4];
int_to_string(42, buf);
```

The caller provides the buffer on its own stack — no allocation, no free, no leaks.

### Fix 2 — Heap Allocation (Caller Must Free)

```c
char *int_to_string(int n) {
    char *buffer = malloc(sizeof(char) * 4);
    sprintf(buffer, "%d", n);
    return buffer; // caller must free this!
}

// Usage
char *s = int_to_string(42);
// use s...
free(s); // mandatory — memory leak if forgotten
```

**Prefer Fix 1** for simple utilities. Use Fix 2 only when the buffer needs to outlive the calling scope.

### memset — Zero Out Memory

```c
void *memset(void *s, int c, size_t n);
```

Fills `n` bytes at pointer `s` with byte value `c`.

```c
Student student;
memset(&student, 0, sizeof(Student));
// all fields now zero, padding bytes zero — no garbage
```

Always `memset` structs before writing to binary files.

---

## 6. Strings in C

### String Literals are Read-Only

```c
// WRONG — segfault
char *name = "Student_";
strcat(name, "42"); // writing to read-only memory!

// CORRECT — writable stack array
char name[11] = "Student_";
strcat(name, "42"); // fine — array is on the stack
```

String literals live in the `.rodata` section of the binary — read-only. Any write attempt is a segfault.

### Arrays Cannot Be Assigned

```c
Student student;

// WRONG — compile error: array type not assignable
student.name = new_name;

// CORRECT — copy bytes
strcpy(student.name, new_name);
```

Arrays in C decay to pointers but they are **not** assignable. You must copy with `strcpy`, `memcpy`, or `strncpy`.

### Structs Must Be Self-Contained for Binary I/O

```c
// WRONG for binary files
typedef struct {
    int   id;
    char *name; // pointer — only 8 bytes of address written to disk!
    int   age;
} BadStudent;

// CORRECT — data lives inside the struct
typedef struct {
    int  id;
    char name[50]; // 50 bytes of actual string data
    int  age;
} Student;
```

When you `fwrite` a struct containing a pointer, you write the **memory address**, not the pointed-to data. On the next run, that address is meaningless.

---

## 7. Hex Dump & Raw Bytes

### Little-Endian Byte Order

x86/x64 machines are **little-endian** — the least significant byte is stored first.

```
id = 13 (0x0000000D) stored as:
Byte 0: 0D  ← least significant byte first
Byte 1: 00
Byte 2: 00
Byte 3: 00
```

Verify: `lscpu | grep "Byte Order"`

### Reading a Hex Dump

```
00 00 00 00  53 74 75 64 65 6e 74 5f 30  00 00 ...  23 00 00 00
^-- id=0 --^ ^---------- "Student_0" ----------^    ^- age=35 -^
```

- `00 00 00 00` → `id = 0`
- `53 74 75 64 65 6e 74 5f 30` → `S t u d e n t _ 0`
- `23 00 00 00` → `age = 0x23 = 35`

### Writing a Hex Dump Function

```c
void hex_dump(const char *filename, int num_records) {
    FILE *file = fopen(filename, "rb");
    if (!file) return;

    for (int i = 0; i < num_records; i++) {
        fseek(file, i * sizeof(Student), SEEK_SET);

        for (int byte = 0; byte < sizeof(Student); byte++) {
            unsigned char value;
            fread(&value, 1, 1, file);
            printf("%02x ", value); // always 2 digits, zero-padded
        }
        printf("\n");
    }
    fclose(file);
}
```

**Key detail:** Use `%02x` not `%x`.  
`%x` prints `d` for 13 — ambiguous.  
`%02x` prints `0d` — always 2 digits, standard hex dump format.

**Key detail:** Use `unsigned char` not `char`.  
Signed char would sign-extend values > 127, printing `ffffffa0` instead of `a0`.

### What memset Fixes in the Hex Dump

Before `memset`:
```
00 00 00 00  53 74 75 64 65 6e 74 5f 30  20 dd c5 86 6a 7f ...
                                          ^--- garbage stack data ---^
```

After `memset(&student, 0, sizeof(Student))`:
```
00 00 00 00  53 74 75 64 65 6e 74 5f 30  00 00 00 00 00 00 ...
                                          ^--- clean zeros ---^
```

---

## 8. Packed Structs & Layout Corruption

### __attribute__((packed))

Forces the compiler to remove all padding. Fields are packed tightly.

```c
typedef struct __attribute__((packed)) {
    int  id;       // 4 bytes, offset 0
    char name[50]; // 50 bytes, offset 4
    int  age;      // 4 bytes, offset 54 — no padding!
} StudentPacked;

sizeof(StudentPacked) = 58  // vs 60 for normal Student
```

**Trade-off:** Saves 2 bytes per record, but unaligned reads are slower (or impossible on some architectures).

### The Corruption Experiment

Write 5 records as `StudentPacked` (58 bytes each), read back as `Student` (60 bytes each):

```
File layout (packed, 58-byte records):
[record 0: bytes 0–57][record 1: bytes 58–115][record 2: bytes 116–173]...

fread reads 60 bytes at a time:
Read 1: bytes 0–59   → record 0 + 2 bytes of record 1 → age is WRONG
Read 2: bytes 60–119 → 2 bytes from record 1 + record 2 body → id is WRONG
Read 3: bytes 120–179 → misaligned further...
```

Output:
```
id='0'           name='Student_0'  age='65536'       ← age wrong
id='1951596544'  name='udent_1'    age='2'            ← id wrong, name shifted
id='1685419091'  name='ent_2'      age='1951596544'   ← everything wrong
```

**Lesson:** Never mix struct layouts when reading binary files. Schema changes require data migration — you cannot just change a struct and re-read old files.

---

## 9. Linear Scan vs Random Access

### find_by_name — O(n)

```c
int find_by_name(FILE *file, const char *name, Student *out) {
    fseek(file, 0, SEEK_SET);

    for (int row = 0; row < 100; row++) {
        fread(out, sizeof(Student), 1, file);
        if (strcmp(out->name, name) == 0) {
            return row; // found
        }
    }
    return -1; // not found
}
```

**Complexity:** O(n × m) where n = number of records, m = string length.  
At 1 million records: up to 1 million `fread` calls + 1 million `strcmp` calls.

### read_record — O(1)

```c
void read_record(FILE *file, int index, Student *out) {
    fseek(file, index * sizeof(Student), SEEK_SET);
    fread(out, sizeof(Student), 1, file);
}
```

**Complexity:** O(1). Always 1 `fseek` + 1 `fread`, regardless of file size.  
Works because `id` maps directly to a predictable byte offset.

### The Core Insight

| Operation         | 100 records | 1M records |
|-------------------|-------------|------------|
| `read_record(42)` | 1 fread     | 1 fread    |
| `find_by_name(x)` | up to 100   | up to 1M   |

**This is exactly the problem B-Trees solve.**  
When you need fast lookup by a non-offset key (name, email, etc.), you need an index structure that gives O(log n) instead of O(n).  
A B-Tree is that structure.

---

## 10. Soft Delete — Tombstone Pattern

### Why Not Hard Delete?

If you delete record #42 by physically removing it and shifting records #43–#99:
- Cost: O(n) — must rewrite the entire rest of the file
- Risk: If the process crashes mid-shift, the file is permanently corrupted
- Complexity: All stored offsets/indexes become invalid

### The Tombstone Pattern

Mark a record as deleted with a flag. Leave it in place. Skip it on reads.

```c
#include <stdbool.h>

typedef struct {
    bool is_deleted; // 1 byte + 3 padding = effectively 4 bytes with alignment
    int  id;
    char name[50];
    int  age;
} SoftStudent; // sizeof = 64
```

### delete_record

```c
void delete_record(FILE *file, int index) {
    // seek to the record
    fseek(file, index * sizeof(SoftStudent), SEEK_SET);

    SoftStudent s;
    fread(&s, sizeof(SoftStudent), 1, file);

    if (s.is_deleted) {
        printf("Already deleted\n");
        return;
    }

    s.is_deleted = true;

    // seek back — fread moved the pointer!
    fseek(file, index * sizeof(SoftStudent), SEEK_SET);
    fwrite(&s, sizeof(SoftStudent), 1, file);
}
```

**Note:** Here we need to `fread` first (to preserve other fields), so we must seek again before writing. This is different from `update_record` where we had the full updated struct ready.

### print_db with Skip

```c
void print_db_skip_deleted(FILE *file, int num_records) {
    fseek(file, 0, SEEK_SET);
    for (int i = 0; i < num_records; i++) {
        SoftStudent s;
        fread(&s, sizeof(SoftStudent), 1, file);
        if (!s.is_deleted) {
            printf("id=%d name=%s age=%d\n", s.id, s.name, s.age);
        }
    }
}
```

**Idempotency:** Calling `delete_record` twice is safe — the check prevents double-delete. This is a good habit for any destructive operation.

### Real-World Usage

PostgreSQL calls these "dead tuples." It runs a background process called **VACUUM** that periodically reclaims space from dead tuples. The pattern is identical to what you built.

---

## 11. Mistake Log

### Mistake #1 — Returning Stack Pointer
**Severity:** Crash / Undefined Behaviour

```c
// WRONG
char *int_to_string(int n) {
    char buffer[20]; // stack — dies when function returns
    sprintf(buffer, "%d", n);
    return buffer;   // dangling pointer
}

// FIXED
void int_to_string(int n, char *buffer) {
    sprintf(buffer, "%d", n); // caller owns buffer
}
```

**Lesson:** Never return a pointer to a local (stack) variable.

---

### Mistake #2 — Writing to a String Literal
**Severity:** Segfault

```c
// WRONG
char *name = "Student_";
strcat(name, "42"); // "Student_" is read-only — SEGFAULT

// FIXED
char name[11] = "Student_";
strcat(name, "42"); // writable stack array
```

**Lesson:** String literals live in read-only memory. Declare a char array for mutable strings.

---

### Mistake #3 — Wrong nmemb in fwrite
**Severity:** Silent data corruption

```c
// WRONG — inside a loop writing 1 student at a time
fwrite(&student, sizeof(Student), 100, file);
// writes 1 valid struct + 99 structs of garbage memory!

// FIXED
fwrite(&student, sizeof(Student), 1, file);
```

**Lesson:** `nmemb` = how many items to write. If you're writing one at a time, it's always 1.

---

### Mistake #4 — Array Assignment
**Severity:** Compile error

```c
// WRONG — arrays are not assignable in C
student.name = new_name;

// FIXED
strcpy(student.name, new_name);
```

**Lesson:** Arrays in C are not first-class values. Use `strcpy`/`memcpy` to copy them.

---

### Mistake #5 — Pointer Instead of Array in Struct
**Severity:** Silent data corruption on read

```c
// WRONG for binary files
typedef struct {
    char *name; // only writes 8-byte pointer address to disk
} Bad;

// FIXED
typedef struct {
    char name[50]; // 50 bytes of actual data inside the struct
} Good;
```

**Lesson:** Structs written to binary files must be self-contained. No pointers to external data.

---

### Mistake #6 — Reading from "wb" File
**Severity:** Returns zeros / garbage

```c
// WRONG
FILE *f = fopen("x.db", "wb"); // write-only
fread(&s, sizeof(Student), 1, f); // always returns 0 bytes

// FIXED
fclose(f);
f = fopen("x.db", "rb"); // reopen for reading
fread(&s, sizeof(Student), 1, f);
```

**Lesson:** Know your file modes. `"wb"` is write-only — reading from it returns nothing.

---

### Mistake #7 — File Pointer Drift After fread
**Severity:** Writes to wrong record

```c
// WRONG
fseek(file, index * sizeof(Student), SEEK_SET);
fread(&s, sizeof(Student), 1, file); // pointer moves to index+1!
// modify s...
fwrite(&s, sizeof(Student), 1, file); // writes to index+1, not index!

// FIXED — seek again before writing
fseek(file, index * sizeof(Student), SEEK_SET);
fread(&s, sizeof(Student), 1, file);
// modify s...
fseek(file, index * sizeof(Student), SEEK_SET); // seek back
fwrite(&s, sizeof(Student), 1, file);

// OR — if you have the full updated struct, skip the read entirely
fseek(file, index * sizeof(Student), SEEK_SET);
fwrite(updated, sizeof(Student), 1, file);
```

**Lesson:** Every `fread`/`fwrite` advances the file pointer. Always seek to the correct position before writing.

---

### Mistake #8 — %x Instead of %02x in Hex Dump
**Severity:** Misleading output

```c
// WRONG — single hex digits print without zero-padding
printf("%x ", value);  // 13 prints as "d", not "0d"

// FIXED
printf("%02x ", value); // always 2 digits, zero-padded
```

**Lesson:** `%02x` means: print as hex, minimum 2 characters, zero-pad if needed.

---

### Mistake #9 — Calling int_to_string Twice (Memory Leak)
**Severity:** Memory leak

```c
// WRONG — two allocations, only one freed
char *buffer = int_to_string(i);    // allocation #1
strcat(name, int_to_string(i));     // allocation #2 — leaked!
free(buffer);                       // only frees #1

// FIXED — call once, reuse
char *buffer = int_to_string(i);
strcat(name, buffer);
free(buffer);
```

**Lesson:** Every `malloc` needs exactly one `free`. Count your allocations.

---

## 12. Git History

```
feat:     add Student struct with include guard
feat:     write 100 Student records to binary file
feat:     random access read of Student record using fseek
feat:     update Student record in-place using fseek
refactor: remove malloc from int_to_string, use pass-by-reference
feat:     add hex_dump utility to inspect raw binary file
feat:     demonstrate packed struct corruption when mixing layouts
feat:     add find_by_name linear scan, demonstrates O(n) vs O(1) access
feat:     soft delete with is_deleted flag, skip deleted in print_db
```

---

## Key Takeaways

1. **sizeof lies** — always account for padding. Calculate manually, then verify.
2. **Binary files need self-contained structs** — no pointers inside structs you write to disk.
3. **Fixed-size records = O(1) access** — this is the foundation of database storage.
4. **File pointer always moves** — after any `fread`/`fwrite`, know where you are.
5. **memset before fwrite** — never write uninitialised padding bytes to disk.
6. **Schema changes break old data** — mixing struct layouts corrupts everything silently.
7. **O(n) scan at scale is painful** — this is why B-Trees exist.
8. **Soft delete > hard delete** — O(1), safe, reversible.

---

*Phase 2 → Memory Mastery: malloc, realloc, double pointers, in-memory page cache.*
