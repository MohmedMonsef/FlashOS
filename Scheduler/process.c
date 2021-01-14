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
void createSem(int key, union Semun *sem);
void up();
void down(); //Not needed in the process
void sendRemainingTime();

void contHandler(int signum);
void stopHandler(int signum);
void clearResources(int signum);

int remaining_time, shm_id, sem_id;
int *sched_shmaddr;
union Semun semun;
int prevClk, nxtClk;
int globalRemaining;
int main(int agrc, char *argv[])
{
    printf("New process forked\n");
    //kill(getppid(), SIGCONT);
    initClk();
    signal(SIGUSR1, stopHandler);
    signal(SIGINT, clearResources);
    signal(SIGCONT, contHandler);
    attachShm(SCHEDULER_SHM_KEY);

    createSem(SCHED_SEM_KEY, &semun);
    //TODO it needs to get the remaining time from somewhere
    remaining_time = atoi(argv[0]);
    prevClk = getClk();
    while (remaining_time > 0)
    {
        nxtClk = getClk();
        if (nxtClk != prevClk)
        {
            remaining_time--;
            *sched_shmaddr = remaining_time;

            if (remaining_time != 0)
            {
                globalRemaining = remaining_time;
                up();
            }

            printf("___Remaining time = %i\n", remaining_time);
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
    // else
    //     printf("\nShared memory ID = %d\n", shmid);

    sched_shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if (*sched_shmaddr == -1)
    {
        perror("Error in attach in Process:(\n");
        exit(-1);
    }
    // else
    //     printf("Process: Shared memory attached at address %ls\n", shmaddr);
    return shm_id;
}

void createSem(int key, union Semun *sem)
{
    //1. Create Sems:
    sem_id = semget(key, 1, 0666 | IPC_CREAT);

    if (sem_id == -1)
    {
        perror("Error in create the semaphor at scheduler:(\n");
        exit(-1);
    }
}

void down()
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem_id, &p_op, 1) == -1)
    {
        perror("Error in down() at Process:(\n");
        exit(-1);
    }
}

void up()
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem_id, &v_op, 1) == -1)
    {
        perror("Error in up() at Process:(\n");
        exit(-1);
    }
    printf("up process\n");
}

void sendRemainingTime()
{
    printf("Sending remaining time to Schedular\n");
    *sched_shmaddr = remaining_time;
    up();
}

/*
 * Send the remaining time to the schedular.
 * Stop.
*/
void stopHandler(int signum)
{
    //sendRemainingTime();
    //ToBeFixed : Remaining Time That The scheduler reads in real needs to be sent in SIGCONT
    remaining_time = globalRemaining;
    printf("Process go to sleep\n");
    raise(SIGSTOP);
}

void contHandler(int signum)
{
    prevClk = getClk();
    nxtClk = getClk();
    signal(SIGCONT, SIG_DFL);
    raise(SIGCONT);
}

/*
 * Detach the shared memory.
 * Send notification to the schedular Via SIGUSR2 signal.
 * Release the clock.
 * Exit.
*/
void clearResources(int signum)
{
    printf("Current process is exiting\n");
    shmdt(sched_shmaddr);
    destroyClk(false);
    kill(getppid(), SIGUSR2);
    printf("Notified Parent and exiting..\n");
    exit(0);
}
