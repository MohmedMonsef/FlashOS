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

struct msgbuff
{
    long mtype;
    char mtext;
};
int createQueue(int key);
int createSem(int key, union Semun *sem);
int createShmem(int key);
void up(int sem_id);
void down(int sem_id);
void clearResources(int signum);
/* Producer/consumer program illustrating conditional variables */
/*keys*/
#define PRODUCER_Q_KEY 114
#define CONSUMER_Q_KEY 115
#define MUTEX_SEM_KEY 202
#define SHM_KEY 404
/*ids*/
int producer_q_id, consumer_q_id, shm_id, mutex_sem_id;
union Semun mutex_semun;
/* Size of shared buffer */
#define BUF_SIZE 3
struct Buffer 
{
    int buffer[BUF_SIZE];							        /* shared buffer */
    int add;										/* place to add next element */
    int rem;										/* place to remove next element */
    int num;										/* number elements in buffer */
};
struct Buffer *buf;
struct shmid_ds buff_status;

int main()
{
    signal(SIGINT, clearResources);
    producer_q_id = createQueue(PRODUCER_Q_KEY);
    consumer_q_id = createQueue(CONSUMER_Q_KEY);
    mutex_sem_id = createSem(MUTEX_SEM_KEY, &mutex_semun);
    shm_id = createShmem(SHM_KEY);
    struct msgbuff send_message;
    send_message.mtype = 2;
    send_message.mtext = 'g';
    struct msgbuff rec_message;
    int item;
    char down_again = 0;

    while(1)
    {
        down(mutex_sem_id);



        if(buf->num < 0)
            clearResources(0);
        /*check if empty*/
        if(buf->num == 0)
        {
            up(mutex_sem_id);
            down_again = 1;
            printf("empty\n");
            //wait for message
    	    int rec_val = msgrcv(consumer_q_id, &rec_message, sizeof(rec_message.mtext), 0, !IPC_NOWAIT);
            if (rec_val == -1)
            {
                perror("Error in receive");
                exit(0);
            }
        }

        /*down_mutex_sem*/
        if(down_again == 1)
        {
            down(mutex_sem_id);
            down_again = 0;
        }

        int rec_val = msgrcv(consumer_q_id, &rec_message, sizeof(rec_message.mtext), 0, IPC_NOWAIT);
        while (rec_val != -1)
        {
            rec_val = msgrcv(consumer_q_id, &rec_message, sizeof(rec_message.mtext), 0, IPC_NOWAIT);
        }
        
        /*remove item from buffer*/
        buf->num --;
        if(buf->num < 0)
        {
            buf->num = 0;
            up(mutex_sem_id);
            continue;
        }
        printf("buf rem = %d\n", buf->rem);
        item = buf->buffer[buf->rem];
        printf("Item Consumed = %i\n", item);
        if(buf->rem == BUF_SIZE - 1)
            buf->rem = 0;
        else
            buf->rem++;
        
        printf("Num = %i\n", buf->num);
        if(buf->num == BUF_SIZE - 1)
        {
            printf("Waking the producer up\n");
            int status = msgsnd(producer_q_id, &send_message, sizeof(send_message.mtext), !IPC_NOWAIT);
            if (status == -1)
            {
                perror("Errror in send\n");
                exit(0);
            }
        }
        /*up mutex sem*/
        up(mutex_sem_id);
        //sleep(1);
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
    int shmid = shmget(key, sizeof(struct Buffer), IPC_CREAT | 0666);

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

    down(mutex_sem_id);
    int status = shmctl(shmid, IPC_STAT, & buff_status);
    while (status == -1)
        status = shmctl(shmid, IPC_STAT, & buff_status);

    if(buff_status.shm_nattch == 1)
    {
        printf("Initialize the memory\n");
        buf->num=0;
        buf->add=0;
        buf->rem=0;
    }
    up(mutex_sem_id);
    return shmid;
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
    printf("Down\n");
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
    printf("Up\n");
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
    int status = shmctl(shm_id, IPC_STAT, & buff_status);
    while(status == -1)
    {
        printf("HAHA\n");
        status = shmctl(shm_id, IPC_STAT, & buff_status);
    }
        
    printf("shm_nattch = %li\n", buff_status.shm_nattch);

    // If last process attached, release all resources:
    if(buff_status.shm_nattch < 2)
    {
        printf("Clear resources\n");
        semctl(mutex_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        msgctl(producer_q_id, IPC_RMID, (struct msqid_ds *)0);
        msgctl(consumer_q_id, IPC_RMID, (struct msqid_ds *)0);
        shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
    }

    // Otherwise, just detach the memory:
    else
    {
        printf("Detach\n");
        shmdt(buf);
    }

    exit(0);
}
