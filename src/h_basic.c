#include "h_basic.h"

// the length of substring needs to be searched
#define CMP_LEN 4

// the substring
char cmp[CMP_LEN] = "MPI_";
char _cmp[CMP_LEN] = "mpi_";

void h_msleep(unsigned int msecs) {
  usleep(msecs * 1000);
}

int h_check_start_string(char *frame_name) {
  int i = 0;

  // check if the frame_name contains keywords "MPI"
  int temp = strlen(frame_name);
  temp = temp < FRAME_NAME_LEN ? temp : FRAME_NAME_LEN;

  if (temp < CMP_LEN + 1)
    return NO;

  if (frame_name[0] == 'P' || frame_name[0] =='p') {
    for (i = 0; i < CMP_LEN; i++) {
      if (frame_name[i+1] != cmp[i] && frame_name[i+1] != _cmp[i])
        break;
    }
  } else if (frame_name[0] != 'M' || frame_name[0] !='m') {
    for (i = 1; i < CMP_LEN; i++) {
      if (frame_name[i] != cmp[i] && frame_name[i] != _cmp[i])
        break;
    }
  }

  if (i == CMP_LEN) 
    return YES;
  else
    return NO;
}


int h_check_string(char *frame_name) {
  int i = 0, j = 0;

  // check if the frame_name contains keywords "MPI"
  int temp = strlen(frame_name);
  temp = temp < FRAME_NAME_LEN ? temp : FRAME_NAME_LEN;

  if (temp < CMP_LEN)
    return NO;

  // compare strings
  for (i = 0; i < temp - CMP_LEN; i++) {

    for (j = 0; j < CMP_LEN; j++) {
      if (frame_name[i + j] != cmp[j] && frame_name[i + j] != _cmp[j])
        break;
    }
    if (j == CMP_LEN) {
      return YES;
    }
  }

  return NO;
}


void h_generate_rand_list(int max_rand, int num, int *rand_list) {

  int i, j;
  int temp;

  // repetition judging tag
  int tag = 0;
  struct timeval tmp;
  gettimeofday(&tmp, NULL);
  srand(tmp.tv_usec);

  for(i = 0; i < num; i++) {

    // reset repetition judging tag
    tag = 0;

    // generate
    temp = rand() % max_rand;

#ifdef PHASE_DETECTION
    // overrides the randome number
    // fix the first randome number as 0 for phase detection
    if (i == 0)
      temp = 0;
#endif
    // judge repetition
    for(j = 0; j < i; j++) {
      // jump out on repetition
      if(temp == rand_list[j]) {
        tag = 1;
        break;
      }
    }

    // count as one random number only when no repetition exists
    if(tag == 0)
      rand_list[i] = temp;
    else
      i--;
  }

  h_bubble_sort(rand_list, num);
}

void h_bubble_sort(int *rand_list, int num) {

  int i, j;
  int temp;

  // num-1 times of scanning
  for (i = 0; i < num - 1; i++) {
    for (j = 0; j < num - 1 - i; j++) {
      if (rand_list[j] > rand_list[j + 1]) {
        temp = rand_list[j];
        rand_list[j] = rand_list[j + 1];
        rand_list[j + 1] = temp;
      }
    }
  }
}


void h_check(int tool_rank, int *array, int num) {

  // index
  int j;

  if (num != 0) {

    printf("Rank %5d: ", tool_rank);

    for(j = 0; j < num; j++) {
      printf("%5d", array[j]);
    }
    printf("\n");
  }
}


void h_check_target(int tool_rank, H_TARGET *array, int num) {

  // index
  int j;

  if (num != 0) {
    printf("Rank %5d: ", tool_rank);

    for(j = 0; j < num; j++) {
      printf("%5d", array[j].rank);
    }
    printf("\n");
  }
}


void h_claim_start (int tool_rank) {

  if (tool_rank == 0) {
    printf("\n*********************************\n");
    printf("*********Diagnose Begins*********\n");
    printf("*********************************\n\n");
  }

}

void h_claim_exit (int tool_rank, int check_ret) {

  MPI_Barrier(MPI_COMM_WORLD);

  if (tool_rank == 0) {

    if (check_ret == OK) {
      printf("\nThe program finished successfully\n");
    }

    printf("\n*******************************\n");
    printf("*********Diagnose Ends*********\n");
    printf("*******************************\n");
  }
}

void h_claim_rand_info (int tool_rank, int tool_size,
    int *tool_info, int *local_rand_num,
    H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2]) {

  int i, k;

  // array used to record mpi rank
  int *tmp_tasks = (int *)malloc(sizeof(int)*tool_info[0]);

  for (k = 0; k < 2; k++) {
    if (tool_rank == 0) {

      printf("-------Random target tasks-------\n\n");

      // record how many random tasks each tool task control
      int *tmp = (int *)malloc(sizeof(int)*tool_size);
      tmp[0] = local_rand_num[k];

      // receive from all other tasks except for itself
      for (i = 1; i < tool_size; i++) {
        MPI_Recv(tmp+i, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }

      // tool 0 printing
      h_check_target (tool_rank, local_rand_tasks[k], tmp[0]);
      // receive from all other tool ranks
      for(i = 1; i < tool_size; i++) {
        if (tmp[i] != 0) {
          MPI_Recv (tmp_tasks, tmp[i], MPI_INT, i, 0, MPI_COMM_WORLD,
              MPI_STATUS_IGNORE);
          h_check(i, tmp_tasks, tmp[i]);
        }
      }
      // free resouces only belong to rank 0
      free (tmp);
      printf("\n-------Random target tasks-------\n\n");

    } else {
      // send to process 0 its number of random tasks
      MPI_Send (local_rand_num + k, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

      if (local_rand_num[k] != 0) {

        // prepare for the sending
        for (i = 0; i < local_rand_num[k]; i++)
          tmp_tasks[i] = local_rand_tasks[k][i].rank;

        // send the random tasks' ranks to tool 0
        MPI_Send (tmp_tasks, local_rand_num[k], MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
    }
  }

  // free resouces allocated inside this function
  free (tmp_tasks);
}
