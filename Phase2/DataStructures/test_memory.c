#include "./memory.h"
struct Process process[5] = {{1, 3, 29, 8,10},
                            {1, 3, 29, 8,8},
                              {2, 13, 8, 9,300},
                              {9, 43, 24, 3,36},
                              {10, 49, 8, 10,600}};
int main()
{
    createMemory();
        displayMemory();
    for (int i = 0; i < 5; i++)
    {
        printf("Inser New Process\n");
        insertInMemory(process[i]);
        displayMemory();
    }
   
}