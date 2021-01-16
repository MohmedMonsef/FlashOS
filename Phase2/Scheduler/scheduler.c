#include "headers.h"
#include <errno.h>

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};
void schedulerPerformance();
int createQueue(int key);
int receiveProcess(struct ProcessBuff *message);

int createShmem(int key);
int createSem(int key, union Semun *sem);
void up(int sem_id); //Not needed in the scheduler
void down(int sem_id);
int getRemainingTime();

void handleChildExit(int signum);
void clearResources(int signum);
void handleGenFinish(int signum);

void HPF();
void STN();

void logProcess(int id, char *status, int clk);
void writeInFile(char **params, int size);
void insertPCB(struct Process newProccess, int pid);
int runProcess(struct Process *curProccess);
void contextSwitching_STN();

int gen_q_id, shm_id, sched_sem_id, curProcessId, gen_sem_id;
int *sched_shmaddr;
int schedulerType, processCount;
bool processRunning = false, waitGen = true;
struct sembuf p_op;
struct PCB *pcb;
FILE *fLog;
FILE *perfLog;
struct ProcessBuff *receivedProcess;
union Semun semun;
bool interrupt_from_generator = false, interrupt_from_process = false;
int totalTimeForProcessesRunning = 0;
double TotalWTA = 0;
double totalWaiting = 0;
int AllProcesses = 0;
int main(int argc, char *argv[])
{
    key_t key_id;
    initClk();
    signal(SIGINT, clearResources);
    signal(SIGUSR2, handleChildExit);
    createMemory();

    struct ProcessBuff message;
    gen_q_id = createQueue(GENERATOR_Q_KEY);

    shm_id = createShmem(SCHEDULER_SHM_KEY);
    sched_sem_id = createSem(SCHED_SEM_KEY, &semun);
    union Semun gen_semun;
    schedulerType = atoi(argv[0]);
    processCount = atoi(argv[1]);
    AllProcesses = processCount;
    pcb = (struct PCB *)malloc(processCount * sizeof(struct PCB));

    if (schedulerType != 3)
    {
        createPriorityQueue(2 - schedulerType);
    }
    receivedProcess = (struct ProcessBuff *)malloc(sizeof(struct ProcessBuff));
    receivedProcess->header = 1;
    openMemoryLogFile();
    fLog = fopen("scheduler.log", "w");
    kill(getppid(), SIGUSR1);
    while (processCount)
    {
        if (schedulerType == 1)
        {
            HPF();
        }
        if (schedulerType == 2)
        {
            STN();
        }
    }
    fclose(fLog);
    closeMemoryLogFile();
    schedulerPerformance();
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
    int x = msgrcv(gen_q_id, message, sizeof(message->content), ALL, IPC_NOWAIT);
    if (message->content.id == -1)
    {
        waitGen = false;
        return -1;
    }
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
    pcb[curProcessId].process.runtime = 0;
    logProcess(curProcessId, "finished", getClk());
    processRunning = false;
    processCount--;
    deleteFromMemory(pcb[curProcessId].process, getClk());
    interrupt_from_process = true;
    if (processCount < 1)
    {
        schedulerPerformance();
        clearResources(0);
    }
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

    if (key != GEN_SEM_KEY)
    {
        sem->val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(sem_id, 0, SETVAL, *sem) == -1)
        {
            perror("Error in semctl: set value\n");
            exit(-1);
        }
    }
    return sem_id;
}

void down(int sem_id)
{

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = (!IPC_NOWAIT);

    if (semop(sem_id, &p_op, 1) == -1)
    {
        if (errno != EINTR)
        {
            perror("Error in down() at Schedular:(\n");
            if (sem_id == gen_sem_id)
                printf("Generator Baz");
            else
                printf("Scheduler Baz");

            exit(-1);
        }
        else
        {
            printf("Interrupted in down\n");
            down(sem_id);
        }
    }
    if (sem_id == gen_sem_id)
        printf("down generator\n");
    else
        printf("Down process\n");
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
    printf("Up scheduler\n");
}

