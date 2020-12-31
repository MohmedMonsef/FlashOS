#include "headers.h"
#include "structs.h"

int createQueue(int key);
int receiveProcess(struct ProcessBuff * message);
void handleChildExit(int signum);
void clearResources(int signum);

int gen_q_id, sched_q_id;

int main(int argc, char * argv[])
{
    initClk();
    signal(SIGINT, clearResources);
    signal(SIGUSR2, handleChildExit);
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    
    clearResources(0);
}



/*
 * IMPORTANT NOTES:
 * The schedular should send the run time to the process when forking.
 * When the schedular wants to stop a process, it send a SIGUSR1 signal to it.
*/


/* 
 * Return q_id on success and -1 on failure.
*/
int createQueue(int key)
{
    return msgget(key, 0666 | IPC_CREAT);
}

/*
 * Return -1 on failure, or no msg received.
 * Send the message to this function by reference.
 * NOTE: no wait is done here
*/
int receiveProcess(struct ProcessBuff * message)
{
    return msgrcv(gen_q_id, message, sizeof(message->content), ALL, IPC_NOWAIT);
}

/*
 * The terminated child should be removed from the PCB.
*/
void handleChildExit(int signum)
{
    printf("Parent is notified of child exit.\n");
}

/*
 * Remove "Schedular Q"
 * Destroy clk
 * Terminate all ??
*/
void clearResources(int signum)
{
    msgctl(sched_q_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(signum);
}
