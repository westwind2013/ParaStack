#ifndef H_COMMON_H

	#define H_COMMON_H

	// silence the insignificant inputs
	#define H_SILENCE

	/**************************checking method*************************/
	/***check only a few***/
	// use random check
	#define H_RAND_CHECK

	// the number of tasks used to detect hang in random mode
	#define RAND_TASKS_NUM 20
	/***check only a few***/

	//#define PHASE_DETECTION
	#ifdef PHASE_DETECTION
		#define MAX_PHASE_NUM 7
	#else
		#define MAX_PHASE_NUM 1
	#endif

	#define DEBUG
	//#define DISABLE_HANG_CHECK
	
	/**************************checking method*************************/

	/***********************program running state**********************/
	// normal exit
	#define OK 0
	// wrong
	#define ERROR -1
	// the target application finishes
	#define TARGET_FINISH 1
	/***********************program running state**********************/


	/*************************comparison result************************/
	// same
	#define YES 0
	// different
	#define NO -1
	/*************************comparison result************************/


	/***************************target state***************************/
	#define INVALID_STATE  2 
	// inside mpi call
	#define  IN_MPI_STATE  1
	// outside of mpi call
	#define OUT_MPI_STATE  0
	/***************************target state***************************/


	/*********************communicator splitting tag*******************/
	// the random communicator
	#define RAND_COLOR 1
	// the other
	#define OTHER_COLOR 0
	/*********************communicator splitting tag*******************/


	/*******************************others******************************/
	// length of command name
	#define CMD_NAME_LEN 100
	// length of stack frame name
	#define FRAME_NAME_LEN 100
	// structure records the mapping of MPI rank and process id
	

	typedef struct h_task_no_name{
		int rank;
		int proc_id;
	} H_TARGET;
	/*******************************others******************************/

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
    #include "mpi.h"

	#ifdef PHASE_DETECTION
		#include <signal.h>
	#endif	
	
	
#endif /* H_COMMON_H_ */
	
