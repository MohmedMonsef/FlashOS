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

int createQueue(int key);
int receiveProcess(struct ProcessBuff *message);
int createShmem(int key);
int createSem(int key, union Semun *sem);
void up(int sem_id); //Not needed in the scheduler
void down(int sem_id);
int getRemainingTime();
void handleChildExit(int signum);
void clearResources(int signum);
void HPF();
void logProcess(int id, char *status, int clk);
void writeInFile(char **params, int size);
void insertPCB(struct Process newProccess, int pid);

int gen_q_id, shm_id, sem_id, curProcessId;
int *sched_shmaddr;
int schedulerType, processCount;
bool processRunning = false;
struct PCB *pcb;
FILE *fLog;
struct ProcessBuff *receivedProcess;
int main(int argc, char *argv[])
{
    key_t key_id;
    initClk();
    signal(SIGINT, clearResources);
    signal(SIGUSR2, handleChildExit);
    struct ProcessBuff message;
    gen_q_id = createQueue(GENERATOR_Q_KEY);
    shm_id = createShmem(SCHEDULER_SHM_KEY);
    union Semun semun;
    sem_id = createSem(SEM_KEY, &semun);
    schedulerType = atoi(argv[0]);
    processCount = atoi(argv[1]);
    pcb = (struct PCB *)malloc(processCount * sizeof(struct PCB));

    if (schedulerType != 3)
    {
        createPriorityQueue(2 - schedulerType);
    }
    receivedProcess = (struct ProcessBuff *)malloc(sizeof(struct ProcessBuff));
    receivedProcess->header = 1;
    fLog = fopen("scheduler.log", "w");
    while (processCount)
    {
        if (schedulerType == 1)
        {
            HPF(receivedProcess);
        }
    }
    fclose(fLog);
    //upon termination release the clock resources.
    clearResources(0);
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
    int q_id = msgget(key, 0666 | IPC_CREAT);
    printf("Schedular subscribing to the generator Q,qid = %d\n", q_id);
    if (q_id == -1)
    {
        printf("failed to connect to the generator Q:(\n");
        exit(-1);
    }
    return q_id;
}

/*
 * Return -1 on failure, or no msg received.
 * Send the message to this function by reference.
 * NOTE: no wait is done here
*/
int receiveProcess(struct ProcessBuff *message)
{
    //printf("Scheduler is Receiving a process\n");
    int x = msgrcv(gen_q_id, message, sizeof(message->content), ALL, IPC_NOWAIT);
    if (x != -1)
    {
        insertPCB(message->content, -1);
    }

    return x;
}

/*
 * The terminated child should be removed from the PCB.
*/
void handleChildExit(int signum)
{
    printf("Parent is notified of child exit.\n");
    pcb[curProcessId].remainingTime = 0;
    logProcess(curProcessId, "finished", getClk());
    processRunning = false;
    processCount--;
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
    if (*sched_shmaddr == -1)
    {
        perror("Error in attach in Scheduler:(\n");
        exit(-1);
    }
    else
        printf("Scheduler: Shared memory attached at address %ls\n", sched_shmaddr);
    return shm_id;
}

int createSem(int key, union Semun *sem)
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

void writer(int newMem)
{
    *sched_shmaddr = newMem;
}

int reader()
{
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
//pcb functions
//insert
void insertPCB(struct Process newProccess, int pid)
{
    int id = newProccess.id;
    pcb[id].process = newProccess;
    pcb[id].pid = pid;
    pcb[id].remainingTime = newProccess.runtime;
}
//TODO
//delete - should free the created process memory

//writing in files
void writeInFile(char **params, int size)
{
    char *firstStrings[9] = {"At time  ", "  process  ", " ", "  arr  ", "  total  ", "  remain  ", "  wait  ", " ta ", "  wta  "};
    char *strOut = (char *)malloc(500);
    for (int i = 0; i < size; i++)
    {
        strcat(strOut, firstStrings[i]);
        strcat(strOut, params[i]);
    }
    fprintf(fLog, "%s\n", strOut);
}

//calc params for log file and write in it

void logProcess(int id, char *status, int clk)
{
    bool finished = strcmp(status, "finished");
    int size = finished == 0 ? 9 : 7;
    char *params[9];
    for (int i = 0; i < 9; i++)
    {
        params[i] = (char *)malloc(50 * sizeof(char));
    }
    sprintf(params[0], "%d", clk);
    sprintf(params[1], "%d", id);
    params[2] = status;
    sprintf(params[3], "%d", pcb[id].process.arrival);
    sprintf(params[4], "%d", pcb[id].process.runtime);
    sprintf(params[5], "%d", pcb[id].remainingTime);
    sprintf(params[6], "%d", pcb[id].wait);
    if (finished == 0)
    {
        int TA = clk - pcb[id].process.arrival;
        float WTA = (float)TA / pcb[id].process.runtime;
        WTA = (int)(WTA * 100 + .5);
        WTA = (float)WTA / 100;
        sprintf(params[7], "%d", TA);
        sprintf(params[8], "%f", WTA);
    }
    writeInFile(params, size);
}
void HPF()
{
    int rsv_value = receiveProcess(receivedProcess);

    if (rsv_value != -1)
    {
        logProcess(receivedProcess->content.id, "arrived", getClk());
        pushProcess(receivedProcess->content);
    }
    if (!processRunning)
    {
        struct Process *curProccess = popProcess();
        if (curProccess)
        {
            int pid = fork();
            if (pid == -1)
                perror("error in fork");
            else if (pid == 0)
            {
                char runtime[50];
                sprintf(runtime, "%d", curProccess->runtime);
                char *arg[] = {runtime, NULL};
                execv("./process.out", arg);
            }
            else
            {
                processRunning = true;
                curProcessId = curProccess->id;
                pcb[curProcessId].wait = getClk() - curProccess->arrival;
                pcb[curProcessId].pid = pid;
                logProcess(curProcessId, "started", getClk());
            }
        }
    }
}
