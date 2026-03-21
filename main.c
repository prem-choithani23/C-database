#include <stdio.h>
#include <stdlib.h>

#include "student.h"
#include <time.h>
#include <string.h>

char * int_to_string(int n)
{
    char * buffer = (char*)malloc(sizeof(char)*3);

    sprintf(buffer  ,"%d" , n );

    return buffer;
}

void read_record(FILE * file , int index , Student *out)
{
    int stream = fseek(file, (index) * sizeof(Student) , SEEK_SET);

    fread(out , sizeof(Student) , 1 , file);

    printf("%d" , stream);
}

void update_record(FILE * file , int index , Student *updated)
{
    int stream = fseek(file , index*sizeof(Student) ,SEEK_SET );


}

void insert_rows(FILE * file)
{



    size_t size_of_item = sizeof(Student);
    char new_name[50];



    int num_items = 100;

    for (int i=0;i<num_items;i++)
    {
        char name [11] = "Student_";

        char * buffer= int_to_string(i);

        strcat(name , buffer);


        strcpy(new_name, name);
        free(buffer);

        int age = 15 + rand()%30;

        Student student;

        student.id = i;
        strcpy(student.name ,new_name);
        student.age = age;

        int write = fwrite( &student,size_of_item , 1, file);
        if (write == 0)
        {
            printf("Write failed...");
        }
    }
}
int main(void)
{
    srand(time(NULL));
    FILE * file = fopen("students.db" ,"rb+");


    if (file == NULL)
    {
        printf("File opening failed");
        return 0;
    }

    // write 100 rows
    // insert_rows(file);


    int search_id = 13;
    Student s;
    read_record(file , search_id , &s);
    printf("\nStudent : (id='%d'  , name='%s' , age='%d')\n" , s.id, s.name , s.age);
    fclose(file);


    return 0;
}