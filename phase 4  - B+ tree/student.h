//
// Created by prem on 3/23/26.
//

#ifndef CDB_STUDENT_H
#define CDB_STUDENT_H
#include <stdbool.h>

typedef struct
{
    bool is_deleted;
    int id;
    char name[50];
    int age;
}Student;

#endif //CDB_STUDENT_H