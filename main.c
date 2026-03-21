#include <stdio.h>
#include <stdlib.h>

#include "student.h"
#include <time.h>
#include <string.h>

void int_to_string(int n , char * buffer)
{
    sprintf(buffer  ,"%d" , n );
}

void read_record(FILE * file , int index , Student *out)
{
    int stream = fseek(file, (index) * sizeof(Student) , SEEK_SET);

    fread(out , sizeof(Student) , 1 , file);

}

void update_record(FILE * file , int index , Student *updated)
{
    int stream = fseek(file , index*sizeof(Student) ,SEEK_SET );

    fwrite(updated , sizeof(Student) , 1 ,file);

    printf("Update Success...\n");

}

void insert_rows(FILE * file)
{
    size_t size_of_item = sizeof(Student);
    char new_name[50];

    int num_items = 100;

    for (int i=0;i<num_items;i++)
    {
        char name [11] = "Student_";
        char id[3];

        int_to_string(i , id);

        strcat(name , id);


        strcpy(new_name, name);

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

void print_student(Student s)
{
    printf("Student : (id='%d'  , name='%s' , age='%d')\n" , s.id, s.name , s.age);
}

void print_db(FILE * file ,int num_records)
{
    for (int i=0;i<100;i++)
    {
        Student s;
        read_record(file , i , &s);
        print_student(s);
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
    insert_rows(file);


    int search_id = 13;
    Student s;
    read_record(file , search_id , &s);



    printf("Before Update : ");
    print_student(s);

    Student updated = {search_id , "Ramesh",37};

    update_record(file , search_id , &updated);

    printf("After Update : ");
    read_record(file , search_id , &s);
    print_student(s);


    print_db(file , 100);

    fclose(file);

    return 0;
}