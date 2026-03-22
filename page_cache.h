//
// Created by prem on 3/22/26.
//

#ifndef CDB_PAGE_CACHE_H
#define CDB_PAGE_CACHE_H
#include "modified_student.h"
typedef struct
{
    SoftStudent *pages;
    int capacity;
    int count;
}PageCache;
#endif //CDB_PAGE_CACHE_H