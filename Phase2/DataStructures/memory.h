#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./utilities.h"
#include "../Structs/structs.h"

struct Memory memory;
bool isMemoryCreated = false;
struct Block
{
    int processId;
    int startOffset;
    struct Block *nextBlock;
};

struct BlockList
{
    struct Block *head;
    int freeCount;
};

struct Memory
{
    struct BlockList memory[10];
};

struct Block *createBlock(int processId, int offset)
{

    struct Block *newProcess = (struct Block *)malloc(sizeof(struct Block));
    newProcess->processId = processId;
    newProcess->startOffset = offset;
    newProcess->nextBlock = NULL;
    return newProcess;
}

bool createMemory()
{
    if (isMemoryCreated)
    {
        return false;
    }
    for (int i = 0; i < 9; i++)
    {
        struct BlockList bl = {NULL, 0};
        memory.memory[i] = bl;
    }

    struct Block *block = createBlock(-1, 0);
    struct BlockList blList = {block, 1};
    memory.memory[9] = blList;
    return isMemoryCreated = true;
}

int SearchAndDeleteListById(int listIndex, int processId)
{
    struct Block *tmpHead = memory.memory[listIndex].head;
    struct Block *next = tmpHead;
    struct Block *prev = NULL;
    int offset = -1;
    while (next && next->processId != processId)
    {
        prev = next;
        next = next->nextBlock;
    }
    if (prev == NULL)
    {
        memory.memory[listIndex].head = next->nextBlock;
    }
    else
    {
        prev->nextBlock = next->nextBlock;
    }
    offset = next->startOffset;
    free(next);
    next = NULL;
    memory.memory[listIndex].freeCount -= 1;
    return offset;
}

struct Block **InsertList(int listIndex, int offset, int processId, struct Block **affectedCells)
{
    if (listIndex >= 10)
    {
        return affectedCells;
    }
    struct Block *first = NULL;
    struct Block *second = memory.memory[listIndex].head;
    if (isMemoryCreated == false)
    {
        return affectedCells;
    }

    struct Block *bl = createBlock(processId, offset);

    if (memory.memory[listIndex].head == NULL)
    {
        memory.memory[listIndex].head = bl;
        affectedCells[0] = bl;
        return affectedCells;
    }

    int currentOffset = second->startOffset;
    while (second != NULL && currentOffset < offset)
    {
        first = second;
        second = second->nextBlock;
        currentOffset = (second == NULL) ? -1 : second->startOffset;
    }

    if (first == NULL)
    {
        bl->nextBlock = second;
        memory.memory[listIndex].head = bl;
    }
    else
    {
        bl->nextBlock = second;
        first->nextBlock = bl;
    }
    if (bl->startOffset % 2 == 0)
    {
        affectedCells[0] = bl;
        affectedCells[1] = second;
        return affectedCells;
    }
    affectedCells[0] = first;
    affectedCells[1] = bl;

    return affectedCells;
}

bool updateList(int listIndex, int processId)
{

    if (isMemoryCreated == false)
    {
        return false;
    }

    struct Block *tmp = memory.memory[listIndex].head;

    while (tmp != NULL && tmp->processId != -1)
    {
        tmp = tmp->nextBlock;
    }

    if (tmp == NULL)
    {
        return false;
    }
    else
    {
        tmp->processId = processId;
    }
    return tmp->startOffset;
}

bool insertInMemory(struct Process process)
{
    int indexInMem = log2_int(process.memSize) - 1;
    int tmpIndex = indexInMem;
    while (memory.memory[tmpIndex].freeCount <= 0)
    {
        tmpIndex += 1;
        if (tmpIndex == 10)
        {
            return false;
        }
    }
    if (tmpIndex == indexInMem)
    {
        updateList(tmpIndex, process.id);
        memory.memory[tmpIndex].freeCount -= 1;
        return true;
    }
    int insetOffset = SearchAndDeleteListById(tmpIndex, -1);
    struct Block *affectedLists[2] = {NULL, NULL};
    while (tmpIndex != indexInMem)
    {
        tmpIndex -= 1;
        InsertList(tmpIndex, 2 * insetOffset + 1, -1, affectedLists);
        memory.memory[tmpIndex].freeCount += 1;
        insetOffset = insetOffset * 2;
    }
    InsertList(tmpIndex, insetOffset, process.id, affectedLists);
}

void SearchAndDeleteListByOffset(int listIndex, int offset)
{
    struct Block *tmpHead = memory.memory[listIndex].head;
    struct Block *next = tmpHead;
    struct Block *prev = NULL;
    while (next && next->startOffset != offset)
    {
        prev = next;
        next = next->nextBlock;
    }
    if (next == NULL)
    {
        return;
    }
    if (prev == NULL)
    {
        memory.memory[listIndex].head = next->nextBlock;
    }
    else
    {
        prev->nextBlock = next->nextBlock;
    }
    free(next);
    next = NULL;
    memory.memory[listIndex].freeCount -= 1;
}

struct Block **freeSpaceMemory(int listIndex, int processId, struct Block **mergedCells)
{
    struct Block *tmp = memory.memory[listIndex].head;
    struct Block *prev = NULL;
    while (tmp && tmp->processId != processId)
    {
        prev = tmp;
        tmp = tmp->nextBlock;
    }
    if (tmp == NULL)
    {
        return mergedCells;
    }
    memory.memory[listIndex].freeCount += 1;

    tmp->processId = -1;
    if (tmp->startOffset % 2 == 0)
    {
        mergedCells[0] = tmp;
        mergedCells[1] = tmp->nextBlock;
        return mergedCells;
    }
    mergedCells[0] = prev;
    mergedCells[1] = tmp;
    return mergedCells;
}

bool mergeCells(int listIndex, struct Block **mergedCells)
{

    if (listIndex > 9)
    {
        return true;
    }
    if (mergedCells[0] == NULL || mergedCells[1] == NULL || mergedCells[0]->processId != -1 || mergedCells[1]->processId != -1)
    {
        return false;
    }
    if (mergedCells[0]->startOffset != mergedCells[1]->startOffset - 1)
    {
        return false;
    }
    int mergedOffset = mergedCells[0]->startOffset / 2;
    SearchAndDeleteListByOffset(listIndex, mergedCells[0]->startOffset);
    SearchAndDeleteListByOffset(listIndex, mergedCells[1]->startOffset);
    listIndex += 1;
    if (listIndex >= 10)
    {
        return false;
    }
    mergedCells[0] = NULL;
    mergedCells[1] = NULL;
    mergedCells = InsertList(listIndex, mergedOffset, -1, mergedCells);
    memory.memory[listIndex].freeCount += 1;
    mergeCells(listIndex, mergedCells);
    return true;
}

void deleteFromMemory(struct Process process)
{

    int indexInMem = log2_int(process.memSize) - 1;
    struct Block *mergedCells[2] = {NULL, NULL};
    struct Block **mergedCellsAddress = freeSpaceMemory(indexInMem, process.id, mergedCells);
    mergeCells(indexInMem, mergedCellsAddress);
}

void displayMemory()
{

    for (int i = 9; i >= 0; i--)
    {
        struct Block *tmp = memory.memory[i].head;
        printf("Memory index = %d , Free_Count= %d \n ", i, memory.memory[i].freeCount);
        while (tmp)
        {
            printf("Process id = %d , start Offset = %d \n", tmp->processId, tmp->startOffset);
            tmp = tmp->nextBlock;
        }
    }
}
