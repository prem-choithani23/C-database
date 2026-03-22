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
    cache.clock=0;
    cache.pages = (PageEntry*)malloc(sizeof(PageEntry)*capacity);

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

int find_min(PageCache *cache)
{
    int min_idx = 0;
    int min_clock = cache->pages[0].last_used;

    for (int i=0;i<cache->count;i++)
    {
        if (cache->pages[i].last_used < min_clock)
        {
            min_clock = cache->pages[i].last_used;
            min_idx = i;
        }
    }

    return min_idx;
}

int put(PageCache * cache, SoftStudent * student)
{
    PageEntry entry;
    entry.student = *student;
    entry.last_used = cache->clock;

    if (is_full(cache))
    {
        int min_idx = find_min(cache);
        cache->pages[min_idx] = entry;
        cache->clock++;
        return 0;
    }

    cache->clock++;
    cache->pages[cache->count] = entry;
    cache->count++;
    return 1;
}

void read_entry(FILE * file , int id , SoftStudent *s)
{
    int stream = fseek(file , id*sizeof(SoftStudent) , SEEK_SET);
    fread(s , sizeof(SoftStudent) , 1 , file);
}

int get(PageCache * cache , int id,SoftStudent *student)
{
    for (int i=0;i<cache->count;i++)
    {
        PageEntry *entry = &cache->pages[i];
        if (entry->student.id == id)
        {
            // update first
            entry->last_used = cache->clock;
            cache->clock++;

            // fill later
            *student = entry->student;
            return 1;

        }
    }

    SoftStudent s;
    FILE * file = fopen("soft_students.db" , "rb");
    read_entry(file , id ,&s);
    fclose(file);

    put(cache , &s);


    return 1;
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

void printPageCache(const PageCache *cache) {
    if (!cache) {
        printf("PageCache: NULL\n");
        return;
    }

    printf("\n");
    printf("============================================================\n");
    printf("                      PAGE CACHE                            \n");
    printf("============================================================\n");
    printf(" Capacity : %d\n", cache->capacity);
    printf(" Count    : %d\n", cache->count);
    printf(" Clock    : %d\n", cache->clock);
    printf("------------------------------------------------------------\n");

    printf("| Slot | ID  | Name                 | Age | Del | LastUsed |\n");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < cache->capacity; i++) {
        PageEntry *p = &cache->pages[i];

        // Empty slot detection (optional logic)
        if (p->student.id == 0 && p->last_used == 0) {
            printf("| %-4d | %-3s | %-20s | %-3s | %-3s | %-8s |\n",
                   i, "-", "EMPTY", "-", "-", "-");
            continue;
        }

        printf("| %-4d | %-3d | %-20s | %-3d | %-3s | %-8d |\n",
               i,
               p->student.id,
               p->student.name,
               p->student.age,
               p->student.is_deleted ? "Y" : "N",
               p->last_used);
    }

    printf("============================================================\n");
}

int main(int argc, char* argv[])
{
    SoftStudent s;
    PageCache cache = create_cache(10);

    FILE * file = fopen("soft_students.db" , "rb");

    load_records(&cache , 0 , 9 , file);

    printPageCache(&cache);


    printf("Size of cache : %d\n" , cache.count);

    int found1 = get(&cache , 5 , &s);
    if (found1)
    {
        printSoftStudent(&s);
    }
    printPageCache(&cache);

    int found2 = get(&cache , 99 , &s);
    if (found2)
    {
        printSoftStudent(&s);
    }

    printPageCache(&cache);



    fclose(file);
    free_cache(&cache);

    return 0;
}
