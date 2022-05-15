#include <stdio.h>

#include "mpi.h"

int main(int argc, char **argv) {

	int i = 0;
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	double start, end, slot;

	int data_size = 100000;
	int *data = (int *)malloc(sizeof(int)*data_size);
	
	int *all = (int *)malloc(sizeof(int)*data_size*size);

	MPI_Barrier(MPI_COMM_WORLD);

	if(rank == 3)
	  sleep(10);
	
	start = MPI_Wtime();
	MPI_Allgather(data, data_size, MPI_INT, 
				all, data_size, MPI_INT, MPI_COMM_WORLD);

	end = MPI_Wtime();
	slot = end - start;

	double *all_time = (double *)malloc(sizeof(double)*size);
	MPI_Gather(&slot, 1, MPI_DOUBLE, all_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	int j = 0, k = 0;
	double total = 0;
	if(rank == 0) {
		for(i = 0; i < size; i++)
		{
			total += all_time[i];
			if (all_time[i] > all_time[j])
			  j = i;
			if(all_time[i] < all_time[k])
			  k = i;
		}
		printf("MIN: %f, \n", all_time[k]);
		printf("MAX: %f, \n", all_time[j]);
		printf("AVG: %f, \n", total/size);

		printf("\n");
	}

	MPI_Finalize();
	return 0;
}
