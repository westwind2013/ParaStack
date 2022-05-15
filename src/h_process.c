#include "h_process.h"

// maximum length of bash line
#define BASH_LEN 150

// maximum length of process id
#define PROC_INFO_LEN 10

// maximum number of iterations to wait the tracee being stopped
#define MAX_ITERS 100

// milliseconds
#define SLEEP_TIME 10


int h_search(char *cmd_line, int task_num, H_TARGET *task_list) {

	// loop variable
	int i;

	// pointer for the pipe stream from a bash execution
	FILE *fp;

	char proc_info[PROC_INFO_LEN];

	// bash command
	char bash_line[BASH_LEN] = "/bin/ps --sort pid -o \"%p\" -C \"";
	memset(proc_info, 0, PROC_INFO_LEN);

	// assemble the full bash command
	strcat(bash_line, cmd_line);
	strcat(bash_line, "\"");

	// debug
	//printf("%s", bash_line);

	int time_out_search = 0;

	do {
		time_out_search ++;

		// execute bash command
		fp = popen(bash_line, "r");

		// debug 
		//system(bash_line);
		if (fp != NULL)
			break;
		else {
			h_msleep (1000);
		}

	} while (time_out_search <= 6000);

	if (fp == NULL) {
		printf("Failed to search the targets! \n");
		return ERROR;
	}

	// decode results and get all tasks in this job
	// skip the first line "PID"
	fgets(proc_info, PROC_INFO_LEN, fp);

	for (i = 0; i < task_num && fgets(proc_info, PROC_INFO_LEN, fp) != NULL; i++) {
		task_list[i].proc_id = atoi(proc_info);
	}

	pclose(fp);

	// double check
	//printf("check!\n");
	//system(bash_line);

	if (i != 0)
		return OK;
	else
		return ERROR;
}

int h_attach_process(H_TARGET *target) {

	int i = 0;

	// statement of variables recording status.
	long ret = 0;
	int wait_status;

	// attach to the target process of id proc_id;
	// the target process will stop after receiving 'SIGSTOP'.
	ret = ptrace(PTRACE_ATTACH, target->proc_id, NULL, NULL);
	if (ret != 0) {
		fprintf(stderr, "Error @ ptrace: %s\n\n", strerror(errno));
		return ERROR;
	}

	i = 0;
	while (i < MAX_ITERS) {
		// suspend the current process until a the target process has changed state.
		waitpid(target->proc_id, &wait_status, WUNTRACED);

		// returns true if the child process was stopped by delivery of signal.
		if (WIFSTOPPED(wait_status)) {

			return OK;
		}

		h_msleep(SLEEP_TIME);
		i++;
	}

	return ERROR;
}

int h_detatch_process(H_TARGET target) {

	int i = 0;

	int ret = 0;

	ret = ptrace(PTRACE_DETACH, target.proc_id, NULL, NULL);
	if (ret != 0) {
		fprintf(stderr, "Error @ ptrace: %s\n\n", strerror(errno));
		return ERROR;
	}

	return OK;
}



// debugging purpose
/*int main (int argc, char **argv) {
    
	H_TARGET h_target;

    char line_proc[400] = "ps -ef | grep \0";
    char tmp[40] = "\0";
    strcpy(tmp, argv[1]);
    strcat(line_proc, tmp);
    strcat(line_proc, " | awk \'{print $8\" \"$2}\' > 1\0");
    system(line_proc);
    printf("%s", line_proc);

    FILE *fp = fopen("1", "r");


    return 0;

    fflush(stdout);
    h_target.proc_id = atoi(argv[1]);
	printf("proc: %d\n", atoi(argv[1]));
	short tag = 1000;
	short prev;
	while (1) {
		prev = tag;
		h_attach_process(&h_target);
		h_backtrace(h_target, &tag);
		if (tag != prev)
			printf("%5d, %5d\n", prev, tag);
		h_detatch_process(h_target);
		h_msleep(1000);
	}
    
}
*/
