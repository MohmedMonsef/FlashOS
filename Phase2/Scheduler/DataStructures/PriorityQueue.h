#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// The Main Priority Queue
struct PriorityQueue *PriorityQueue = NULL;

struct Node *createNewNode(struct Process process)
{
    struct Node *newProcess = (struct Node *)malloc(sizeof(struct Node));
    newProcess->process = process;
    newProcess->next = NULL;
    return newProcess;
}
struct LinkedList *createNewLinkedList(struct Process process)
{
    struct Node *node = createNewNode(process);
    struct LinkedList *newLinkedList = (struct LinkedList *)malloc(sizeof(struct LinkedList));
    newLinkedList->head = node;
    newLinkedList->tail = node;
    newLinkedList->nextList = NULL;
    return newLinkedList;
}

/*
@Description : Create New Instance Of A Priority Queue - return NULL if there is already one
@Param type : Type Of Algo 0=>STRN , 1=>HPF 
@Return bool
*/

bool createPriorityQueue(int type)
{
    if (PriorityQueue != NULL)
    {
        return false;
    }
    PriorityQueue = (struct PriorityQueue *)malloc(sizeof(struct PriorityQueue));
    PriorityQueue->front = NULL;
    PriorityQueue->tail = NULL;
    PriorityQueue->type = type;
    return true;
}

/*
@Description : Push New Process To The Queue
@Param process : The Process Needs To Be Inserted
@Return bool
*/

bool pushProcess(struct Process process)
{
    if (PriorityQueue == NULL)
    {
        return false;
    }
    struct LinkedList *ls = createNewLinkedList(process);

    if (PriorityQueue->front == NULL)
    {
        PriorityQueue->front = ls;
        PriorityQueue->tail = ls;
        return true;
    }
    struct LinkedList *first = NULL;
    struct LinkedList *second = PriorityQueue->front;
    int processPriority = (PriorityQueue->type == 0) ? process.runtime : process.priority;

    int currentPriority = (PriorityQueue->type == 0) ? second->head->process.runtime : second->head->process.priority;
    while (second != NULL && currentPriority < processPriority)
    {
        first = second;
        second = second->nextList;
        currentPriority = (second == NULL) ? 0 : (PriorityQueue->type == 0) ? second->head->process.runtime : second->head->process.priority;
    }

    if (second == NULL)
    {
        first->nextList = ls;
        PriorityQueue->tail = ls;
    }
    else if (currentPriority == processPriority)
    {
        struct Node *nd = createNewNode(process);
        second->tail->next = nd;
        second->tail = nd;
    }
    else if (first == NULL)
    {
        ls->nextList = second;
        PriorityQueue->front = ls;
    }
    else
    {
        ls->nextList = second;
        first->nextList = ls;
    }
    return true;
}

/*
@Description : Pop The Front Process In The Queue
@Return : Pointer to Struct Process
*/

struct Process *popProcess()
{
    if (PriorityQueue == NULL || PriorityQueue->front == NULL)
    {
        return NULL;
    }

    struct Process *frontProcess = &PriorityQueue->front->head->process;

    if (PriorityQueue->front->head == PriorityQueue->front->tail)
    {
        struct LinkedList *del = PriorityQueue->front;
        PriorityQueue->front = PriorityQueue->front->nextList;
        if (PriorityQueue->front == NULL)
        {
            PriorityQueue->tail = NULL;
        }
    }
    else
    {
        struct Node *del = PriorityQueue->front->head;
        PriorityQueue->front->head = PriorityQueue->front->head->next;
    }
    return frontProcess;
}

/*
@Description : Get The Front Process In The Queue
@Return : Pointer to Struct Process
*/

struct Process *getFrontProcess()
{
    if (PriorityQueue == NULL || PriorityQueue->front == NULL)
    {
        return NULL;
    }

    struct Process *frontProcess = &PriorityQueue->front->head->process;
    return frontProcess;
}

/*
@Description : Check If The Queue Is Empty
@Return bool
*/

bool isEmpty()
{
    if (PriorityQueue == NULL || PriorityQueue->front == NULL)
    {
        return true;
    }
    return false;
}

/*
@Description : Destruct The Whole Queue
@Return bool
*/

bool destructQueue()
{
    if (PriorityQueue != NULL)
    {
        while (PriorityQueue->front != NULL)
        {
            popProcess();
        }

        free(PriorityQueue);
        PriorityQueue = NULL;
    }
    return true;
}

/*
@Description : Display The Whole Queue
*/

void displayQueue()
{
    struct LinkedList *tmp = PriorityQueue->front;
    while (tmp != NULL)
    {
        if (PriorityQueue->type == 0)
        {
            struct Node *tmpNode = tmp->head;
            while (tmpNode != NULL)
            {
                printf("%d  id= %d  \n", tmpNode->process.runtime, tmpNode->process.id);
                tmpNode = tmpNode->next;
            }
        }
        else
        {
            struct Node *tmpNode = tmp->head;
            while (tmpNode != NULL)
            {
                printf("%d  id= %d  \n", tmpNode->process.priority, tmpNode->process.id);
                tmpNode = tmpNode->next;
            }
        }
        tmp = tmp->nextList;
    }

    return;
}