#ifndef H_MAP_H

#define H_MAP_H

#include "common.h"

// 0. function:
// description:	correspond mpi rank to process id
// input:		target_num, rank_base
// output:		targetlist
void h_map(int target_num, int rank_base, H_TARGET *target_list);

// 1. function:
// description:	generate a constant number of random tasks
// input:		tool_rank, tool_size, tool_info
// output:		local_rand_num, local_rand_tasks
void h_generate_rand_tasks(int tool_rank, int tool_size, int *tool_info,
		int *local_rand_num, H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2]);

// 2. function:
// description:	correspond mpi rank to process id (random tasks)
// input:		tool_info, target_list, local_rand_num
// output:		local_rand_tasks
void h_map_rand(int *tool_info, H_TARGET *target_list,
		int *local_rand_num, H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2]);

// 3. function:
// description:	split communicator into two
// input:		tool_rank, local_rand_num, color, local_rand_tasks
// output:		new_comm, tool_rank_new, tool_size_new
void h_comm_split(int tool_rank, int *local_rand_num,
		int *color,
		MPI_Comm *new_comm, int *tool_rank_new, int *tool_size_new);

#endif /* H_COMMON_H */
