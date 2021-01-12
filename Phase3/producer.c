#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include<errno.h>
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

struct msgbuff
{
    long mtype;
    char mtext[10];
};
int createQueue(int key);
int createShmem(int key);
int createSem(int key, union Semun *sem);
void up(int sem_id); //Not needed in the scheduler
void down(int sem_id);
void clearResources(int signum);
/* Producer/consumer program illustrating conditional variables */
/*keys*/
#define Q_KEY 114
#define MUTEX_SEM_KEY 202
#define SHM_KEY 404
/*ids*/
int q_id, shm_id, mutex_sem_id;
union Semun mutex_semun;
/* Size of shared buffer */
#define BUF_SIZE 3
struct Buffer 
{
    int* buffer;							        /* shared buffer */
    int add;										/* place to add next element */
    int rem;										/* place to remove next element */
    int num;										/* number elements in buffer */
};
struct Buffer *buf;
int to_be_added=0;										/*next number to be added*/


int main()
{
    signal(SIGINT, clearResources);
    q_id = createQueue(Q_KEY);
    shm_id = createShmem(SHM_KEY);
    mutex_sem_id = createSem(MUTEX_SEM_KEY, &mutex_semun);
    int item;
    struct msgbuff message;
    while(1)
    {
        /*produce_item*/
        item = to_be_added;
        /*check if full*/
        if(buf->num == BUF_SIZE)
        {
            //wait for message
            printf("full \n");
    	    int rec_val = msgrcv(q_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
        }
        /*down_mutex_sem*/
        down(mutex_sem_id);
        /*add item to buffer*/
        printf("item produced : %d",item);
        to_be_added++;
        buf ->buffer[buf->add] = item;
        buf->add++;
        if(buf->num == 0)
        {
            //send message to say its not empty
        }
        buf->num++;
        /*up mutex sem*/
        up(mutex_sem_id);
    }
}


/* 
 * Return q_id on success and exit on failure.
*/
int createQueue(int key)
{
    int q_id = msgget(key, 0666 | IPC_CREAT);
    if (q_id == -1)
    {
        perror("Error in creating the msg queue:(\n");
        exit(-1);
    }
    return q_id;
}

int createShmem(int key)
{
    int shmid = shmget(key, 4, IPC_CREAT | 0644);

    if (shmid == -1)
    {
        perror("Error in creating the shared memory :(\n");
        exit(-1);
    }

    buf = shmat(shmid, (void *)0, 0);
    if (buf == (struct Buffer*)-1)
    {
        perror("Error in attach :(\n");
        exit(-1);
    }
    buf->buffer = malloc(sizeof(int)*BUF_SIZE);
    buf->num=0;
    buf->add=0;
    buf->rem=0;
    return shm_id;
}

int createSem(int key, union Semun *sem)
{
    //1. Create Sems:
    int sem_id = semget(key, 1, 0666 | IPC_CREAT);

    if (sem_id == -1)
    {
        perror("Error in create the semaphor :(\n");
        exit(-1);
    }
    /* initial value of the semaphore, Binary semaphore */
    sem->val = 1;
    if (semctl(sem_id, 0, SETVAL, *sem) == -1)
    {
        perror("Error in semctl: set value\n");
        exit(-1);
    }
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
            perror("Error in down() :(\n");
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
        perror("Error in up():(\n");
        exit(-1);
    }
}

void clearResources(int signum)
{
    semctl(mutex_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
    msgctl(q_id, IPC_RMID, (struct msqid_ds *)0);
    shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
    exit(0);
}
