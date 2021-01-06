#include "headers.h"
#include "./Structs/structs.h"

void clearResources(int signum);
int createQueue(int key);
int sendProcess(struct ProcessBuff * message, int q_id);


int gen_q_id;

int main(int argc, char * argv[])
{
	signal(SIGINT, clearResources);
	gen_q_id = createQueue(GENERATOR_Q_KEY);
    struct ProcessBuff message;
	message.header = 1;
    // TODO Initialization
    // 1. Read the input files.(done)
    int n = 0;
    FILE *fs = fopen(argv[1],"r");
    if(!fs)
    {
    	printf("No such File with this Name");
    	return 0 ;
    }
    else
    {
    	char L[1000];
    	while(!feof(fs))
    	{
    		fscanf(fs,"%s",L);
    		if(L[0] == '#')
    		{
    			fscanf(fs,"%[^\n]",L);
    		} 
    		else
    		{
    			//sscanf(L,"%d",&Array[i]);
				n++;
    		}
    	}
    }
    int i = 0;
    int Array[n];
    fs = fopen(argv[1],"r");
    if(!fs)
    {
    	printf("No such File with this Name");
    	return 0 ;
    }
    else
    {
    	char L[1000];
    	while(!feof(fs))
    	{
    		fscanf(fs,"%s",L);
    		if(L[0] == '#')
    		{
    			fscanf(fs,"%[^\n]",L);
    		} 
    		else
    		{
    			sscanf(L,"%d",&Array[i]);
				i++;
    		}
    	}
    }
    // 5. Create a data structure for processes and provide it with its parameters.
    n = (i-1)/4;
    struct Process Processes[n];
    int index = 0;
    for(int j = 0 ; j < i-1 ; j+= 4)
    {
    	struct Process P;
    	P.id = Array[j];
    	P.arrival = Array[j+1];
    	P.runtime = Array[j+2];
    	P.priority = Array[j+3];
    	Processes[index] = P;
    	index++;
    } 
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.(done)
    int pid1,pid2, algo = 0;
    char* Parameter,*Algo;
    printf("What is the Scheduling Algorthim you want to work with\n");
    printf("Enter:\n");
    printf("1 for Non-Peremative Priority\n");
    printf("2 for Short job first\n");
    printf("3 for Round Robin\n");            
	scanf("%s",Algo);
	algo = atoi(Algo);
	while(algo < 1 || algo > 3)
	{
		printf("Please Enter a vaild Algorithm number: ");
		scanf("%s",Algo);
		algo = atoi(Algo);
	}
	if(algo == 3)
	{
		printf("Enter time slot for each process");
		scanf("%s",Parameter);
	}
    // 3. Initiate and create the scheduler and clock processes.
    pid1 = fork();
	if(pid1 == -1)
		perror("error in fork");
	else if(pid1 == 0)
	{
				// schedular
		char*a[] = { Algo ,Parameter , NULL };
		execv("./scheduler.out",a);
	}
	else
	{
		pid2 = fork();
		if(pid2 == -1)
			perror("error in fork");
		else if(pid2 == 0)
		{
			//clock
			char*a[] = { NULL };
			execv("./clk.out",a);
		}
		else
		{
			//generator		
			int index = 0 ;
			// 4. Use this function after creating the clock process to initialize clock
			initClk();
			// TODO Generation Main Loop
			int x ;
			while(index < n)
			{
				// To get time use this
				x = getClk();
				while(x >= Processes[index].arrival && index < n)
    			{
    			// 6. Send the information to the scheduler at the appropriate time.
					message.content = Processes[index];
					if(sendProcess(&message, gen_q_id) != -1)
    					index++;	
    			}	
			}
		}	
		}
    // 7. Clear clock resources
	// Sleep untill being interrupted or the Scheduler exits.
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
    if(q_id == -1)
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
int sendProcess(struct ProcessBuff * message, int q_id)
{
    int status = msgsnd(q_id, message, sizeof(message->content), !IPC_NOWAIT);
	if(status == -1)
		printf("Failed to send the process @ Generator:(\n");
	else
		printf("sent from Generator\n");
	
	return status;
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
