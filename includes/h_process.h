#ifndef H_PROCESS_H

#define H_PROCESS_H

#include <unistd.h>

#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "h_common.h"
#include "h_basic.h"

// 0. function:
// description:	search for all targets processes
//				based on the command name
// input: 		cmd_line
// output:		target_list
int h_search(char *cmd_line, int task_num, H_TARGET *task_list);

// 1. function:
// description:	attach to a given process (main thread)
// input: 		target
// output:
int h_attach_process(H_TARGET *target);

// 2. function:
// description:	detatch to a given process (main thread)
// input: 		target
// output:
int h_detatch_process(H_TARGET target);

#endif /* H_PROCESS_H */
