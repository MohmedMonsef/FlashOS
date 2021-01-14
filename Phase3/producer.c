#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

int createShmem(int key);
int createSem(int key, union Semun *sem);
void up(int sem_id);
void down(int sem_id);
void clearResources(int signum);
/* Producer/consumer program illustrating conditional variables */
/*keys*/
#define SEM_FULL_KEY 114
#define SEM_EMPTY_KEY 115
#define SEM_MUTEX_KEY 202
#define SHM_KEY 404
/*ids*/
int shm_id, mutex_sem_id, full_sem_id, empty_sem_id;
union Semun sem_mutex_semun;
union Semun sem_full_semnum;
union Semun sem_empty_semun;
/* Size of shared buffer */
#define BUF_SIZE 3
struct Buffer 
{
    int buffer[BUF_SIZE];							        /* shared buffer */
    int next_add;										/* place to next_add next element */
    int next_rem;										/* place to remove next element */
};
struct Buffer *buf;
struct shmid_ds buff_status;


int main()
{
    signal(SIGINT, clearResources);
    /*create semaphore to know if buffer is empty*/
    empty_sem_id = createSem(SEM_EMPTY_KEY, &sem_empty_semun);
    /*create semaphore to know if buffer is full*/
    full_sem_id = createSem(SEM_FULL_KEY, &sem_full_semnum);
    /*create semaphore to the crtical section*/
    mutex_sem_id = createSem(SEM_MUTEX_KEY, &sem_mutex_semun);
    /*create shared memory*/
    shm_id = createShmem(SHM_KEY);
    int item = 1;
    while(1)
    {
        down(empty_sem_id);
        /*critical section*/
        down(mutex_sem_id);

        buf->buffer[buf->next_add] = item;
        printf("Item added = %i\n", item);
        item ++;
        buf->next_add = (buf->next_add + 1) % BUF_SIZE;

        up(mutex_sem_id);
        /*critical section*/
        up(full_sem_id);
        sleep(1);
    }
}

/*creating shared memory*/
int createShmem(int key)
{
    int shmid = shmget(key, sizeof(struct Buffer), IPC_CREAT | 0666);

    if (shmid == -1)
    {
        perror("Error in creating the shared memory @ Producer:(\n");
        exit(-1);
    }

    buf = shmat(shmid, (void *)0, 0);
    if (buf == (struct Buffer*)-1)
    {
        perror("Error in attach @ Producer :(\n");
        exit(-1);
    }
    /*critecal section*/
    down(mutex_sem_id);
    int status = shmctl(shmid, IPC_STAT, &buff_status);
    while (status == -1)
    {
        printf("Failed to ge status, try again\n");
        status = shmctl(shmid, IPC_STAT, &buff_status);
    }
    /*if this process is the first to attach then make it initialize the memory*/
    if(buff_status.shm_nattch == 1)
    {
        printf("Initialize the memory\n");
        buf->next_add=0;
        buf->next_rem=0;
    }
    up(mutex_sem_id);
    /*critecal section*/
    return shmid;
}

int createSem(int key, union Semun *sem)
{
    //1. Create Sems:
    int sem_id = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(sem_id == -1)//already exists, just get its id
    {
        printf("Just get the id...\n");
        sem_id = semget(key, 1, 0666 | IPC_CREAT);
        if (sem_id == -1)
        {
            perror("Error in create the semaphor @ Producer :(\n");
            exit(-1);
        }

    }
    else //Fist time to be created, set the values.
    {
        // initial value of the semaphore 
        switch (key)
        {
            case SEM_MUTEX_KEY: printf("sem = mutex"); sem->val = 1; break;
            case SEM_FULL_KEY : printf("sem = full");sem->val = 0; break;
            case SEM_EMPTY_KEY: printf("sem = empty");sem->val = BUF_SIZE; break;
            default: break;
        }
        printf("SEM VAL = %i\n",sem->val);

        if (semctl(sem_id, 0, SETVAL, *sem) == -1)
        {
            perror("Error in semctl: set value  @ Producer :(\n");
            exit(-1);
        }

    }
    
    //sleep(5);
    return sem_id;
}

void down(int sem_id)
{
    struct sembuf v_op;
    v_op.sem_num = 0;
    v_op.sem_op = -1;
    v_op.sem_flg = (!IPC_NOWAIT);

    if (semop(sem_id, &v_op, 1) == -1)
    {
        if (errno != EINTR)
        {
            perror("Error in down() @ Producer :(\n");
            exit(-1);
        }
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
        perror("Error in up() @ Producer :(\n");
        exit(-1);
    }
}

void clearResources(int signum)
{
    /*the last process running will clear all resources when interupted*/
    int status = shmctl(shm_id, IPC_STAT, & buff_status);
    while(status == -1)
        status = shmctl(shm_id, IPC_STAT, & buff_status);

    // If last process attached, release all resources:
    if(buff_status.shm_nattch < 2)
    {
        printf("Clear resources\n");
        semctl(mutex_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        semctl(full_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        semctl(empty_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
    }
    
    // Otherwise, just detach the memory:
    else
    {
        printf("Detach");
        shmdt(buf);
    }

    exit(0);
}
