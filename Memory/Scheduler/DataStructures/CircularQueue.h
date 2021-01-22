#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct CircularQueue*initiate(struct CircularQueue*Q)
{
	Q = (struct CircularQueue*) malloc(sizeof(struct CircularQueue));
	Q->head = NULL;
	Q->last = NULL;
	return Q;
}
//check if queue is empty or not
bool Empty(struct CircularQueue*Q) {
   return Q->head == NULL;
}
//insert new process in Circular Queue
void CircularQueueInsert(struct CircularQueue*Q,struct Process P) {

   struct Node *New = (struct Node*) malloc(sizeof(struct Node));
   New->process = P;
   if (Empty(Q)) {
      Q->head = New;
      Q->last = Q->head;
   } else {
      Q->last->next = New;
      Q->last = New;
   }    
   New->next = NULL;
}

//delete first item
struct Node * CircularQueueDeleteFirst(struct CircularQueue*Q) {

   struct Node *temp = Q->head;
	if(!temp) return NULL;
   if(Q->head->next == Q->head) {  
      Q->head = NULL;
      return temp;
   }     

   Q->head = Q->head->next;
   return temp;
}

//display the list
void PrintCircularQueue(struct CircularQueue*Q) {

   struct Node *ptr = Q->head;
   printf("\n[ ");
	
   //start from the beginning
   if(Q->head != NULL) {	
      while(ptr->next != NULL) {     
         printf("(%d,%d,%d,%d) ",ptr->process.id,ptr->process.arrival,ptr->process.runtime,ptr->process.priority);
         ptr = ptr->next;
      }
      printf("(%d,%d,%d,%d) ",ptr->process.id,ptr->process.arrival,ptr->process.runtime,ptr->process.priority);
   }
	
   printf(" ]");
}

