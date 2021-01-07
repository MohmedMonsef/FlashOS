struct Process
{
    int id;
    int arrival;
    int runtime;
    int priority;
};
struct Node
{
    struct Process process;
    struct Node *next;
};
struct LinkedList
{
    struct Node *head;
    struct LinkedList *nextList;
    struct Node *tail;
};
struct PriorityQueue
{
    struct LinkedList *front;
    struct LinkedList *tail;
    int type;
};
/*
*Messages structs:
*Note: A message should not contain pointers as it will cause segmentaiton fault. 
*/
struct ProcessBuff
{
    long header;
    struct Process content;
};
struct RemainingTimeBuff
{
    long header;
    int content;
};

struct PCB
{
    struct Process * process;
    int pid;
    int remainingTime;
    int wait;
    int lastStopped;
};