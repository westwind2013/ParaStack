/* C Example */
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>


int main (argc, argv)
		int argc;
		char *argv[];
{
	int rank, size;
	char name[30];
	int len;

	MPI_Init (&argc, &argv);	/* starts MPI */
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */

sleep(10);
	MPI_Get_processor_name(name, &len);
	if(rank == 2 )
	{	
		printf("%d\n", getpid());
		sleep(120);
	}
	printf( "(1) %f  Hello world from process %5d of @ node %s\n",MPI_Wtime(), rank, name);
	//sleep(100);
	
	MPI_Barrier(MPI_COMM_WORLD);	

	MPI_Finalize();
	return 0;
}

