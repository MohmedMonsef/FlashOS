#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "../Structs/structs.h"

struct Memory memory;
bool isMemoryCreated=false;
struct Block{
    int processId;
    int startOffset;
    struct Block* nextBlock;
};

struct BlockList{
    struct Block* head;
    int freeCount;
};

struct Memory
{
   struct BlockList memory[10];
};

struct Block* createBlock(int processId,int offset){

    struct Block *newProcess = (struct Block *)malloc(sizeof(struct Block));
    newProcess->processId = processId;
    newProcess->startOffset = offset;
    newProcess->nextBlock = NULL;
    return newProcess;
}

bool createMemory(){
    if(isMemoryCreated){
        return false;
    }
    for (int i = 0; i < 9; i++)
    {
        struct BlockList bl= {NULL,0};
        memory.memory[i]=bl;
    }

    struct Block* block=createBlock(-1,0);
    struct BlockList blList= {block,1};
    memory.memory[9]=blList;
    return isMemoryCreated=true;
}

bool insertList(struct Block newProcess){

}

int SearchAndDeleteList(int listIndex,int processId){
    printf("list index %d \n ",listIndex);
    struct Block* tmpHead=memory.memory[listIndex].head;
    struct Block* next=tmpHead;
    struct Block* prev=NULL;
    int offset=-1;
    printf("w\n");
    while (next && next->processId!=processId)
    {
        prev=next;
        next=next->nextBlock;
        
    }
    if(prev==NULL){
        memory.memory[listIndex].head=next->nextBlock;
    }
    else
    {
        prev->nextBlock=next->nextBlock;
    }
    offset=next->startOffset;
    free(next);
    next=NULL;
    memory.memory[listIndex].freeCount-=1;
    return offset;
}

bool InsertList(int listIndex,int offset,int processId){
    if (isMemoryCreated == false)
    {
        return false;
    }

    struct Block* bl=createBlock(processId,offset);

    if (memory.memory[listIndex].head == NULL)
    {
        memory.memory[listIndex].head  = bl;
        return true;
    }
    struct Block *first = NULL;
    struct Block *second = memory.memory[listIndex].head ;

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
    return true;
}

bool updateList(int listIndex,int processId){

    if (isMemoryCreated == false)
    {
        return false;
    }
    
    struct Block *tmp = memory.memory[listIndex].head ;

    while (tmp != NULL && tmp->processId!=-1 )
    {
        tmp = tmp->nextBlock;
    }

    if (tmp == NULL)
    {
       return false;
    }
    else
    {
        tmp->processId=processId;
    }
    return tmp->startOffset;

}

bool insertInMemory(struct Process process){
    int indexInMem = ceil(log(process.memSize)/(log(2)) - 1);
    int tmpIndex=indexInMem;
    while (memory.memory[tmpIndex].freeCount<=0)
    {
        tmpIndex+=1;
        if(tmpIndex==10){
            return false;
        }
    }
    if(tmpIndex==indexInMem){
        updateList(tmpIndex,process.id);
        memory.memory[tmpIndex].freeCount-=1;
        return true;
    }
    int insetOffset=SearchAndDeleteList(tmpIndex,-1);
    while(tmpIndex!=indexInMem){
        tmpIndex-=1;
        InsertList(tmpIndex,2*insetOffset+1,-1);
        memory.memory[tmpIndex].freeCount+=1;
        insetOffset=insetOffset*2;
        
    }
    InsertList(tmpIndex,insetOffset,process.id);

}

bool deleteFromMemory(struct Process process){

}


void displayMemory(){

    for(int i=9;i>=0;i--){
        struct Block*tmp=memory.memory[i].head;
        printf("Memory index = %d , Free_Count= %d \n ",i,memory.memory[i].freeCount);
        while (tmp)
        {
            printf("Process id = %d , start Offset = %d \n",tmp->processId,tmp->startOffset);
            tmp=tmp->nextBlock;
        }
        
        
    }
}
