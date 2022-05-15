#include <stdio.h>
#include <mpi.h>
#include <unistd.h>

int main (argc, argv)
		int argc;
		char *argv[];
{
	int rank, size;

	MPI_Init (&argc, &argv);	/* starts MPI */
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */
	//printf( "Hello world from process %d of %d\n", rank, size );
	if (rank % 32 == 0)
	  printf("%d:%d\n", rank, getpid());

	int i = 0;
	for (i = 0; i < 60; i++) {
		//system("pkill -1 stack");
		MPI_Barrier(MPI_COMM_WORLD);
		if (rank == 0)
		  system("pkill -SIGUSR1 -x stack");
		sleep(1);
		MPI_Barrier(MPI_COMM_WORLD);
		if (rank == 0)
		  system("pkill -SIGUSR1 -x stack");
	}	

	MPI_Finalize();
	return 0;
}
