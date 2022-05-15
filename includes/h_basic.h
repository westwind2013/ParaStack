#ifndef H_BASIC_H

#define H_BASIC_H

#include <unistd.h>

#include "h_common.h"

// 0. function:
// description:	sleep for milliseconds
// input: 		msecs
// output:
void h_msleep(unsigned int msecs);

// 1. function:
// description:	check whether a string contains keyword "MPI"
// input:		frame_name
// output:
int h_check_string(char *frame_name);

// 2. function:
// description:	generate a list of random numbers with no repetitions
// input:		max_rand, num
// output:		rand_list
void h_generate_rand_list(int max_rand, int num, int *rand_list);

// 3. function:
// description:	bubble sort
// input:		rand_list, num
// output:		rand_list
void h_bubble_sort(int *rand_list, int num);

// 4. function:
// description:	check arrays (int)
// input:		tool_rank, tool_size, array, num
// output:
void h_check(int tool_rank, int *array, int num);

// 5. function:
// description:	check monitored targetss (int)
// input:		tool_rank, tool_size, array, num
// output:
void h_check_target(int tool_rank, H_TARGET *array, int num);

// 6. function:
// description:	claim this tool starts
// input:		tool_rank
// output:
void h_claim_start (int tool_rank);

// 7. function:
// description:	claim this tool exits
// input:		tool_rank, check_ret
// output:
void h_claim_exit (int tool_rank, int check_ret);

// 8. function:
// description:	claim the locations of randomly selected
//				target tasks
// input:		tool_rank, tool_size, tool_info,
// 				local_rand_num, local_rand_tasks
// output:
void h_claim_rand_info (int tool_rank, int tool_size,
		int *tool_info, int *local_rand_num,
		H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2]);

#endif  // H_BASIC_H
