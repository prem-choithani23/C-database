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
    // move the file ptr to correct indexed row using offset
    int stream = fseek(file, (index) * sizeof(Student) , SEEK_SET);

    // read the row in location pointed by 'out'
    fread(out , sizeof(Student) , 1 , file);

}


void update_record(FILE * file , int index , Student *updated)
{
    // move the file ptr to correct indexed row using offset
    int stream = fseek(file , index*sizeof(Student) ,SEEK_SET );

    // write the row with contents present in location pointed by 'updated'

    fwrite(updated , sizeof(Student) , 1 ,file);

    printf("Update Success...\n");

}

void insert_rows(FILE * file)
{
    // define size of item
    size_t size_of_item = sizeof(Student);

    // temp array for storing names
    char new_name[50];


    // number of items to write
    int num_items = 100;

    for (int i=0;i<num_items;i++)
    {
        // all students having name of form 'Student_id'

        char name [11] = "Student_";
        char id[3];

        // converting id to string
        int_to_string(i , id);

        // append the converted int to name
        strcat(name , id);

        // as arrays cannot be changes by simply changie the reference , we do cpy
        strcpy(new_name, name);

        // define a random age
        int age = 15 + rand()%30;

        Student student;
        memset(&student , 0 , sizeof(Student));

        // set id
        student.id = i;

        // set name ( using cpy) and not 'new_name = name'
        strcpy(student.name ,new_name);



        // set age
        student.age = age;

        // write 'size_of_item' bytes with the contents pointed by &s , 1 time . at location pointed by file
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

void hex_dump(const char * filename ,int num_records)
{
    FILE * file = fopen(filename , "rb");

    for (int i=0;i<num_records;i++)
    {
        int stream = fseek(file, i*sizeof(Student) ,SEEK_SET);

        for (int byte = 0;byte<sizeof(Student);byte++)
        {


            unsigned char value;
            fread(&value , 1  , 1 , file);

            // zero pad printing (hex value in byte)
            printf("%02x " , value);

        }
        printf("\n");
    }

    fclose(file);
}

int main(int argc, char* argv[])
{
    hex_dump("students.db" , 100);
}

// int main(void)
// {
//     srand(time(NULL));
//     FILE * file = fopen("students.db" ,"rb+");
//
//
//     if (file == NULL)
//     {
//         printf("File opening failed");
//         return 0;
//     }
//
//     // write 100 rows
//     insert_rows(file);
//
//
//     // search key
//     int search_id = 13;
//     Student s;
//     // read record into s
//     read_record(file , search_id , &s);
//
//     // print s
//     printf("Before Update : ");
//     print_student(s);
//
//
//     // modified student
//     Student updated = {search_id , "Ramesh",37};
//
//     // replace contents of row 'index' by contents of 'updated'
//     update_record(file , search_id , &updated);
//
//
//     printf("After Update : ");
//
//     // read record into s
//     read_record(file , search_id , &s);
//     // print s
//     print_student(s);
//
//     // print all rows (sample)
//     // print_db(file , 100);
//
//     // close file connection
//     fclose(file);
//
//     return 0;
// }