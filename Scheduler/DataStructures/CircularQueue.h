#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
// //Process
/*struct Process
{
     int id;
     int arrival;
     int runtime;
     int priority;
 };
//Node in CircularQueue
 struct Node {
    struct Process process;
    struct Node *next;
 };
//CircularQueue
struct CircularQueue
{
 	struct Node *head;
 	struct Node *last;
 };
 */
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
//this is for testing only
/*
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
	struct Node *N = CircularQueueDeleteFirst(Q);
	PrintCircularQueue(Q);
	CircularQueueInsert(Q,N->process);
	PrintCircularQueue(Q);
}
*/
