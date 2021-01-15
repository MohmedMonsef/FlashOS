#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "../Structs/structs.h"
#include "./DataStructures/PriorityQueue.h"
#include <string.h>
#include "../Memory/memory.h"

typedef short BOOLEAN;
#define true 1
#define false 0

#define SHKEY 300
#define GENERATOR_Q_KEY 114
#define GEN_SEM_KEY 115
#define SCHEDULER_SHM_KEY 404
#define SCHED_SEM_KEY 405
#define ALL 0

///==============================
//don't mess with this variable//
int *shmaddr; //
int shmid;
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
        killpg(getpgrp(), SIGINT);
    }
}
