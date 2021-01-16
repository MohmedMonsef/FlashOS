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
void handleGenFinish(int signum);

void HPF();
void STN();
void RR();

void logProcess(int id, char *status, int clk);
void writeInFile(char **params, int size);
void insertPCB(struct Process newProccess, int pid);
int runProcess(struct Process *curProccess);
void contextSwitching_STN();
void contextSwitching_RR();

int gen_q_id, shm_id, sched_sem_id, curProcessId, gen_sem_id, quantum, counter = 0, prev_time=-1;
int *sched_shmaddr;
int schedulerType, processCount;
bool processRunning = false, waitGen = true;
struct sembuf p_op;
struct PCB *pcb;
FILE *fLog;
struct ProcessBuff *receivedProcess;
union Semun semun;
bool interrupt_from_generator = false, interrupt_from_process = false;
struct CircularQueue*Q;

int main(int argc, char *argv[])
{
    key_t key_id;
    initClk();
    signal(SIGINT, clearResources);
    signal(SIGUSR2, handleChildExit);
    signal(SIGUSR1, handleGenFinish);

    struct ProcessBuff message;
    gen_q_id = createQueue(GENERATOR_Q_KEY);

    shm_id = createShmem(SCHEDULER_SHM_KEY);
    sched_sem_id = createSem(SCHED_SEM_KEY, &semun);
    union Semun gen_semun;
    gen_sem_id = createSem(GEN_SEM_KEY, &gen_semun);

    schedulerType = atoi(argv[0]);
    if(schedulerType == 3)
    {
        quantum = atoi(argv[2]);
        Q = initiate(Q);
    }
    else
    {
        createPriorityQueue(2 - schedulerType);
    }
        
    processCount = atoi(argv[1]);
    pcb = (struct PCB *)malloc(processCount * sizeof(struct PCB));

    receivedProcess = (struct ProcessBuff *)malloc(sizeof(struct ProcessBuff));
    receivedProcess->header = 1;
    fLog = fopen("scheduler.log", "w");
    kill(getppid(), SIGUSR1);
    while (processCount)
    {
        if (waitGen)
        {
            printf("Try down gen\n");
            down(gen_sem_id);
        }
        if (processRunning)
        {
            printf("Try down process\n");
            down(sched_sem_id);
        }

        if (schedulerType == 1)
        {
            HPF();
        }
        else if (schedulerType == 2)
        {
            STN();
        }
        else if (schedulerType == 3)
        {
            while(getClk() - prev_time < 1)
                ;
            printf("processes: %d\n",processCount);
            RR();
            prev_time = getClk();
        }
        printf("processes: %d\n",processCount);
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
    if(schedulerType == 3)
    {
        CircularQueueDeleteFirst(Q);
        PrintCircularQueue(Q);
    }
    pcb[curProcessId].process.runtime = 0;
    logProcess(curProcessId, "finished", getClk());
    processRunning = false;
    //p_op.sem_flg = (IPC_NOWAIT);
    processCount--;
    interrupt_from_process = true;
    up(sched_sem_id);
    if (processCount < 1)
        clearResources(0);
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
        sem->val = 0; /* initial value of the semaphore, Binary semaphore */
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
    printf("inside the down\n");
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
            if(sem_id == gen_sem_id && interrupt_from_process)
                down(sched_sem_id);
            interrupt_from_process = false;
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
    //id += 1;
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
    printf("log id= %d , w= %d \n", id, pcb[id].wait);
    if (finished == 0)
    {
        int TA = clk - pcb[id].process.arrival;
        float WTA = (pcb[id].totalRunTime != 0) ? (float)TA / pcb[id].totalRunTime : 0;
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
    while (rsv_value != -1)
    {
        printf("Received id = %i\n", receivedProcess->content.id + 1);
        //logProcess(receivedProcess->content.id, "arrived", getClk());
        pushProcess(receivedProcess->content);
        rsv_value = receiveProcess(receivedProcess);
    }
    if (!processRunning)
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
    printf("Enter STN\n");

    // if(processRunning)
    //     kill(pcb[curProcessId].pid, SIGUSR1);

    int rsv_value = receiveProcess(receivedProcess);
    bool received = false;
    while (rsv_value != -1)
    {
        received = true;
        printf("############Received id = %i\n", receivedProcess->content.id + 1);
        //logProcess(receivedProcess->content.id, "arrived", getClk());
        pushProcess(receivedProcess->content);
        rsv_value = receiveProcess(receivedProcess);
    }
    if (processRunning && received) //&& (*sched_shmaddr > 1))
    {
        contextSwitching_STN();
    }
    if (!processRunning)
    {
        printf("Enter Process not running so add new one.\n");
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
    /*
    printf("Stop the current process to compare.\n");
    int what = kill(pcb[curProcessId].pid, SIGUSR1);
    printf("the sent kill = %i\n", what);
    int curRemaining = getRemainingTime();
    */
    int curRemaining = *sched_shmaddr;
    printf("curRemaining %d\n", curRemaining);
    struct Process *process = getFrontProcess();

    if (curRemaining > process->runtime)
    {
        //switch
        printf("Switch\n");

        kill(pcb[curProcessId].pid, SIGUSR1);
        
        process = popProcess();
        pcb[curProcessId].lastStopped = getClk();
        pcb[curProcessId].process.runtime = curRemaining;
        logProcess(curProcessId, "stopped", getClk());
        pushProcess(pcb[curProcessId].process);
        curProcessId = process->id;
        runProcess(process);
    }
    // else
    //     kill(pcb[curProcessId].pid, SIGCONT);
    
}
int runProcess(struct Process *curProccess)
{
    int pid;

    if (pcb[curProccess->id].pid != -1)
    {
        pid = pcb[curProccess->id].pid;
        pcb[curProccess->id].wait += (getClk() - pcb[curProcessId].lastStopped);
        printf("resumed time %d\n", pcb[curProccess->id].process.runtime);
        logProcess(curProccess->id, "resumed", getClk());
        *sched_shmaddr = pcb[curProccess->id].process.runtime;
        kill(pid, SIGCONT);
    }
    else
    {
        pid = fork();
        if (pid == -1)
            perror("error in fork");
        else if (pid == 0)
        { //sleep(1);
            char runtime[50];
            sprintf(runtime, "%d", curProccess->runtime);
            char *arg[] = {runtime, NULL};
            *sched_shmaddr = curProccess->runtime;
            execv("./process.out", arg);
        }
        else
        {
            pcb[curProccess->id].pid = pid;
            printf("cur wait id = %d  w= %d ls = %d\n", curProccess->id, pcb[curProccess->id].wait, pcb[curProccess->id].lastStopped);
            pcb[curProccess->id].wait = (getClk() - pcb[curProccess->id].lastStopped);
            logProcess(curProccess->id, "started", getClk());
        }
    }
    printf("New Process entered, id = %i\n", curProccess->id + 1);
    return pid;
}


void RR()
{
    printf("Enter RR\n");
    counter++;
    int rsv_value = receiveProcess(receivedProcess);
    bool received = false;
    while (rsv_value != -1)
    {
        received = true;
        printf("############Received id = %i\n", receivedProcess->content.id + 1);
        logProcess(receivedProcess->content.id, "arrived", getClk());
        CircularQueueInsert(Q,receivedProcess->content);
        rsv_value = receiveProcess(receivedProcess);
    }
    if (counter % quantum == 0 && processRunning && *shmaddr != 0)
    {
        contextSwitching_RR();
        counter = 0;
    }   
    if (!processRunning)
    {
        printf("Enter Process not running so add new one.\n");
        struct Process *curProccess = &(Q->head->process);
        if (curProccess)
        {
            //Q->head = Q->head->next;
            printf("Run new processs\n");
            int pid = runProcess(curProccess);
            processRunning = true;
            curProcessId = curProccess->id;
            pcb[curProcessId].pid = pid;
            counter = 0;
        }
    }    
printf("black hole\n");
}
void contextSwitching_RR()
{
    printf("Enter context switching\n");
    int curRemaining = *sched_shmaddr;
    printf("curRemaining %d\n", curRemaining);
    //Q->head = Q->head->next;
    //Q->last = Q->last->next;
    struct Node* N = CircularQueueDeleteFirst(Q);
    CircularQueueInsert(Q,N->process);
    struct Process *process =&(Q->head->process);
    //switch
    printf("Switch\n");
    kill(pcb[curProcessId].pid, SIGSTOP);
    printf("processStpped\n");   
    pcb[curProcessId].lastStopped = getClk();
    pcb[curProcessId].process.runtime =curRemaining;
    logProcess(curProcessId, "stopped", getClk());
    curProcessId = process->id;
    runProcess(process);
}


void handleGenFinish(int signum)
{
    interrupt_from_generator = true;
    up(gen_sem_id);
    waitGen = false;
    printf("wait Gen = false.\n");
}
