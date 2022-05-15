#include "h_step.h"
#include "h_qnorm.h"

// tolerant false probability
const double false_prob = 0.001;
// variable from source file stack.c
extern int phase_id;

// time slot between two consecutive times of checking
static int check_interval = 400;
// sedd for generating random number
int rand_seed = 0;

float median = 5;
const int runsTestSize = 16;
const int run_test_tableA[7] = {  2,   2,   3,   4,   4,   4,   4};
const int run_test_tableB[7] = {100, 100, 100, 100,  13,  14,  14};
// for big sample size, runs test
//const float randTestSig = 0.025;

#ifdef H_RAND_CHECK
// model building time
const int model_build_phase = 20000;
const int model_build_times = 20;
// lower_bound specifies a CROUTS value, below which a suspicion
// is implied
int lower_bound = 0;
// swap task set after a certain times of checking stacks
const int swap_set_times = 30;
// the threshold ratio for h_check_all3
const float threshold_outs_ratio = 0.05;
// the total number of backtrace times for reporting RC tasks
const int bt_times_rc = 5;
// very big sample size
#define INFINITY_SIZE 10000000
#else
/***check all***/
// this value means a suspicion of a hang
#define MIN_OUT_MPI_ALL 20
#endif

#ifdef PHASE_DETECTION
extern const int sig[];
extern int phase_times[];
#endif


inline int h_mpi_parse(int argc, char **argv,
		int *tool_rank, int *tool_size,
		char *cmd_name, int *tool_info)
{
	int opt;

	while ((opt = getopt(argc, argv, "n:p:c:")) != -1) {
		switch (opt) {
			case 'n':
				tool_info[2] = atoi(optarg);
				break;
			case 'p':
				tool_info[0] = atoi(optarg);
				break;
			case 'c':
				strcpy(cmd_name, optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-c cmd_name] [-n file_name]\n",
						argv[0]);
				return ERROR;
		}
	}

	// return if cmd_name or file_name not obtained
	if(strlen(cmd_name) == 0 || tool_info[0] == -1 || tool_info[2] == -1) {
		printf("Illegal input, exit now! \n");
		return ERROR;
	}

	tool_info[1] = *tool_rank * tool_info[0];
	if ( *tool_rank == *tool_size -1 ) {
		tool_info[0] = tool_info[2] - tool_info[1];
	}

	return OK;
}


inline int h_mpi_init(int argc, char **argv,
		int *tool_rank, int *tool_size,
		int *tool_info, char *cmd_name)
{
	int i;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, tool_rank);
	MPI_Comm_size(MPI_COMM_WORLD, tool_size);

	// parse parameters ==> cmd_name
	if(h_mpi_parse(argc, argv, tool_rank, tool_size, cmd_name, tool_info) == ERROR)
		return ERROR;

	// debug
	// printf("%d: %d, %d, %d\n", *tool_rank, tool_info[0], tool_info[1], tool_info[2]);
	// sleep(5);
	return OK;
}


int h_backtrace_all_clean(int tool_rank, int *tool_info,
		H_TARGET *target_list, short *state) {

	int i = 0;

	memset(state, OUT_MPI_STATE, sizeof(short)*tool_info[0]);

	// backrace
	for (i = 0; i < tool_info[0]; i++) {
		if (h_attach_process(&target_list[i]) == ERROR) {
			printf("Attach Error from\n");
			return ERROR;
		}
		if (h_backtrace(target_list[i], &state[i]) == ERROR) {
			printf("Backtrace Error from\n");
			return ERROR;
		}
		
		if (h_detatch_process(target_list[i]) == ERROR) {
			printf("Detach Error from\n");
			return ERROR;
		}
	}

	return OK;
}

int h_backtrace_all(int tool_rank, int *tool_info,
		H_TARGET *target_list, short *state) {

	int i = 0;

	memset(state, OUT_MPI_STATE, sizeof(short)*tool_info[0]);

	// backrace
	for (i = 0; i < tool_info[0]; i++) {
		if (h_attach_process(&target_list[i]) == ERROR) {
			printf("Attach Error from\n");
			return ERROR;
		}
		if (h_backtrace2(target_list[i], &state[i]) == ERROR) {
			printf("Backtrace Error from\n");
			return ERROR;
		}
		
		if (OUT_MPI_STATE == state[i])
			printf("%10d\n", target_list[i].rank);

		if (h_detatch_process(target_list[i]) == ERROR) {
			printf("Detach Error from\n");
			return ERROR;
		}
	}

	return OK;
}

int h_backtrace_all_funcname(int tool_rank, int *tool_info,
		H_TARGET *target_list, char *p_funcname_set) {

	int i = 0;
	char tmp[30] = "\0";
	memset(p_funcname_set, '\0', sizeof(char)*30*tool_info[0]);

	// backrace
	for (i = 0; i < tool_info[0]; i++) {
		memset(tmp, '\0', sizeof(char)*30);
		if (h_attach_process(&target_list[i]) == ERROR) {
			printf("Attach Error from\n");
			return ERROR;
		}
		if (h_backtrace_funcname(target_list[i], tmp) == ERROR) {
			printf("Backtrace Error from\n");
			return ERROR;
		} 
		strncpy(p_funcname_set+i*30, tmp, 30);

		if (h_detatch_process(target_list[i]) == ERROR) {
			printf("Detach Error from\n");
			return ERROR;
		}
	}

	return OK;
}

#ifndef H_RAND_CHECK

