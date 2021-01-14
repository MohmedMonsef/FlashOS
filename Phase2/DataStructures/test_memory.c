#include "./memory.h"
struct Process process[5] = {{1, 3, 29, 8, 10},
                             {2, 3, 29, 8, 8},
                             {3, 13, 8, 9, 100},
                             {9, 43, 24, 3, 36},
                             {10, 49, 8, 10, 400}};
int main()
{
    createMemory();
    for (int i = 0; i < 5; i++)
    {
        printf("Insert  Process %d\n", i);
        insertInMemory(process[i]);
        displayMemory();
    }
    for (int i = 0; i < 5; i++)
    {
        printf("Delete  Process %d\n", i);
        deleteFromMemory(process[i]);
        displayMemory();
    }
    for (int i = 0; i < 5; i++)
    {
        printf("Insert  Process %d\n", i);
        insertInMemory(process[i]);
        displayMemory();
    }
    for (int i = 0; i < 5; i++)
    {
        printf("Delete  Process %d\n", i);
        deleteFromMemory(process[i]);
        displayMemory();
    }
}