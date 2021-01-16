#include "../headers.h"
struct Process process[10] = {{1, 3, 29, 8},
                              {2, 13, 8, 9},
                              {3, 16, 17, 3},
                              {4, 21, 20, 5},
                              {5, 26, 10, 0},
                              {6, 36, 7, 3},
                              {7, 39, 14, 6},
                              {8, 41, 6, 8},
                              {9, 43, 24, 3},
                              {10, 49, 8, 10}};
int main()
{
    createPriorityQueue(1);
    for (int i = 0; i < 10; i++)
    {
        pushProcess(process[i]);
    }
    displayQueue();
    for (int i = 0; i < 10; i++)
    {
        struct Process *p = popProcess(process[i]);
        printf("cur process data %d  %d  %d  %d \n", p->id, p->arrival, p->priority, p->runtime);
    }
}