/*
 * Remove "Schedular Q"
 * Destroy clk
 * Terminate all ??
*/
void clearResources(int signum)
{
    printf("Clear Resources @ Scheduler\n");
    semctl(sched_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
    semctl(gen_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
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
    pcb[id].totalRunTime = newProccess.runtime;
    pcb[id].lastStopped = newProccess.arrival;
    totalTimeForProcessesRunning += pcb[id].totalRunTime;
}
//TODO
//delete - should free the created process memory

//writing in files
void writeInFile(char **params, int size)
{
    char *firstStrings[9] = {"At time ", " process ", " ", " arr ", " total ", " remain ", " wait ", " TA ", " WTA "};
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
    sprintf(params[4], "%d", pcb[id].totalRunTime);
    sprintf(params[5], "%d", pcb[id].process.runtime);
    sprintf(params[6], "%d", pcb[id].wait);
    if (finished == 0)
    {
        int TA = clk - pcb[id].process.arrival;
        float WTA = (pcb[id].totalRunTime != 0) ? (float)TA / pcb[id].totalRunTime : 0;
        WTA = (int)(WTA * 100 + .5);
        WTA = (float)WTA / 100;
        WTA = floor(100 * WTA) / 100;
        pcb[id].WTA = WTA;
        TotalWTA += WTA;
        totalWaiting += pcb[id].wait;
        sprintf(params[7], "%d", TA);
        sprintf(params[8], "%g", WTA);
    }
    writeInFile(params, size);
}

void HPF()
{
    bool received = false;
    if (waitGen)
    {
        int rsv_value = receiveProcess(receivedProcess);
        while (rsv_value != -1)
        {
            received = true;
            pushProcess(receivedProcess->content);
            rsv_value = receiveProcess(receivedProcess);
        }
    }
    struct Process *receivedProcessSize = getFrontProcess();
    bool isProcessFitInMemory = true;
    if (receivedProcessSize)
    {
        isProcessFitInMemory = checkIfProcessFitInMemory(receivedProcessSize->memSize);
    }
    if (!processRunning && isProcessFitInMemory)
    {
        struct Process *curProccess = popProcess();
        if (curProccess)
        {
            int pid = runProcess(curProccess);
            processRunning = true;
            curProcessId = curProccess->id;
            pcb[curProcessId].wait = getClk() - pcb[curProcessId].process.arrival;
            pcb[curProcessId].pid = pid;
        }
    }
}

void STN()
{
    bool received = false;
    if (waitGen)
    {
        int rsv_value = receiveProcess(receivedProcess);
        while (rsv_value != -1)
        {
            received = true;
            pushProcess(receivedProcess->content);
            rsv_value = receiveProcess(receivedProcess);
        }
    }
    struct Process *receivedProcessSize = getFrontProcess();
    bool isProcessFitInMemory = true;
    if (receivedProcessSize)
    {
        isProcessFitInMemory = checkIfProcessFitInMemory(receivedProcessSize->memSize);
    }
    if (processRunning && received && isProcessFitInMemory) //&& (*sched_shmaddr > 1))
    {
        contextSwitching_STN();
    }
    if (!processRunning && isProcessFitInMemory)
    {
        struct Process *curProccess = popProcess();
        if (curProccess)
        {

            int pid = runProcess(curProccess);
            processRunning = true;
            curProcessId = curProccess->id;
            pcb[curProcessId].pid = pid;
        }
    }
}

void contextSwitching_STN()
{
    printf("Enter context switching\n");
    down(sched_sem_id);
    int curRemaining = *sched_shmaddr;
    up(sched_sem_id);
    struct Process *process = getFrontProcess();

    if (curRemaining > process->runtime)
    {
        kill(pcb[curProcessId].pid, SIGSTOP);
        process = popProcess();
        pcb[curProcessId].lastStopped = getClk();
        pcb[curProcessId].process.runtime = curRemaining;
        logProcess(curProcessId, "stopped", getClk());
        pushProcess(pcb[curProcessId].process);
        curProcessId = process->id;
        runProcess(process);
    }
}
int runProcess(struct Process *curProccess)
{
    int pid;
    if (pcb[curProccess->id].pid != -1)
    {
        pid = pcb[curProccess->id].pid;
        pcb[curProccess->id].wait += (getClk() - pcb[curProcessId].lastStopped);
        logProcess(curProccess->id, "resumed", getClk());
        kill(pid, SIGCONT);
    }
    else
    {
        pid = fork();
        if (pid == -1)
            perror("error in fork");
        else if (pid == 0)
        {
            char runtime[50];
            sprintf(runtime, "%d", curProccess->runtime);
            char *arg[] = {runtime, NULL};
            *sched_shmaddr = curProccess->runtime;
            execv("./process.out", arg);
        }
        else
        {
            insertInMemory(pcb[curProccess->id].process, getClk());
            pcb[curProccess->id].wait = (getClk() - pcb[curProccess->id].lastStopped);
            pcb[curProccess->id].pid = pid;
            logProcess(curProccess->id, "started", getClk());
        }
    }
    return pid;
}

void schedulerPerformance()
{

    int totalSchedulerRunningTime = getClk();
    double CPU_Utilization = (1.0 * totalTimeForProcessesRunning / totalSchedulerRunningTime) * 100.0;
    double AvgWTA = TotalWTA / AllProcesses;
    double AvgWaiting = totalWaiting / AllProcesses;

    double variation = 0;
    for (int i = 0; i < AllProcesses; i++)
    {
        variation += ((pcb[i].WTA - AvgWTA) * (pcb[i].WTA - AvgWTA));
    }
    double StdWTA = sqrt(variation);

    CPU_Utilization = (int)(CPU_Utilization * 100 + .5);
    CPU_Utilization = (float)CPU_Utilization / 100;
    CPU_Utilization = floor(100 * CPU_Utilization) / 100;

    AvgWTA = (int)(AvgWTA * 100 + .5);
    AvgWTA = (float)AvgWTA / 100;
    AvgWTA = floor(100 * AvgWTA) / 100;

    AvgWaiting = (int)(AvgWaiting * 100 + .5);
    AvgWaiting = (float)AvgWaiting / 100;
    AvgWaiting = floor(100 * AvgWaiting) / 100;

    StdWTA = (int)(StdWTA * 100 + .5);
    StdWTA = (float)StdWTA / 100;
    StdWTA = floor(100 * StdWTA) / 100;

    char util[50];
    char avgWTA[50];
    char avgWaiting[50];
    char std[50];
    sprintf(util, "%g", CPU_Utilization);
    sprintf(avgWTA, "%g", AvgWTA);
    sprintf(avgWaiting, "%g", AvgWaiting);
    sprintf(std, "%g", StdWTA);
    char *conc = (char *)malloc(2);
    conc = (char *)"%";
    strcat(util, conc);

    perfLog = fopen("performance.log", "w");
    char *firstStrings[4] = {"CPU utilization = ", "Avg WTA = ", "Avg Waiting = ", "Std WTA = "};

    for (int i = 0; i < 4; i++)
    {
        char *strOut = (char *)malloc(200 * sizeof(char));
        strcat(strOut, firstStrings[i]);
        if (i == 0)
            strcat(strOut, util);
        else if (i == 1)
            strcat(strOut, avgWTA);
        else if (i == 2)
            strcat(strOut, avgWaiting);
        else
            strcat(strOut, std);
        fprintf(perfLog, "%s \n", strOut);
    }
    fclose(perfLog);
}