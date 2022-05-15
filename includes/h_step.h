#ifndef H_STEP_H

#define H_STEP_H

#include <unistd.h>

#include <math.h>
#include <time.h>

#include "h_common.h"
#include "h_unwind.h"
#include "h_basic.h"

// 0.function:
// description:	parse the input from command line
// input:		argc, argv
// output:		cmd_name, file_name
int h_mpi_parse(int argc, char **argv,
    int *tool_rank, int *tool_size,
    char *cmd_name, int *tool_info);

// 1. function: 0, 1
// description:	perform initialization to get all related
// 				information of the target processes.
// input: 		argc, argv
// output: 		tool_rank, tool_size, tool_info, cmd_name
int h_mpi_init(int argc, char **argv,
    int *tool_rank, int *tool_size,
    int *tool_info, char *cmd_name);

// 2. function:
// description:	back trace all target processes
// input:		tool_rank, tool_info, target_list
// output:		state
int h_backtrace_all(int tool_rank, int *tool_info,
    H_TARGET *target_list, short *state);

// 3. function:	2
// description:	check to see if a hang happens
// input:		tool_rank, tool_info, target_list
// output:		state
int h_check_all(int tool_rank, int *tool_info,
    H_TARGET *target_list, short *state);

// 4. function:
// description: report the root cause of a hang
// input: 		tool_rank, tool_info, target_list, state
// output:		state
int h_report(int tool_rank, int *tool_info,
    H_TARGET *target_list, short *state);

//5. function:
// description: report the root cause of a hang based on multiple 
//				times of h_backtrace_all calls
// input:       tool_rank, tool_info, target_list, state
// output:      state
int h_report_multi_backtrace(int tool_rank, int* tool_info,
    H_TARGET *target_list, short *state);

// 6. function:
// description:	get the root of the other communicator
// input:		tool_rank, tool_rank_new, tool_size_new,
//				this_comm
// output:		root_other_color
void h_get_root_other_color (int tool_rank,
    int tool_rank_new, int tool_size_new,
    MPI_Comm this_comm, int *root_other_color);

// 7. function:
// description: report the root cause of a hang
// input:       tool_rank, tool_info, target_list, state
// output:      state
float h_chi_squared_test(int tool_rank_new, int *model, int *backup_model[]);

// 8. function:
// description: check to see if a hang happens
// input:       tool_rank, color, tool_rank_new, tool_size_new,
//              local_rand_num, this_comm, local_rand_tasks, state
// output:
int h_check_rand_model(int tool_rank, int color,
    int *tool_info,
    int tool_rank_new, int tool_size_new,
    int *local_rand_num, MPI_Comm this_comm,
    H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2], short *state, int **simple_model);

// 9. function:
// description:	back trace random target processes
// input:		tool_rank_new, local_rand_num, local_rand_tasks
// output:		state
int h_backtrace_rand(int tool_rank_new, int local_rand_num,
    H_TARGET *local_rand_tasks, short *state);

#endif // H_STEP_H
