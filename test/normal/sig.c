#include <stdio.h>
#include <signal.h>
#include <time.h> 

#include "mpi.h"

void sigproc(void);
void quitproc(void); 

int times = 0;
int rank;

int main(int argc, char** argv)
{		

	int size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	struct sigaction h_action;
	h_action.sa_sigaction = (void *)sigproc;
	//sigemptyset(&h_action.sa_mask);
	sigfillset(&h_action.sa_mask);
	//sigdelset(&h_action.sa_mask, SIGUSR1);

	h_action.sa_flags = SA_SIGINFO;

	sigaction(SIGUSR1, &h_action, NULL);
	signal(SIGQUIT, (void *)quitproc);
	//printf("ctrl-c disabled use ctrl-\\ to quit\n");
	
	for(;;) {/* infinite loop */
		sleep(5);
		//printf("%d", rank);
	}
}

void sigproc()
{ 		 
	
	/* NOTE some versions of UNIX will reset signal to default
	   after each call. So for portability reset signal each time */
	printf("%d: %d\n", rank, times++);
	printf("you have pressed ctrl-c \n");
	MPI_Barrier(MPI_COMM_WORLD);
}

void quitproc()
{ 		 printf("ctrl-\\ pressed to quit\n");
	exit(0); /* normal exit status */
}
