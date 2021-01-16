#include "headers.h"

/* arg for semctl system calls. */
union Semun
{
	int val;			   /* value for SETVAL */
	struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
	ushort *array;		   /* array for GETALL & SETALL */
	struct seminfo *__buf; /* buffer for IPC_INFO */
	void *__pad;
};

void clearResources(int signum);
void handleSchedulerWake(int signum);
int createQueue(int key);
int sendProcess(struct ProcessBuff *message, int q_id);

void createSem(int key, union Semun *sem);
void up();
void down();

int gen_q_id, gen_sem_id;
bool go = false;

int main(int argc, char *argv[])
{
	signal(SIGINT, clearResources);
	signal(SIGUSR1, handleSchedulerWake);
	gen_q_id = createQueue(GENERATOR_Q_KEY);
	struct ProcessBuff message;
	message.header = 1;
	union Semun semun;
	createSem(GEN_SEM_KEY, &semun);
	// 1. Read the input files.(done)
	int n = 0;
	FILE *fs = fopen(argv[1], "r");
	if (!fs)
	{
		printf("No such File with this Name");
		return 0;
	}
	else
	{
		char L[1000];
		while (!feof(fs))
		{
			fscanf(fs, "%s", L);
			if (L[0] == '#')
			{
				fscanf(fs, "%[^\n]", L);
			}
			else
			{
				n++;
			}
		}
	}
	int i = 0;
	int Array[n];
	fs = fopen(argv[1], "r");
	if (!fs)
	{
		printf("No such File with this Name");
		return 0;
	}
	else
	{
		char L[1000];
		while (!feof(fs))
		{
			fscanf(fs, "%s", L);
			if (L[0] == '#')
			{
				fscanf(fs, "%[^\n]", L);
			}
			else
			{
				sscanf(L, "%d", &Array[i]);
				i++;
			}
		}
	}
	// 5. Create a data structure for processes and provide it with its parameters.
	n = (i - 1) / 4;
	struct Process Processes[n];
	int index = 0;
	for (int j = 0; j < i - 1; j += 4)
	{
		struct Process P;
		P.id = Array[j];
		P.arrival = Array[j + 1];
		P.runtime = Array[j + 2];
		P.priority = Array[j + 3];
		Processes[index] = P;
		index++;
	}

	// 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.(done)
	int pid1, pid2, algo = 0;
	char *Parameter, *Algo;
	Algo = (char *)malloc(1 * sizeof(char *));
	Parameter = (char *)malloc(10 * sizeof(char *));
	printf("What is the Scheduling Algorthim you want to work with\n");
	printf("Enter:\n");
	printf("1 for Non-Peremative Priority\n");
	printf("2 for Short job first\n");
	printf("3 for Round Robin\n");
	scanf("%s", Algo);
	algo = atoi(Algo);
	while (algo < 1 || algo > 3)
	{
		printf("Please Enter a vaild Algorithm number: ");
		scanf("%s", Algo);
		algo = atoi(Algo);
	}
	if (algo == 3)
	{
		printf("Enter time slot for each process: ");
		scanf("%s", Parameter);
		int quantum = atoi(Parameter);
		while(quantum < 1)
		{
			printf("Please Enter a valid time slot for each process: ");
			scanf("%s", Parameter);
			quantum = atoi(Parameter);
		}
	}
	// 3. Initiate and create the scheduler and clock processes.
	pid1 = fork();
	if (pid1 == -1)
		perror("error in fork");
	else if (pid1 == 0)
	{
		// schedular
		char *pcbSize = (char *)malloc(100 * sizeof(char *));
		sprintf(pcbSize, "%d", n);
		char *a[] = {Algo, pcbSize, Parameter, NULL};
		execv("./scheduler.out", a);
	}
	else
	{
		pid2 = fork();
		if (pid2 == -1)
			perror("error in fork");
		else if (pid2 == 0)
		{
			//clock
			char *a[] = {NULL};
			execv("./clk.out", a);
		}
		else
		{
			//generator
			int index = 0;
			// 4. Use this function after creating the clock process to initialize clock
			initClk();
			// TODO Generation Main Loop
			int current_time, prev_time = -1, last_time = Processes[n - 1].arrival;
			bool allowed_up = false;
			while (!go)
				; // Wait the scheduler
			while (index < n)
			{
				// To get time use this
				current_time = getClk();
				while (current_time >= Processes[index].arrival && index < n)
				{
					// 6. Send the information to the scheduler at the appropriate time.
					printf("======%d : %d\n", current_time, Processes[index].id); //instead send
					struct ProcessBuff *newProcess = (struct ProcessBuff *)malloc(sizeof(struct ProcessBuff));
					struct Process p = {Processes[index].id - 1, Processes[index].arrival, Processes[index].runtime, Processes[index].priority};
					newProcess->header = 1;
					newProcess->content = p;
					if (sendProcess(newProcess, gen_q_id) != -1)
						index++;
					allowed_up = true;
				}
				if ((allowed_up || current_time > prev_time) && (current_time < last_time))
				{
					up();
					allowed_up = false;
					prev_time = current_time;
				}
			}
			//When all proceeses are sent, don't make the scheduler wait for you:
			kill(pid1, SIGUSR1);
		}
	}

	// 7. Clear clock resources
	// wait untill being interrupted or the Scheduler exits.
	int status;
	waitpid(pid1, &status, 0);
	clearResources(0);
}

