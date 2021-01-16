#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
//Process
struct Process
{
    int id;
    int arrival;
    int runtime;
    int priority;
};
//Node in CircularQueue
struct CircularQueueNode {
   struct Process Ps;
   struct CircularQueueNode *next;
};
//CircularQueue
struct CircularQueue
{
	struct CircularQueueNode *head;
	struct CircularQueueNode *last;
};
//Q is the Circular Queue You are Working on
//this is considered like a constructor in C++
struct CircularQueue*initiate(struct CircularQueue*Q)
{
	Q = (struct CircularQueue*) malloc(sizeof(struct CircularQueue));
	Q->head = NULL;
	Q->last = NULL;
	return Q;
}
//check if queue is empty or not
bool isEmpty(struct CircularQueue*Q) {
   return Q->head == NULL;
}
//Get number of processes in Queue
int CircularQueueLength(struct CircularQueue*Q) {
   int length = 0;
   if(Q->head == NULL) {
      return 0;
   }
   length++;
   struct CircularQueueNode *current = Q->head->next;
   while(current != Q->head) {
      length++;
      current = current->next;   
   }
	
   return length;
}
//insert new process in Circular Queue
void CircularQueueInsert(struct CircularQueue*Q,struct Process P) {

   struct CircularQueueNode *New = (struct CircularQueueNode*) malloc(sizeof(struct CircularQueueNode));
   New->Ps = P;
   if (isEmpty(Q)) {
      Q->head = New;
      Q->last = Q->head;
   } else {
      Q->last->next = New;
      Q->last = New;
   }    
   New->next = Q->head;
}

//delete first item
struct CircularQueueNode * CircularQueueDeleteFirst(struct CircularQueue*Q) {

   struct CircularQueueNode *temp = Q->head;
	
   if(Q->head->next == Q->head) {  
      Q->head = NULL;
      return temp;
   }     

   Q->head = Q->head->next;
   Q->last->next = Q->head;
   return temp;
}

//display the list
void PrintCircularQueue(struct CircularQueue*Q) {

   struct CircularQueueNode *ptr = Q->head;
   printf("\n[ ");
	
   //start from the beginning
   if(Q->head != NULL) {	
      while(ptr->next != Q->head) {     
         printf("(%d,%d,%d,%d) ",ptr->Ps.id,ptr->Ps.arrival,ptr->Ps.runtime,ptr->Ps.priority);
         ptr = ptr->next;
      }
      printf("(%d,%d,%d,%d) ",ptr->Ps.id,ptr->Ps.arrival,ptr->Ps.runtime,ptr->Ps.priority);
   }
	
   printf(" ]");
}
//this is for testing only
void main()
{
	struct CircularQueue*Q = initiate(Q);
	struct Process P;
	P.id = 1;
	P.arrival = 1;
	P.runtime = 4;
	P.priority = 10;
	CircularQueueInsert(Q,P);
	PrintCircularQueue(Q);
	struct Process P1;
	P1.id = 2;
	P1.arrival = 5;
	P1.runtime = 2;
	P1.priority = 8;
	CircularQueueInsert(Q,P1);
	PrintCircularQueue(Q);
	CircularQueueDeleteFirst(Q);
	PrintCircularQueue(Q);
}

