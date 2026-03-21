//
// Created by prem on 3/22/26.
//

#ifndef CDB_MODIFIED_STUDENT_H
#define CDB_MODIFIED_STUDENT_H
#include <stdbool.h>

typedef struct
{
    bool is_deleted;
    int id;
    char name[50];
    int age;
}SoftStudent;

#endif //CDB_MODIFIED_STUDENT_H