int h_check_all2(int tool_rank, int *tool_info,
		H_TARGET *target_list, short *state)	{

	int i = 0;

	// total number of tasks outside of MPI call
	int out_total = 0;

	// probability model
	int model[2];
	model[0] = model[1] = 1;

	double no_hang_prob = 1.0;
	double reject_prob = 1.0;

	// the number of tasks outside of MPI call on each node @MPI mode
	// the total number of tasks outside of MPI call @NONE_MPI mode
	int out_local = 0;

	// the return value of this running that
	// needs to be passed to the other communicator
	int check_ret;

	// random check interval to be used
	int rand_check_interval;

	// generate the seed during running
	if (tool_rank == 0) {
		rand_seed == (unsigned int) MPI_Wtime ();
	}
	MPI_Bcast (&rand_seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

	// plant the seed for random number generator
	srand(rand_seed);

	// iteration index
	int j = 0;
	int k = 0;
	int temp_ret, all_ret;

	short *d_state = (short *)calloc(tool_info[0], sizeof(short));

	while (1) {

		rand_check_interval = check_interval/2 + rand() % check_interval + 1;

		h_msleep(rand_check_interval);

		// empty recording variables
		out_local = 0;

		for (i = 0; i < tool_info[0]; i++)
			d_state[i] = state[i];
		// backtrace all targets
		temp_ret = h_backtrace_all_clean(tool_rank, tool_info, target_list, state);
		MPI_Allreduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

		if (all_ret == ERROR) {

			if (tool_rank == 0) {
				// print model information
				printf("\nThe rejection prob is:\n");
				printf ("%7.3f\n", reject_prob);
			}

			// return OK == the target job finish successfully
			break;
		};

		// compute the tasks outside of MPI call
		for (i = 0; i < tool_info[0]; i++) {

			//if (state[i] == OUT_MPI_STATE)
			//out_local++;
			//if (j > 70 && d_state[i] != state[i])
			//printf("%d: rank: %d\n", j, target_list[i].rank);
			if (state[i] == INVALID_STATE)
				out_local++;
		}
		// get the total number of tasks outside of MPI call
		MPI_Allreduce(&out_local, &out_total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

		if (tool_rank == 0)
			printf("%d: %d\n", j, out_total);
		j++;
		continue;

		// increment the observed
		if (out_total <= 0.01*tool_info[2]){
			model[0] ++;
		}
		// increment the total number
		model[1] ++;

		// build model
		if (j < model_build_times) {

			reject_prob = (double)model[0] / (double)model[1];

			if (tool_rank == 0) {
				printf("Model creation:   %5d, %5d,         / %8.6f\n"
						, j, out_total, reject_prob);
			}

			j++;
			continue;
		}

		// update the healthiness of program
		if (out_total <= 0.01*tool_info[2]) {
			no_hang_prob *= reject_prob;
		} else {
			no_hang_prob = 1.0;
			reject_prob = (double)model[0] / (double)model[1];
		}

		// show health information of this task
		if (tool_rank == 0) {
			printf("Hang detection:   %5d, %5d, %8.6f / %8.6f\n",
					j, out_total, no_hang_prob, reject_prob);
		}

		// bug fix, iteration counts for all monitors
		j++;

		// warn @ too many times of suspicious behaviors
		if (no_hang_prob <= false_prob) {

			// inform the other communicator
			if (tool_rank == 0) {
				printf("\n%d: Hang: it is dead @ %ld\n", tool_rank, time(NULL));

				// print model information
				printf("\nThe rejection prob is:\n");
				printf ("%7.3f\n", reject_prob);
			}

			// return ERROR == the target job comes to a hang
			return ERROR;
		}
	}

	return OK;
}

/*
int h_check_all(int tool_rank, int *tool_info,
		H_TARGET *target_list, short *state)	{

	int i = 0;

	// total number of tasks outside of MPI call
	int out_total = 0;

	// previous value of out_total
	int prev_out_total = MIN_OUT_MPI_ALL + 1;

	// the number of tasks outside of MPI call on each node @MPI mode
	// the total number of tasks outside of MPI call @NONE_MPI mode
	int out_local = 0;

	// record the times of suspicious behavior
	int suspition_times = 0;

	// sedd for generating random number
	//int rand_seed = 0;

	// random check interval to be used
	int rand_check_interval;

	// generate the seed during running
	if (tool_rank == 0) {
		rand_seed == (unsigned int) MPI_Wtime ();
	}
	MPI_Bcast (&rand_seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

	// plant the seed for random number generator
	srand(rand_seed);

	// iteration index
	int j = 0;

	while (1) {

		//rand_check_interval = check_interval/2 + rand() % check_interval + 1;
		//h_msleep(rand_check_interval);

		//h_msleep(check_interval);

		// empty recording variables
		out_local = 0;

		// backtrace all targets
		if (h_backtrace_all_clean(tool_rank, tool_info,
					target_list, state) == ERROR)
			break;

		for (i = 0; i < tool_info[0]; i++) {
			if (state[i] == OUT_MPI_STATE)
				out_local++;
		}

		MPI_Allreduce(&out_local, &out_total, 1, MPI_INT,
				MPI_SUM, MPI_COMM_WORLD);

		if(out_total <= MIN_OUT_MPI_ALL) {

			if(prev_out_total <= MIN_OUT_MPI_ALL)
				suspition_times++;
			else
				suspition_times = 1;
		} else {
			suspition_times = 0;
		}

		prev_out_total = out_total;

		if (tool_rank == 0)
			printf("%5d, %5d\n", j++, out_total);

	}

	return OK;
} */



int h_report(int tool_rank, int *tool_info,
		H_TARGET *target_list, short *state)	 {

	int i = 0;

	int out_local = 0;
	int out_total = 0;

	for (i = 0; i < tool_info[0]; i++) {
		if (state[i] == OUT_MPI_STATE) {
			out_local++;
		}
	}

	MPI_Reduce (&out_local, &out_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	if (tool_rank == 0) {
		printf("TOTAL is : %d\n", out_total);
	}	

	// master task to report
	if (tool_rank == 0) {

		if (out_local > 0) {
			// report local suspicious tasks
			for (i = 0; i < tool_info[0]; i++) {
				if (state[i] == 0) {
					printf("ROOT CAUSE @ rank:%5d @ node:     0 @ %ld.\n",
							target_list[i].rank, time(NULL));
				}
			}
		}

		// total number of suspicious tasks globally
		int out_remote = out_total - out_local;
		if (out_remote == 0)
			return OK;

		// report remote suspicious tasks
		// preparation for MPI_Irecv & MPI_Wait
		MPI_Request *req = (MPI_Request *)malloc(sizeof(MPI_Request)*out_remote);
		int *root_cause = (int *)malloc(sizeof(int)*out_remote);
		int req_index;
		MPI_Status status;
		int req_count = out_remote;
		int flag;

		// initialize the rank array as all '-1'
		memset(root_cause, -1, sizeof(root_cause));

		for (i = 0; i < out_remote; i++) {
			MPI_Irecv(&root_cause[i], 1, MPI_INT, MPI_ANY_SOURCE, 0,
					MPI_COMM_WORLD, &req[i]);
		}



		MPI_Waitall(out_remote, req, MPI_STATUSES_IGNORE);
		for (i = 0; i < out_remote; i++) {
			printf("ROOT CAUSE @ rank:%5d @ node: %5d @ %ld.\n",
					root_cause[i], root_cause[i]%tool_info[0], time(NULL));
			fflush(stdout);

		}

		// used to break the loop by force
		/*		while (i < out_remote) {
				MPI_Testany(req_count, req, &req_index, &flag, &status);

				if (flag) {
				i++;
				printf("ROOT CAUSE @ rank:%5d @ node: %5d @ %ld.\n",
				root_cause[req_index], status.MPI_SOURCE, time(NULL));
				fflush(stdout);
				}

		// sleep for 50 milliseconds
		//h_msleep(50);
		}*/
	}
	// slave tasks send information to master task
	else {

		for (i = 0; i < tool_info[0]; i++) {
			if (state[i] == 0) {

				//				printf("%d, %d", tool_rank, target_list[i].rank);
				MPI_Send(&target_list[i].rank, 1, MPI_INT, 0, 0,
						MPI_COMM_WORLD);
			}
		}
	}

	return OK;
}

#else





inline void h_get_root_other_color (int tool_rank,
		int tool_rank_new, int tool_size_new,
		MPI_Comm this_comm, int *root_other_color) {

	int i;

	int *all;

	if (tool_rank_new == 0) {
		all = (int *) malloc (sizeof(int) * tool_size_new);
	}
	MPI_Gather (&tool_rank, 1, MPI_INT, all, 1, MPI_INT,
			0, this_comm);

	if ( tool_rank_new == 0 )
	{
		if ( all[0] != 0 ) {
			*root_other_color = 0;
		} else {
			for (i = 0; i < tool_size_new-1; i++) {
				if (all[i] + 1 < all[i+1]) {
					*root_other_color = all[i]+1;
					break;
				}
			}

			if (i == tool_size_new-1)
				*root_other_color = all[tool_size_new - 1] + 1;
		}
	}
}


float h_chi_squared_test (int tool_rank_new, int *model, int *backup_model[]) {

	int i;
	int test[12] = {0};
	for (i = 0; i < 12; i++)
		test[i] = 0;
	// X^2 testing
	float chi_square = 0;
	float freq_E = 0;
	int k = 0;
	for (i = 0; i < RAND_TASKS_NUM/2 + 2; i++) {
		for (k = 1; k < 5; k++)
			test[i] += backup_model[k][i];
	}

	for (i = 0; i < RAND_TASKS_NUM/2 + 1; i++) {
		freq_E = (float)(test[i]) / test[RAND_TASKS_NUM/2 + 1]
			* (model[RAND_TASKS_NUM/2 + 1] - test[RAND_TASKS_NUM/2 + 1]);
		if (freq_E != 0)
			chi_square += (model[i] - test[i] - freq_E) *
				(model[i] - test[i] - freq_E) / freq_E;

	}


	if (tool_rank_new == 0) {
		printf ("\nThe Chi-squared test statistics is %f\n", chi_square);
	}

	if ( chi_square > 18.307 ) {

		if (tool_rank_new == 0) {
			printf("\n Model change: \n");
		}
		for (i = 0; i < RAND_TASKS_NUM/2 + 2; i++) {
			model[i] = test[i];
			if (tool_rank_new == 0) {
				printf("%4d", model[i]);
			}
		}
	} else {
		if(tool_rank_new == 0) {
			printf("\n Latest model: \n");
			for (i = 0; i < RAND_TASKS_NUM/2 + 2; i++) {
				printf("%4d", test[i]);
			}
			printf("\n\n");
		}
	}

	return chi_square;
}

int h_active_check(int tool_rank, int *tool_info,
		int tool_rank_new, int tool_size_new,
		int *local_rand_num, MPI_Comm this_comm,
		H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2], 
		short *state, int **simple_model) {

	int i = 0, j = 0;

	// total number of tasks outside of MPI call
	int out_total = 0;

	// the number of tasks outside of MPI call on each node @MPI mode
	int out_local = 0;

	// the root of the other communicator
	int other_root_rank;

	// the return value of this running that
	// needs to be passed to the other communicator
	int check_ret;

	// obtain the root rank of the other communicator
	h_get_root_other_color (tool_rank, tool_rank_new, tool_size_new,
			this_comm, &other_root_rank);

	// sedd for generating random number
	//int rand_seed = 0;
	// random check interval to be used
	int rand_check_interval;
	// generate the seed during running
	if (tool_rank_new == 0) {
		rand_seed == (unsigned int) MPI_Wtime ();
	}

	MPI_Bcast (&rand_seed, 1, MPI_UNSIGNED, 0, this_comm);
	if (tool_rank_new == 0) {
		printf("rand_seed: %d\n", rand_seed);
	}
	// plant the seed for random number generator
	srand(rand_seed);

	// swap random set by k
	int k = 0;
	// record return value
	int temp_ret, all_ret;
#ifdef DEBUG
	static short d_tag = 0;
	static int detailed_model[MAX_PHASE_NUM][RAND_TASKS_NUM/2+1];
	if (d_tag == 0){
		memset(detailed_model, 0, sizeof(detailed_model));
		d_tag = 1;
	} else {
		printf("Monitor resumed\n");
	}
#endif

	// iteration index
	static int sample_size = 0;
	double no_hang_prob = 1.0;
	static double reject_prob = 1.0;

	// temporary phase_id
	int tmp_phase_id = phase_id;
	short phase_change_tag = 0;

	// run test to set rmax: only work for non-phased behavior
	int runs = 0, zeros = 0, ones = 0;
	int pre_value = 0;
	int sample_sum = 0;
	char randTestSwitch;
	int bound1, bound2, min;
	// timer for runs test & model building
	static int stable_times = 0;
	static double time_counter = 0.0;

	// size, q, e
	static int robustN[4] = {11, 19, 42, 86};
	const double tolerance[4] = {0.3, 0.2, 0.1, 0.05};
	const double prob[4] = {0.47, 0.27, 0.12, 0.06};
	static int robustL[4];
	static int robustS[4];
	int robust_index = 0;
	int robust_switch = 0;

	//double mu, sigma, bound1, bound2;	
	while (1) {

		rand_check_interval = check_interval/2 + rand()%check_interval + 1;
		h_msleep(rand_check_interval);
		time_counter += rand_check_interval;

		// empty recording variables
		out_local = 0;
		// reset state array
		memset(state, OUT_MPI_STATE, sizeof(short)*tool_info[0]);

		k = (sample_size / swap_set_times) % 2;
		/*********************************************************************************1*/
		// backtrace all targets

		temp_ret = h_backtrace_rand(tool_rank_new, local_rand_num[k], local_rand_tasks[k], state);

		/**********************************************************************************2*/
		// check error
		MPI_Allreduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, this_comm);
		if (all_ret == ERROR) {
			check_ret = OK;

			if (tool_rank_new == 0) {
				// print model information
				printf("\nThe rejection probability is %7.6f\n", reject_prob);

#ifdef DEBUG
				printf("\nDetailed model information:\n");
				for (i = 0; i < MAX_PHASE_NUM; i++) {
					printf("Model %d: \n", i);
					for(j = 0; j < RAND_TASKS_NUM/2+1; j++) {
						printf("%-5d", detailed_model[i][j]);
					}
					printf("\n");
				}	
				printf("\n");			
#endif

				// inform inactive monitors
				int global_comm_size;
				MPI_Comm_size (MPI_COMM_WORLD, & global_comm_size);	
				printf("global:%d, tool_size: %d\n", global_comm_size, tool_size_new);
				if (global_comm_size == tool_size_new)
					break;
				printf("The other root %d\n", other_root_rank);
				MPI_Send (&check_ret, 1, MPI_INT, other_root_rank, 0, MPI_COMM_WORLD);
			}

			// return OK == the target job finish successfully
			break;
		};

		/**********************************************************************************3*/
		// compute the tasks outside of MPI call
		for (i = 0; i < local_rand_num[k]; i++) {
			if (state[i] == OUT_MPI_STATE) 
				out_local++;

		}

		// get the total number of tasks outside of MPI call
		MPI_Allreduce(&out_local, &out_total, 1, MPI_INT, MPI_SUM, this_comm);

		// increase sample size counter
		sample_size++;

		// refresh model
		if (out_total <= lower_bound) {
			simple_model[tmp_phase_id][0]++;
		}
		simple_model[tmp_phase_id][1]++;
#ifdef DEBUG
		detailed_model[tmp_phase_id][out_total]++;
#endif

		// perform runs test in the 1st minutes 
		if (time_counter < 60000 || stable_times < 1) {
			// record runs test statistics
			if (out_total >= median) {
				ones += 1;
				if (randTestSwitch == 0) {
					//runs += 1;
					randTestSwitch = 1;
				}
				else if (pre_value < median)
					runs += 1;
			}
			else if (out_total < median){
				zeros += 1;
				if (randTestSwitch == 0) {
					//runs += 1;
					randTestSwitch = 1;
				}
				else if (pre_value >= median)
					runs += 1;
			}
			pre_value = out_total;
			sample_sum += out_total;

			// runs test
			if (ones+zeros == runsTestSize) {
				// the times that r_max not being reset
				stable_times++;

				runs += 1;

				//mu = (2*zeros*ones)/runsTestSize + 1;
				//sigma = (mu-1)*(mu-2)/(runsTestSize - 1);
				//bound2 = QNorm(randTestSig, mu, sigma, 0, 0);
				//bound1 = 2*mu - bound2;

				if (ones >= 2 && zeros >= 2) {
					// turn on robustness adjustness
					robust_switch = 0;
					min = ones <= zeros ? ones: zeros;
					bound1 = run_test_tableA[min-2];
					bound2 = run_test_tableB[min-2];
					if (runs <= bound1 || runs >= bound2) {
						stable_times = 0;
						// reset interval
						check_interval *= 2;

						//update model for larger interval
						int tmp_accum = 0;
						for (i = 0; i < RAND_TASKS_NUM/2+1; i++){
							detailed_model[tmp_phase_id][i] = (double)detailed_model[tmp_phase_id][i] * 0.5;
							tmp_accum += detailed_model[tmp_phase_id][i];
							if (i <= lower_bound)
								simple_model[tmp_phase_id][0] += detailed_model[tmp_phase_id][i];
						}
						simple_model[tmp_phase_id][1] = tmp_accum;

						if (tool_rank_new == 0) {
							//printf("mean = %lf, sigma = %lf, (%lf~%lf), %d\n", 
							//        mu, sigma, bound1, bound2, runs);
							printf("@RESET: range = (%d, %d), runs = %d\n", bound1, bound2, runs);
							printf("@Model adjust: #suspicions = %d, #all = %d\n", 
									simple_model[tmp_phase_id][0], simple_model[tmp_phase_id][1]);
						}
					}
				} else {
					stable_times = 0;
					// turn on robustness adjustness
					robust_switch = 0;
					// reset interval
					check_interval *= 2;

					//update model for larger interval
					int tmp_accum = 0;
					for (i = 0; i < RAND_TASKS_NUM/2+1; i++){
						detailed_model[tmp_phase_id][i] = (double)detailed_model[tmp_phase_id][i] * 0.5;
						tmp_accum += detailed_model[tmp_phase_id][i];
						if (i <= lower_bound)
							simple_model[tmp_phase_id][0] += detailed_model[tmp_phase_id][i];
					}
					simple_model[tmp_phase_id][1] = tmp_accum;

					if (tool_rank_new == 0) {
						printf("#RESET: runs = %d\n", runs);
						printf("#Model adjust: #suspicions = %d, #all = %d\n", 
								simple_model[tmp_phase_id][0], simple_model[tmp_phase_id][1]);
					}
				}

				if (tool_rank_new == 0) {
					printf("ones = %d, zeros = %d, runs = %d, boundary = %f, r_max = %d\n", 
							ones, zeros, runs, median, check_interval);
					printf("stable_times = %d\n", stable_times);
					printf("\n");
				}

				// reset
				ones = zeros = runs = 0;
				randTestSwitch = 0;
				median = (float)sample_sum / runsTestSize;
				sample_sum = 0;
			}
		}

		/**********************************************************************************4*/

		static int prev_model_size = 10;

		// update every 16 times
		if (simple_model[tmp_phase_id][1] >= 1.1*prev_model_size || robust_switch == 0) {

			// update previous size
			prev_model_size = simple_model[tmp_phase_id][1];

			// disable robustness adjustness 
			robust_switch = 1;

			// compute start_index
			int start_index;
			if (robust_index == -1)
				start_index = 0;
			else
				start_index = robust_index;
			if (tool_rank_new == 0)
				printf("%5d\n", start_index);

			for (j = start_index; j < 4; j++) {

				robustS[j] = 0;
				// avoid *divide by 0*
				double accum_prob = 0.000001;
				// set prev to a very big number such that 
				int prev = INFINITY_SIZE, cur = 0;

				for (i = 0; i < RAND_TASKS_NUM/2+1; i++){
					accum_prob += (double)detailed_model[tmp_phase_id][i]/(double)simple_model[tmp_phase_id][1];
					robustS[j] += detailed_model[tmp_phase_id][i];
					if (accum_prob >= prob[j]) {
						if (accum_prob <= 1-prob[j])
							cur = 3.8416*accum_prob*(1-accum_prob) / (tolerance[j]*tolerance[j]);
						else
							cur = 5.0 / (1 - accum_prob);
						// note accum_prob should not be equal to 1.0
						if (cur <= prev && robustS[j] != simple_model[tmp_phase_id][1]) {
							robustL[j] = i;
							robustN[j] = cur;
						} else {
							robustL[j] = i-1;
							robustN[j] = prev;
							robustS[j] -= detailed_model[tmp_phase_id][i];
						}
						break;
					}
					prev = 5.0 / accum_prob;
				}
			}

			// update lower_bound and simple_model
			robust_index = -1;
			for (i = 3; i >=0; i--) {
				if (simple_model[tmp_phase_id][1] > robustN[i]){
					robust_index = i;
					lower_bound = robustL[i];
					simple_model[tmp_phase_id][0] = robustS[i];
					break;
				}
			}
			/*if (i == 0) {
			  robust_index = 0;
			  lower_bound = robustL[0];
			  simple_model[tmp_phase_id][0] = robustS[0];
			  }*/
			reject_prob = (double)simple_model[tmp_phase_id][0] / (double)simple_model[tmp_phase_id][1];

			if (tool_rank_new == 0) {
				printf("\n lower_bound: %5d \n", lower_bound);
				printf("model size requirement: ");   
				for (i = 0; i < 4; i++) {
					printf("%5d", robustN[i]);   
				}
				printf("\n");
			}
		}

		// sample size not enough ==> continue learning
		if (robust_index == -1) {
			if (tool_rank_new == 0) 
				printf("Model building: %5d, %5d\n", sample_size, out_total);
			continue;
		}


		/**********************************************************************************4*/
		// update the healthiness of program
		if (out_total <= lower_bound) {
			if (reject_prob <= 0.5)
				no_hang_prob *= (reject_prob + tolerance[robust_index]);
			else
				no_hang_prob *= reject_prob;
		} else {
			no_hang_prob = 1.0;
			//reject_prob = (double)simple_model[tmp_phase_id][0] / (double)simple_model[tmp_phase_id][1];
		}
		// show health information of this task
		if (tool_rank_new == 0) {
			printf("Hang detection:   %5d, %5d, %8.6f / %8.6f\n",
					sample_size, out_total, no_hang_prob, reject_prob);
		}
		/**********************************************************************************6*/
		
		// debug
		//static int bug1 = 0, bug2 = 0;
		//bug2 = bug1;
		//bug1 = time_counter / 30000;
		
		//debug 
		// warn @ too many times of suspicious behaviors
		if (no_hang_prob < false_prob) { //|| bug1 - bug2 >= 1) {
			if (tool_rank_new == 0) 
				printf("NAH\n");			
#ifndef DISABLE_HANG_CHECK
			// target job comes to a hang
			check_ret = ERROR;

			// inform the other communicator
			if (tool_rank_new == 0) {
				printf("\n%d: Hang: it is dead @ %ld\n", tool_rank, time(NULL));
				//printf("W0.00001 @ %ld\n", time(NULL));
				// print model information
				printf("\nThe rejection probability is %7.6f\n", reject_prob);

#ifdef DEBUG
				printf("\nDetailed model information:\n");
				for (i = 0; i < MAX_PHASE_NUM; i++) {
					printf("Model %d: \n", i);
					for(j = 0; j < RAND_TASKS_NUM/2+1; j++) {
						printf("%-5d", detailed_model[i][j]);
					}
					printf("\n");
				}
				printf("\n");				
#endif

				int global_comm_size;
				MPI_Comm_size (MPI_COMM_WORLD, & global_comm_size);	
				// bug fix @ small scale
				if (global_comm_size == tool_size_new)
					return ERROR;
				MPI_Send (&check_ret, 1, MPI_INT, other_root_rank, 0, MPI_COMM_WORLD);
			}

			// return ERROR == the target job comes to a hang
			return ERROR;
#endif
		}
	}

	return OK;
}

int h_idle_wait (int tool_rank_new, int tool_size_new, MPI_Comm this_comm) {

	// MPI_Irecv parameters
	MPI_Request recv_req;
	int recv_flag;

	// return value passed from the other communicator
	int check_ret;

	if (tool_rank_new == 0) {
		// receive information from the other communicator
		MPI_Irecv (&check_ret, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
				&recv_req);
	} else {
		// receive signal
		MPI_Irecv (&check_ret, 1, MPI_INT, MPI_ANY_SOURCE, 0, this_comm, &recv_req);
	}

	while (1) {
		// wait for the completion of previous receiving operation
		MPI_Test (&recv_req, &recv_flag, MPI_STATUS_IGNORE);

		// passing signal to the next task after successful receiving
		if (recv_flag) {
			//			// pipelined broadcast
			//			if (tool_rank_new != tool_size_new-1)
			//				MPI_Send (&check_ret, 1, MPI_INT, tool_rank_new+1, 0, this_comm);

			// pass signal to left child node
			if (2*tool_rank_new+1 < tool_size_new)
				MPI_Send (&check_ret, 1, MPI_INT, 2*tool_rank_new+1, 0, this_comm);

			// pass signal to right child node
			if (2*tool_rank_new+2 < tool_size_new)
				MPI_Send (&check_ret, 1, MPI_INT, 2*tool_rank_new+2, 0, this_comm);
			break;
		}
		h_msleep (1);
	}

	return check_ret;
}

int h_check_rand_model(int tool_rank, int color,
		int *tool_info,
		int tool_rank_new, int tool_size_new,
		int *local_rand_num, MPI_Comm this_comm,
		H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2], 
		short *state, int ** simple_model) {

	if (color == RAND_COLOR) {
		return h_active_check(tool_rank, tool_info, tool_rank_new, tool_size_new,
				local_rand_num, this_comm, local_rand_tasks, state, simple_model);
	} else { // for the communicator marked as OTHER_COLOR
		return h_idle_wait(tool_rank_new, tool_size_new, this_comm);
	}
}



