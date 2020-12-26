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