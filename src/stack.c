#include "h_common.h"
#include "h_step.h"

#ifdef PHASE_DETECTION
// the default phase id is 0
const int sig[MAX_PHASE_NUM-1] = {SIGUSR1, SIGUSR2, SIGHUP, SIGINT, SIGQUIT, SIGFPE};
int phase_times[MAX_PHASE_NUM-1] = {0, 0, 0, 0, 0, 0}; 
#endif

// the phase id by default is 0 
int phase_id = 0;


#ifdef PHASE_DETECTION
void h_sig_handler_paired_marks(int sig) {

	switch(sig){
		case SIGUSR1:
			phase_times[0]++;
			if (phase_times[0] % 2 == 1)
				phase_id = 1;
			else
				phase_id = 0;
			break;

		case SIGUSR2:
			phase_times[1]++;
			if (phase_times[1] % 2 == 1)
				phase_id = 2;
			else
				phase_id = 0;
			break;

		case SIGHUP:
			phase_times[2]++;
			if (phase_times[2] % 2 == 1)
				phase_id = 3;
			else
				phase_id = 0;
			break;

		case SIGINT:
			phase_times[3]++;
			if (phase_times[3] % 2 == 1)
				phase_id = 4;
			else
				phase_id = 0;
			break;

		case SIGQUIT:
			phase_times[4]++;
			if (phase_times[4] % 2 == 1)
				phase_id = 5;
			else
				phase_id = 0;
			break;

		case SIGFPE:
			phase_times[5]++;
			if (phase_times[5] % 2 == 1)
				phase_id = 6;
			else
				phase_id = 0;
			break;
	}
}

void h_sig_handler_single_mark(int sig) {

	switch(sig){
		case SIGUSR1:
			phase_id = 1;
			break;

		case SIGUSR2:
			phase_id = 2;
			break;

		case SIGHUP:
			phase_id = 3;
			break;

		case SIGINT:
			phase_id = 4;
			break;

		case SIGQUIT:
			phase_id = 5;
			break;

		case SIGFPE:
			phase_id = 6;
			break;
	}
}
#endif


