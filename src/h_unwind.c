#include "h_unwind.h"

unw_addr_space_t addr_space = NULL;
struct UPT_INFO * upt_info = NULL;

int h_initialize(H_TARGET target) {

	// create new unwind address space and initializes it based on callback routines
	// passed via parameter 1 of type 'unw_accessors_t *' and parameter 2 (byteorder).
	addr_space = unw_create_addr_space(&_UPT_accessors, 0);
	if (addr_space == NULL) {
		fprintf(stderr, "Error @ unw_create_addr_space: %s\n\n",
				strerror(errno));
		return ERROR;
	}

	// 'struct UPT_INFO' variable needs to be filled to serves as argument pointer
	// to unw_init_remote.
	upt_info = _UPT_create(target.proc_id);
	if (upt_info == NULL) {
		fprintf(stderr, "Error @ _UPT_create: %s\n\n", strerror(errno));
		return ERROR;
	}

	return OK;
}

int h_finish() {

	// make sure to free up all memory and other resources
	_UPT_destroy(upt_info);

	// release all associated resources
	unw_destroy_addr_space(addr_space);

	return OK;
}

int h_backtrace(H_TARGET target, short *tag) {
	int ret;
	int i = 0, j = 0, temp = 0;

	h_initialize(target);

	// pointer for stack frames
	unw_cursor_t cursor;

	// current stack frame's name & offset.
	char frame_name[FRAME_NAME_LEN] = "\0";
	unw_word_t frame_name_offset = 0;

	// obtain the cursor pointing to the current stack frame of given context.
	ret = unw_init_remote(&cursor, addr_space, upt_info);

	if (ret < 0) {
		fprintf(stderr, "Error @ unw_init_remote: %s\n\n", strerror(errno));
		return ERROR;
	}
	
	// backtrace to check whether the current process is inside a MPI call
	do {
		// get current stack frame's name, instruction pointer and stack pointer
		ret = unw_get_proc_name(&cursor, frame_name, FRAME_NAME_LEN,
				&frame_name_offset);

		if (ret != 0) {
			//*tag = INVALID_STATE;
			//printf("UNRESOLVED!\n");
			//fflush(stdout);
			continue;
		}

		//printf("%5d: (%d)name = %s\n", target.rank, ret, frame_name);

		// if the frame_name contains substring "MPI"
		if (h_check_start_string(frame_name) == YES)
		{
			*tag = IN_MPI_STATE;
			break;
		} else {
			*tag = OUT_MPI_STATE;
		}

	} while (unw_step(&cursor) > 0);

	h_finish();

	return OK;
}


int h_backtrace2(H_TARGET target, short *tag) {
	int ret;
	int i = 0, j = 0, temp = 0;

	h_initialize(target);

	// pointer for stack frames
	unw_cursor_t cursor;

	// current stack frame's name & offset.
	char frame_name[FRAME_NAME_LEN] = "\0";
	unw_word_t frame_name_offset = 0;

	// obtain the cursor pointing to the current stack frame of given context.
	ret = unw_init_remote(&cursor, addr_space, upt_info);

	if (ret < 0) {
		fprintf(stderr, "Error @ unw_init_remote: %s\n\n", strerror(errno));
		return ERROR;
	}
	
	// debug
	//char file_name[10]; 
	//sprintf(file_name, "%d", target.rank);
	//FILE *fp;
	//static int file_switch = 0;
	//if (0 == file_switch){
	//	fp = fopen(file_name, "w");
	//	file_switch = 1;
	//} else {
	//	fp = fopen(file_name, "a");
	//}

	
	// backtrace to check whether the current process is inside a MPI call
	do {
		// get current stack frame's name, instruction pointer and stack pointer
		ret = unw_get_proc_name(&cursor, frame_name, FRAME_NAME_LEN,
				&frame_name_offset);

		if (ret != 0) {
			//if (ret == UNW_EUNSPEC)
			//	printf("Error code: %d, UNW_EUNSPEC\n", ret);
			//else if (ret == UNW_ENOINFO)
			//	printf("Error code %d, UNW_ENOINFO\n", ret);
			//else if (ret == UNW_ENOMME)
			//	printf("UNW_ENOMME\n");
			//else
			//	printf("Error code %d, UNKNOWN ERROR\n", ret);
			//*tag = INVALID_STATE;
			//printf("UNRESOLVED!\n");
			//fflush(stdout);
			continue;
		}
		
		// debug
		//fprintf(fp, "%s\n", frame_name);

		// if the frame_name contains substring "MPI"
		// 2.0 add: MPI process's stack trace only contain "pthread_spin_init" sometimes when it is 
		// in MPI library
		if (h_check_start_string(frame_name) == YES || strstr(frame_name, "pthread_spin_init") != NULL)
		{
			*tag = IN_MPI_STATE;
			break;
		} else {
			*tag = OUT_MPI_STATE;
		}

	} while (unw_step(&cursor) > 0);

	h_finish();
	
	// debug
	//fputs("\n\n", fp);
	//fclose(fp);

	return OK;
}

int h_backtrace_funcname(H_TARGET target, char *p_func) {
	int ret;

	h_initialize(target);

	// pointer for stack frames
	unw_cursor_t cursor;

	// current stack frame's name & offset.
	char frame_name[FRAME_NAME_LEN] = "\0";
	unw_word_t frame_name_offset = 0;

	// obtain the cursor pointing to the current stack frame of given context.
	ret = unw_init_remote(&cursor, addr_space, upt_info);

	if (ret < 0) {
		fprintf(stderr, "Error @ unw_init_remote: %s\n\n", strerror(errno));
		return ERROR;
	}
	

	// backtrace to check whether the current process is inside a MPI call
	do {
		// get current stack frame's name, instruction pointer and stack pointer
		ret = unw_get_proc_name(&cursor, frame_name, FRAME_NAME_LEN,
				&frame_name_offset);

		if (ret != 0) {
			continue;
		}
		
		// debug
		//fprintf(fp, "%s\n", frame_name);

		// if the frame_name contains substring "MPI"
		// 2.0 add: MPI process's stack trace only contain "pthread_spin_init" sometimes when it is 
		// in MPI library
		if (h_check_start_string(frame_name) == YES || strstr(frame_name, "pthread_spin_init") != NULL)
		{
			strncpy(p_func, frame_name, 30);
		} 
	} while (unw_step(&cursor) > 0);

	h_finish();
	
	return OK;
}