int h_verify(int tool_rank, int *tool_info,
		H_TARGET *target_list)	{

	int temp_ret = 0, all_ret = 0;
	
	// random check interval to be used
	int rand_check_interval;

	// store the function name and each function name takes at most 30 characters
	char func_name_set[2][30*tool_info[0]];
	
	// debug
        //char file_name[10];
        //sprintf(file_name, "%d", tool_rank);
        //FILE *fp;
        //static int file_switch = 0;
        //if (0 == file_switch){
        //        fp = fopen(file_name, "w");
        //        file_switch = 1;
        //} else {
         //       fp = fopen(file_name, "a");
        //}
	
	int i, j = 0;
	
	while (j < 2) {
		h_msleep(check_interval);
		
		// debug		
		//fprintf(fp, "%d: %d, a\n", tool_rank, j);
		//fflush(fp);

		// backtrace all targets
		temp_ret = h_backtrace_all_funcname(tool_rank, tool_info, target_list, func_name_set[j%2]);
		
		// debug		
		//fprintf(fp, "%d: %d b\n", tool_rank, j);
		//fflush(fp);
		
		//MPI_Allreduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
		MPI_Reduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
		
		// debug		
		//fprintf(fp, "%d: %d c\n", tool_rank, j);
		//fflush(fp);
		
		MPI_Bcast (&all_ret, 1, MPI_INT, 0, MPI_COMM_WORLD);
		
		// debug		
		//fprintf(fp, "%d: %d d\n", tool_rank, j);
		//fflush(fp);

		if (all_ret == ERROR) {
			// return OK == the target job finish successfully
			return TARGET_FINISH;
		};
		j++;
	}
	
	//debug 
	MPI_Barrier(MPI_COMM_WORLD);
	if (tool_rank == 0) {
		printf("debug @ h_verify @ 1 finished\n");
		fflush(stdout);
	}

	for (i = 0; i < 2; i++) 
		for (j = 0; j < 30*tool_info[0]; j++) {
			if(func_name_set[i][j] >= 'A' && func_name_set[i][j] <= 'Z')
				func_name_set[i][j] += 32;
		}
	// debug
	//for (j = 0; j < tool_info[0]; j++)
	//	printf("%d: %s\n", tool_rank, *(func_name_set+0)+j*30);
	//for (j = 0; j < tool_info[0]; j++)
	//	printf("%d: %s\n", tool_rank, *(func_name_set+1)+j*30);

	temp_ret = all_ret = 0;	
	for (j = 0; j < tool_info[0]; j++) {
		
		// debug		
		//fprintf(fp, "%s: %s\n", *(func_name_set+0)+j*30, *(func_name_set+1)+j*30);
		//fflush(fp);
		
		if (strcmp(*(func_name_set+0)+j*30, *(func_name_set+1)+j*30) == 0) {
			continue;
		} else if (strstr(*(func_name_set+0)+j*30, "pthread_spin_init") != NULL ||
			strstr(*(func_name_set+1)+j*30, "pthread_spin_init") != NULL) {
			continue;
		} else if (strstr(*(func_name_set+0)+j*30, "mpi_test") != NULL  && func_name_set[1][j*30] == '\0') {
			printf("MPI_Test treated as IN_MPI\n");
			continue;
		} else if (strstr(*(func_name_set+1)+j*30, "mpi_test") != NULL  && func_name_set[0][j*30] == '\0') {
			printf("MPI_Test treated as IN_MPI\n");
			continue;
		} else if (strstr(*(func_name_set+0)+j*30, "mpi_iprobe") != NULL  && func_name_set[1][j*30] == '\0') {
			printf("MPI_Iprobe treated as IN_MPI\n");
			continue;
		} else if (strstr(*(func_name_set+1)+j*30, "mpi_iprobe") != NULL  && func_name_set[0][j*30] == '\0') {
			printf("MPI_Iprobe treated as IN_MPI\n");
			continue;
		} else {
			temp_ret = ERROR;
			break;
		}
	}

	// debug
	//fprintf(fp, "Monitor ID %d: (%d)\n", tool_rank, temp_ret);
	//fputs("\n\n", fp);
        //fclose(fp);
	
	//printf("Monitor ID %d: (%d)\n", tool_rank, temp_ret);
	//MPI_Allreduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
	MPI_Reduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Bcast (&all_ret, 1, MPI_INT, 0, MPI_COMM_WORLD);	
	
	if (all_ret == ERROR) {
		if(tool_rank == 0) {
			printf("Verification failed (%d) ==> resume!\n", all_ret);
			fflush(stdout);
		}
		// return OK == the target job finish successfully
		return ERROR;
	} else {
		if (tool_rank == 0) {
			printf("Verification passed (%d)!\n", all_ret);
			fflush(stdout);
		}
		return OK;
	}

}


