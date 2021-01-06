#include "headers.h"
#include "./Structs/structs.h"
#include "./DataStructures/PriorityQueue.h"

int createQueue(int key);
int receiveProcess(struct ProcessBuff * message);
void handleChildExit(int signum);
void clearResources(int signum);
void HPF();

int gen_q_id, sched_q_id;
int schedulerType;
bool processRunning=false;
int main(int argc, char * argv[])
{
    key_t key_id;
    initClk();
    signal(SIGINT, clearResources);
    signal(SIGUSR2, handleChildExit);
    //TODO implement the scheduler :)
    schedulerType = atoi(argv[1]);

    if(schedulerType != 3){
        createPriorityQueue(2-schedulerType);
    }
    //upon termination release the clock resources.
    key_id = ftok("keyFile", SCHEDULAR_Q_KEY);
    int key = msgget(key_id, 0666 | IPC_CREAT);
    createQueue(key);
    struct ProcessBuff* receivedProcess;
    while(1){
        if(schedulerType == 1)
        {
            HPF(receivedProcess);
        }
    }
    //clearResources(0); I commented this cause it terminates the program as schedular not built yet

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
    processRunning=false;
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
    exit(0);
}

void HPF(struct ProcessBuff* receivedProcess){
    receivedProcess=NULL;
    receiveProcess(receivedProcess);
    if(receivedProcess){
        pushProcess(receivedProcess->content);
    }
    if(!processRunning){
        struct Process *curProccess = popProcess();
        if(curProccess){
            int pid = fork();
            if(pid == -1)
                perror("error in fork");
            else if(pid == 0)
            {
                char*arg[] = { curProccess->runtime , NULL };
                execv("./process.out",arg);
            }
        }
    }
}