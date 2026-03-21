//
// Created by prem on 3/21/26.
//

#ifndef CDB_STUDENT_PACKED_H
#define CDB_STUDENT_PACKED_H

typedef struct __attribute((packed)){
    int id;
    char name[50];
    int age;
}StudentPacked;

#endif //CDB_STUDENT_PACKED_H