/* 
 * Return q_id on success and exit on failure.
*/
int createQueue(int key)
{
	printf("Create the generator Q\n");
	int q_id = msgget(key, 0666 | IPC_CREAT);
	printf("generator subscribing to the generator Q,qid = %d\n", q_id);
	if (q_id == -1)
	{
		printf("failed to Create the generator Q:(\n");
		exit(-1);
	}
	printf("@gen: Q_id = %i\n", q_id);
	return q_id;
}

/* 
 * Send the message to this function by reference.
 * Return -1 on failure.
*/
int sendProcess(struct ProcessBuff *message, int q_id)
{
	int status = msgsnd(q_id, message, sizeof(message->content), !IPC_NOWAIT);
	if (status == -1)
		printf("Failed to send the process @ Generator:(\n");

	return status;
}

void createSem(int key, union Semun *sem)
{
	//1. Create Sems:
	gen_sem_id = semget(key, 1, 0666 | IPC_CREAT);

	if (gen_sem_id == -1)
	{
		perror("Error in create the semaphor at scheduler:(\n");
		exit(-1);
	}

	sem->val = 0; /* initial value of the semaphore, Binary semaphore */
	if (semctl(gen_sem_id, 0, SETVAL, *sem) == -1)
	{
		perror("Error in semctl: set value\n");
		exit(-1);
	}
}

void down()
{
	struct sembuf p_op;

	p_op.sem_num = 0;
	p_op.sem_op = -1;
	p_op.sem_flg = !IPC_NOWAIT;

	if (semop(gen_sem_id, &p_op, 1) == -1)
	{
		perror("Error in down() at Process:(\n");
		exit(-1);
	}
	printf("Wait Scheduler to wake\n");
}

void up()
{
	union Semun sem;
	sem.val = 1; /* initial value of the semaphore, Binary semaphore */
	if (semctl(gen_sem_id, 0, SETVAL, sem) == -1)
	{
		perror("Error in semctl: set value\n");
		exit(-1);
	}
	printf("up generator\n");
}

void handleSchedulerWake(int signum)
{
	go = true;
}

/*
 * Clear all resources(Generator Q, Clk, Terminate all)
*/
void clearResources(int signum)
{
	printf("Clear Resources @ Generator\n");
	msgctl(gen_q_id, IPC_RMID, (struct msqid_ds *)0);
	destroyClk(true);
	exit(0);
}
