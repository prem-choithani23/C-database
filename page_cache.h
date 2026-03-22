//
// Created by prem on 3/22/26.
//

#ifndef CDB_PAGE_CACHE_H
#define CDB_PAGE_CACHE_H
#include "modified_student.h"
typedef struct entry PageEntry;

typedef struct
{
    PageEntry *pages;
    int capacity;
    int count;
    int clock;
}PageCache;

typedef struct entry
{
    SoftStudent student;
    int last_used;
}PageEntry;
#endif //CDB_PAGE_CACHE_H