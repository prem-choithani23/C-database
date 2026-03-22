//
// Created by prem on 3/22/26.
//
#include "modified_student.h"
#include "page_cache.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PageCache create_cache(int capacity)
{
    PageCache cache;

    cache.capacity = capacity;
    cache.count = 0;
    cache.pages = (SoftStudent*)malloc(sizeof(SoftStudent)*capacity);

    return cache;
}
void free_cache(PageCache *cache) {
    free(cache->pages);
    cache->pages = NULL;
    cache->count = 0;
    cache->capacity = 0;
}
bool is_full(PageCache *cache)
{
    return cache->count == cache->capacity;
}

bool is_empty(PageCache *cache)
{
    return cache->count == 0;
}
int put(PageCache * cache, SoftStudent * student)
{
    if (is_full(cache))
    {
        printf("Cache full...operation failed...\n");
        return 0;
    }

    cache->pages[cache->count] = *student;
    cache->count++;
    return 1;
}
int get(PageCache * cache , int id,SoftStudent *student)
{
    for (int i=0;i<cache->count;i++)
    {
        SoftStudent s = cache->pages[i];
        if (s.id == id)
        {
            *student = s;
            return 1;

        }
    }
    printf("No student with id: %d found in cache...\n",id);
    return 0;
}
void load_records(PageCache *cache , int start_id , int end_id, FILE * file)
{
    int stream = fseek(file , start_id*sizeof(SoftStudent) , SEEK_SET);

    SoftStudent student;
    for (int i=start_id;i<=end_id;i++)
    {
        fread(&student , sizeof(SoftStudent), 1, file);
        put(cache , &student);
    }

}

void printSoftStudent(const SoftStudent *s) {
    if (s == NULL) {
        printf("SoftStudent: NULL pointer\n");
        return;
    }

    printf("====================================\n");
    printf("           SoftStudent Record        \n");
    printf("====================================\n");
    printf(" ID        : %d\n", s->id);
    printf(" Name      : %s\n", s->name);
    printf(" Age       : %d\n", s->age);
    printf(" Deleted   : %s\n", s->is_deleted ? "Yes" : "No");
    printf("====================================\n");
}

int main(int argc, char* argv[])
{
    SoftStudent s;
    PageCache cache = create_cache(10);

    FILE * file = fopen("soft_students.db" , "rb");

    load_records(&cache , 0 , 9 , file);

    printf("Size of cache : %d\n" , cache.count);

    int found1 = get(&cache , 5 , &s);
    if (found1)
    {
        printSoftStudent(&s);
    }
    int found2 = get(&cache , 99 , &s);
    if (found2)
    {
        printSoftStudent(&s);
    }


    fclose(file);
    free_cache(&cache);

    return 0;
}
