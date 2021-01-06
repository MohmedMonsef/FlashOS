#include "headers.h"
#include "./Structs/structs.h"

int createQueue(int key);
int sendRemainingTime(struct RemainingTimeBuff * message, int q_id);

void stopHandler(int signum);
void clearResources(int signum);

int remaining_time, sched_q_id ;
struct RemainingTimeBuff msg;

int main(int agrc, char * argv[])
{
    initClk();
    signal(SIGUSR1, stopHandler);
    signal(SIGINT, clearResources);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    remaining_time=atoi(argv[1]);
    int prevClk = getClk();
    while (remaining_time > 0)
    {
        // remainingtime = ??;
        int nxtClk = getClk();
        if(nxtClk != prevClk){
            remaining_time--;
            prevClk = nxtClk;
        }

    }
    
    
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
 * Returns -1 on failure.
 * Send the message to this function by reference.
*/
int sendRemainingTime(struct RemainingTimeBuff * message, int q_id)
{
    return msgsnd(q_id, message, sizeof(message->content), !IPC_NOWAIT);
}

/*
 * Send the remaining time to the schedular.
 * Stop.
*/
void stopHandler(int signum)
{
    msg.content = remaining_time;
    sendRemainingTime(&msg, sched_q_id);
    raise(SIGSTOP);
}

/*
 * Send notification to the schedular Via SIGUSR2 signal.
 * Release the clock.
 * Exit.
*/
void clearResources(int signum)
{
    kill(getppid(), SIGUSR2);
    destroyClk(false);
    exit(0);
}
