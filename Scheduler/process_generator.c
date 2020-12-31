#include "headers.h"
#include "./Structs/structs.h"

void clearResources(int signum);
int createQueue(int key);
int sendProcess(struct ProcessBuff * message, int q_id);


int gen_q_id;

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    clearResources(0);
}

/*
 * Return q_id on success and -1 on failure.
*/
int createQueue(int key)
{
    return msgget(key, 0666 | IPC_CREAT);
}

/* 
 * Send the message to this function by reference.
 * Return -1 on failure.
*/
int sendProcess(struct ProcessBuff * message, int q_id)
{
    return msgsnd(q_id, message, sizeof(message->content), !IPC_NOWAIT);
}

/*
 * Clear all resources(Generator Q, Clk, Terminate all)
*/ 
void clearResources(int signum)
{
    msgctl(gen_q_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(signum);
}