int h_check_all3(int tool_rank, int *tool_info,
		H_TARGET *target_list, short *state)	{

	int i = 0;

	// total number of tasks outside of MPI call
	int out_total = 0;

	// the number of tasks outside of MPI call on each node @MPI mode
	// the total number of tasks outside of MPI call @NONE_MPI mode
	int out_local = 0;

	// sedd for generating random number
	//int rand_seed = 0;

	// random check interval to be used
	int rand_check_interval;


	int temp_ret = 0, all_ret = 0;


	int j = 0;
	//while (j < 100) {
	while (j < 3) {
		j++;

		h_msleep(rand()%(check_interval/3));

		// backtrace all targets
		temp_ret = h_backtrace_all(tool_rank, tool_info, target_list, state);
		MPI_Allreduce (&temp_ret, &all_ret, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

		if (all_ret == ERROR) {
			// return OK == the target job finish successfully
			return TARGET_FINISH;
		};

		// empty recording variables
		out_local = 0;
		for (i = 0; i < tool_info[0]; i++) {
			if (state[i] == OUT_MPI_STATE) {
				out_local++;
				//			printf("%5d, %5d\n", tool_rank, i);
			}
		}

		MPI_Allreduce(&out_local, &out_total, 1, MPI_INT,
				MPI_SUM, MPI_COMM_WORLD);


		if (tool_rank == 0)
			printf("OUTS: %5d, rand_seed: %d, threshold: %7.3f\n", out_total, rand_seed, threshold_outs_ratio * tool_info[2]);

		if (out_total > threshold_outs_ratio * tool_info[2]) {
			if(tool_rank == 0)
				printf("FALSE HANG, resume!\n");
			return ERROR;
		}  

	}
	return OK;
}

int h_backtrace_rand(int tool_rank_new, int local_rand_num,
		H_TARGET *local_rand_tasks, short *state) {

	int i = 0;

	// backrace
	for (i = 0; i < local_rand_num; i++) {
		if (h_attach_process(&local_rand_tasks[i]) == ERROR) {
			printf("Attach Error\n");
			return ERROR;
		}

		if (h_backtrace2(local_rand_tasks[i], &state[i]) == ERROR) {
			printf("Backtrace Error\n");
			return ERROR;
		}
		if (h_detatch_process(local_rand_tasks[i]) == ERROR) {
			printf("Detach Error\n");
			return ERROR;
		}
	}

	return OK;
}

#endif


int h_report_multi_backtrace(int tool_rank, int *tool_info,
	H_TARGET *target_list, short *state)	 {

	int i = 0, j = 0;

	int out_local = 0;
	int out_total = 0;

	// 3 times of backtrace
	int *records = calloc (tool_info[0], sizeof(int));
	for (i = 0; i < bt_times_rc; i++) {
		memset(state, 0, tool_info[0]);
		h_backtrace_all_clean(tool_rank, tool_info, target_list, state);

		for (j = 0; j < tool_info[0]; j++) {
			if (state[j] == OUT_MPI_STATE) {
				records[j]++;
			}
		}
		//		h_msleep(300);
	}
	// update the out_local
	for (i = 0; i < tool_info[0]; i++) {
		if (records[i] >= bt_times_rc) {
			out_local ++;
		}
	}

	// update the out_total
	MPI_Reduce (&out_local, &out_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	// master task to report
	if (tool_rank == 0) {

		printf("TOTAL is : %d\n", out_total);
		if (out_local > 0) {
			// report local suspicious tasks
			for (i = 0; i < tool_info[0]; i++) {
				if (records[i] >= bt_times_rc) {
					printf("ROOT CAUSE @ rank:%5d @ node:     0 @ %ld.\n",
							target_list[i].rank, time(NULL));
				}
			}
		}

		// total number of suspicious tasks globally
		int out_remote = out_total - out_local;
		if (out_remote == 0)
			return OK;

		// report remote suspicious tasks
		// preparation for MPI_Irecv & MPI_Wait
		MPI_Request *req = (MPI_Request *)malloc(sizeof(MPI_Request)*out_remote);
		int *root_cause = (int *)malloc(sizeof(int)*out_remote);
		int req_index;
		MPI_Status status;
		int req_count = out_remote;
		int flag;

		// initialize the rank array as all '-1'
		memset(root_cause, -1, sizeof(root_cause));

		for (i = 0; i < out_remote; i++) {
			MPI_Irecv(&root_cause[i], 1, MPI_INT, MPI_ANY_SOURCE, 0,
					MPI_COMM_WORLD, &req[i]);
		}

		MPI_Waitall(out_remote, req, MPI_STATUSES_IGNORE);
		for (i = 0; i < out_remote; i++) {
			printf("ROOT CAUSE @ rank:%5d @ node: %5d @ %ld.\n",
					root_cause[i], root_cause[i]/tool_info[0], time(NULL));
			fflush(stdout);

		}

		free(root_cause);

	}
	// slave tasks send information to master task
	else {

		for (i = 0; i < tool_info[0]; i++) {
			if (records[i] >= bt_times_rc) {
				//printf("####Sending###%d, %d", tool_rank, target_list[i].rank);
				MPI_Send(&target_list[i].rank, 1, MPI_INT, 0, 0,
						MPI_COMM_WORLD);
			}
		}
	}

	// free the allocation
	free(records);

	return OK;
}


int h_report_multi_backtrace2(int tool_rank, int *tool_info, int tool_size,
	H_TARGET *target_list, short *state)	 {

	int i = 0, j = 0;

	int out_local = 0;

	int *out_list = NULL;

	if(tool_rank == 0)
		out_list = calloc(tool_size, sizeof(int));;

	// 3 times of backtrace
	int *records = calloc (tool_info[0], sizeof(int));
	for (i = 0; i < 3; i++) {
		memset(state, 0, tool_info[0]);
		h_backtrace_all_clean(tool_rank, tool_info, target_list, state);

		for (j = 0; j < tool_info[0]; j++) {
			if (state[j] == OUT_MPI_STATE) {
				records[j]++;
			}
		}
		h_msleep(300);
	}
	// update the out_local
	for (i = 0; i < tool_info[0]; i++) {
		if (records[i] >= 2) {
			out_local ++;
		}
	}

	// update the out_total
	MPI_Gather (&out_local, 1, MPI_INT, out_list, tool_size, MPI_INT, 0, MPI_COMM_WORLD);

	// master task to report
	if (tool_rank == 0) {
		// comment out
		printf("TOTAL is : 0");
		if (out_local > 0) {
			// report local suspicious tasks
			for (i = 0; i < tool_info[0]; i++) {
				if (records[i] >= 2) {
					printf("ROOT CAUSE @ rank:%5d @ node:     0 @ %ld.\n",
							target_list[i].rank, time(NULL));
				}
			}
		}

		int root_cause;
		for (i = 1; i < tool_size; i++) {
			for (j = 0; j < out_list[i]; j++) {
				root_cause = -1;
				MPI_Recv (&root_cause, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				printf("ROOT CAUSE @ rank:%5d @ node: %5d @ %ld.\n",
						root_cause, root_cause/tool_info[0], time(NULL));
				fflush(stdout);
			}
		}

		free(out_list);
	}
	// slave tasks send information to master task
	else {

		for (i = 0; i < tool_info[0]; i++) {
			if (records[i] >= 2) {

				//				printf("%d, %d", tool_rank, target_list[i].rank);
				MPI_Send(&target_list[i].rank, 1, MPI_INT, 0, 0,
						MPI_COMM_WORLD);
			}
		}
	}

	// free the allocation
	free(records);

	return OK;
}


