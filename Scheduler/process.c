#include "headers.h"

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

int attachShm(int key);
int createSem(int key, union Semun * sem);
void up(int sem_id);
void down(int sem_id); //Not needed in the process
void sendRemainingTime();

void stopHandler(int signum);
void clearResources(int signum);

int remaining_time, shm_id, sem_id;
int *sched_shmaddr;

int main(int agrc, char * argv[])
{
    initClk();
    signal(SIGUSR1, stopHandler);
    signal(SIGINT, clearResources);
    attachShm(SCHEDULER_SHM_KEY);
    union Semun semun;
    createSem(SEM_KEY, &semun);
    //TODO it needs to get the remaining time from somewhere
    remaining_time=atoi(argv[0]);
    int prevClk = getClk();
    while (remaining_time > 0)
    {
        int nxtClk = getClk();
        if(nxtClk != prevClk){
            remaining_time--;
            prevClk = nxtClk;
        }

    }
    
    
    clearResources(0);
}


int attachShm(int key)
{
    int shmid = shmget(key, 4, IPC_CREAT | 0644);

    if (shmid == -1)
    {
        perror("Error in creating the shared memory @ Process:(\n");
        exit(-1);
    }
    else
        printf("\nShared memory ID = %d\n", shmid);

    
    sched_shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if (*sched_shmaddr == -1)
    {
        perror("Error in attach in Process:(\n");
        exit(-1);
    }
    else
        printf("Process: Shared memory attached at address %ls\n", sched_shmaddr);
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

void sendRemainingTime()
{
    printf("Sending remaining time to Schedular\n");
    up(sem_id);
    *sched_shmaddr = remaining_time;
}

/*
 * Send the remaining time to the schedular.
 * Stop.
*/
void stopHandler(int signum)
{
    sendRemainingTime();
    printf("Process go to sleep\n");
    raise(SIGSTOP);
}


/*
 * Detach the shared memory.
 * Send notification to the schedular Via SIGUSR2 signal.
 * Release the clock.
 * Exit.
*/
void clearResources(int signum)
{
    shmdt(sched_shmaddr);
    kill(getppid(), SIGUSR2);
    destroyClk(false);
    exit(0);
}