int main(int argc, char **argv) {

	/////////////////////////STEP 1: Initialization//////////////////////

	// index variable
	int i = 0;

	// tool_info[0], responsible number of targets for each task of tool
	// tool_info[1], rank_base, the lowest rank of all targets on a node
	// tool_info[2], total number of targets across all nodes
	int tool_info[3] = { -1, -1, -1 };

	// command name
	char cmd_name[CMD_NAME_LEN] = "\0";

	// contains all the targets need to be traced
	H_TARGET *target_list = NULL;

	// record the state of each target
	short *state = NULL;

	// the rank of tool process
	int tool_rank;

	// the size of the distributed tool
	int tool_size;

	// parse parameters & MPI initialization
	if (h_mpi_init(argc, argv, &tool_rank, &tool_size, tool_info,
				cmd_name) == ERROR)
		return ERROR;

	clock_t cpu_start, cpu_finish;
	double cpu_duration, max_cpu_duration;
	cpu_start = clock();	
	time_t h_start, h_finish;
	h_start = time(NULL);
	//printf("Time %d\n", h_start);

	// malloc space to save target id
	target_list = (H_TARGET *) calloc(tool_info[0], sizeof(H_TARGET));

	// contains the states of each target process
	// either in or out of MPI call
	state = (short *) calloc(tool_info[0], sizeof(short));

	//printf("%d: %d, %d, %s\n", tool_rank, tool_info[0], tool_info[1], cmd_name);

	/////////////////////////STEP 2: Find & Map////////////////////////

	// claim that this tool has been started successfuly
	h_claim_start(tool_rank);

	// find all target processes
	if (h_search(cmd_name, tool_info[0], target_list) == ERROR) {
		printf("Rank %5d: Unable to find targets!\n", tool_rank);
		return ERROR;
	}

	// construct map between mpi rank and process id
	h_map(tool_info[0], tool_info[1], target_list);


#ifdef H_RAND_CHECK //@1

	// new MPI communicator
	MPI_Comm new_comm;

	// rank in the new communicator
	int tool_rank_new = -1;

	// size of the new communicator
	int tool_size_new = -1;

	// the number of local random target tasks used for checking
	int local_rand_num[2] = {0, 0};

	// the default number of spots we malloc for loacal random task list
	//int dft_num = (tool_info[0] < RAND_TASKS_NUM ? tool_info[0] : RAND_TASKS_NUM);

	// this will be used to split communicator
	int color = 0;
	// H_TARGET *local_rand_tasks = (H_TARGET *) calloc(dft_num, sizeof(H_TARGET));
	H_TARGET local_rand_tasks[2][RAND_TASKS_NUM/2];

	// generate a small number of random tasks
	h_generate_rand_tasks(tool_rank, tool_size, tool_info, local_rand_num,
			local_rand_tasks);

	// split communicator into 2: one for the random tasks,
	// the other for the remaining
	h_comm_split(tool_rank, local_rand_num, &color, &new_comm,
			&tool_rank_new, &tool_size_new);

	// map the rank of random tasks to the corresponding process id
	h_map_rand(tool_info, target_list, local_rand_num, local_rand_tasks);

	h_claim_rand_info(tool_rank, tool_size, tool_info, local_rand_num,
			local_rand_tasks);

#ifdef PHASE_DETECTION
	if (tool_rank_new == 0 && color == RAND_COLOR) {
		struct sigaction h_action;
		//h_action.sa_sigaction = (void *)h_sig_handler;
		h_action.sa_handler = (void *)h_sig_handler_single_mark;
		sigfillset(&h_action.sa_mask);
		//h_action.sa_flags = SA_SIGINFO;
		h_action.sa_flags = SA_RESTART;

		printf("registered signals: ");
		for(i = 0; i < MAX_PHASE_NUM -1; i++) {
			sigaction(sig[i], &h_action, NULL);
			printf("%4d", sig[i]);
		}
		printf("\n\n");
		//sigaction(SIGUSR1, &h_action, NULL);
		//sigaction(SIGUSR2, &h_action, NULL);
	}
#endif

#endif // @1 end of H_RAND_CHECK

	/////////////////////////STEP 3: Hang detection//////////////////////

	int check_ret;

#ifndef H_RAND_CHECK // @2

	check_ret = h_check_all2(tool_rank, tool_info, target_list, state);

	if (check_ret == ERROR) {
		//h_report(tool_rank, tool_info, target_list, state);
		h_report_multi_backtrace(tool_rank, tool_info, target_list, state);
	}

#else // @2 if H_RAND_CHECK

	int **simple_model;
	int how_many = 1;
#ifdef PHASE_DETECTION
	how_many = MAX_PHASE_NUM;
#endif
	if (color == RAND_COLOR) {
		simple_model = (int **)calloc(how_many, sizeof(int *));
		for(i = 0; i < how_many; i++) {
			simple_model[i] = (int *)calloc(2, sizeof(int));
			// temp fix
			simple_model[i][0] = simple_model[i][1] = 1;
		} 
	}

	do {
		check_ret = h_check_rand_model(tool_rank, color, tool_info, tool_rank_new,
				tool_size_new, local_rand_num, new_comm, local_rand_tasks, state, simple_model);
		
		if (check_ret == ERROR) {
			
			int tmp_ret = h_verify(tool_rank, tool_info, target_list);	
			//int tmp_ret =h_check_all3(tool_rank, tool_info, target_list, state);	
			if ( tmp_ret == ERROR)
				continue;
			else if (tmp_ret == OK) {
				if (tool_rank == 0)
					printf("\n%d: HORRIBLE  @ %ld\n", tool_rank, time(NULL));
				h_report_multi_backtrace(tool_rank, tool_info, target_list, state);
			}
		}
		break;
	} while(1);
#endif // @2 H_RAND_CHECK

	/////////////////////STEP 4: Release Resources/////////////////////

#ifdef H_RAND_CHECK	
	if (color == RAND_COLOR) {
		for (i = 0; i < how_many; i++)
			free(simple_model[i]);
		free(simple_model);
	}
#endif	

	free(target_list);
	free(state);

	cpu_finish = clock();
	cpu_duration = (double)(cpu_finish - cpu_start) / CLOCKS_PER_SEC;
	MPI_Reduce(&cpu_duration, &max_cpu_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	h_finish = time(NULL);	
	if (tool_rank == 0) {
		printf("Total running time: %ld - %ld => %ld\n", h_finish, h_start, h_finish - h_start);
		printf("Total CPU time: %ld - %ld => %lf\n", cpu_finish, cpu_start, max_cpu_duration);
	}
	h_claim_exit(tool_rank, check_ret);

	MPI_Finalize();

	return 0;
}


