#include "headers.h"
#include "./Structs/structs.h"


/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

int createQueue(int key);
int receiveProcess(struct ProcessBuff * message);

int createShmem(int key);
int createSem(int key, union Semun * sem);
void up(int sem_id); //Not needed in the scheduler
void down(int sem_id);
int getRemainingTime();

void handleChildExit(int signum);
void clearResources(int signum);

int gen_q_id, shm_id, sem_id, *sched_shmaddr;

int main(int argc, char * argv[])
{
    initClk();
    signal(SIGINT, clearResources);
    signal(SIGUSR2, handleChildExit);
    struct ProcessBuff message;
    gen_q_id = createQueue(GENERATOR_Q_KEY);
    shm_id = createShmem(SCHEDULER_SHM_KEY);
    union Semun semun;
    sem_id = createSem(SEM_KEY, &semun);
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    while(1) //To Be MODIFIED.
    {
        if(receiveProcess(&message) != -1)
        {
            printf("message Received: id = %i, arrival = %i\n", message.content.id, message.content.arrival);
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
 * Return q_id on success and exit on failure.
*/
int createQueue(int key)
{
    printf("Schedular subscribing to the generator Q\n");
    int q_id = msgget(key, 0666 | IPC_CREAT);
    if(q_id == -1)
    {
        printf("failed to connect to the generator Q:(\n");
        exit(-1);
    }
    printf("Scheduler Subscribed to the generator Q, Q_id = %i\n", q_id);
    return q_id;
}

/*
 * Return -1 on failure, or no msg received.
 * Send the message to this function by reference.
 * NOTE: no wait is done here
*/
int receiveProcess(struct ProcessBuff * message)
{
    //printf("Scheduler is Receiving a process\n");
    return msgrcv(gen_q_id, message, sizeof(message->content), ALL, IPC_NOWAIT);
}

/*
 * The terminated child should be removed from the PCB.
*/
void handleChildExit(int signum)
{
    printf("Parent is notified of child exit.\n");
}

int createShmem(int key)
{
    int shmid = shmget(key, 4, IPC_CREAT | 0644);

    if (shmid == -1)
    {
        perror("Error in creating the shared memory @ Scheduler:(\n");
        exit(-1);
    }
    else
        printf("\nShared memory ID = %d\n", shmid);

    
    sched_shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if (sched_shmaddr == -1)
    {
        perror("Error in attach in Scheduler:(\n");
        exit(-1);
    }
    else
        printf("Scheduler: Shared memory attached at address %x\n", sched_shmaddr);
    return shm_id;
}

int createSem(int key, union Semun * sem)
{
    //1. Create Sems:
    int sem_id = semget(key, 1, 0666 | IPC_CREAT);

    if (sem_id == -1)
    {
        perror("Error in create the semaphor at scheduler:(\n");
        exit(-1);
    }

    sem->val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem_id, 0, SETVAL, *sem) == -1)
    {
        perror("Error in semctl: set value\n");
        exit(-1);
    }

    return sem_id;
}

void down(int sem_id)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem_id, &p_op, 1) == -1)
    {
        perror("Error in down() at Schedular:(\n");
        exit(-1);
    }
}

void up(int sem_id)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem_id, &v_op, 1) == -1)
    {
        perror("Error in up() at Scheduler:(\n");
        exit(-1);
    }
}

int getRemainingTime()
{
    printf("Getting rem time at Scheduler\n");
    down(sem_id);
    printf("Remaining time = %i", *sched_shmaddr);
    return *sched_shmaddr;
}


/*
 * Remove "Schedular Q"
 * Destroy clk
 * Terminate all ??
*/
void clearResources(int signum)
{
    printf("Clear Resources @ Scheduler\n");
    semctl(sem_id, 0, IPC_RMID, (struct semid_ds *)0);
    shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
    destroyClk(true);
    exit(0);
}
