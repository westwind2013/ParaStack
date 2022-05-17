#include "map.h"

void h_map(int target_num, int rank_base, H_TARGET *target_list) {

  int i;

  for (i = 0; i < target_num; i++) {
    // rank increases as process id does
    target_list[i].rank = rank_base + i;
  }
}

#ifdef H_RAND_CHECK

void h_map_rand(int *tool_info, H_TARGET *target_list,
    int *local_rand_num, H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2]) {

  int i, j, k;

  // map random ranks to corresponding process ids according to target_list
  for (k = 0; k < 2; k++) {
    for (i = 0; i < local_rand_num[k]; i++) {
      for (j = 0; j < tool_info[0]; j++) {
        if (local_rand_tasks[k][i].rank == target_list[j].rank)
          local_rand_tasks[k][i].proc_id = target_list[j].proc_id;
      }
    }
  }
}


void h_generate_rand_tasks(int tool_rank, int tool_size, int *tool_info,
    int *local_rand_num, H_TARGET local_rand_tasks[][RAND_TASKS_NUM/2]) {

  int i, j, k;

  // a list of random numbers standing for random ranks
  int *rand_lists = malloc(sizeof(int) * RAND_TASKS_NUM);

  if (tool_rank == 0) {
    h_generate_rand_list(tool_info[2], RAND_TASKS_NUM, rand_lists);
  }

  // broadcast this random list
  MPI_Bcast (rand_lists, RAND_TASKS_NUM, MPI_INT, 0, MPI_COMM_WORLD);

  // h_check(tool_rank, rand_lists, RAND_TASKS_NUM/2);

  // h_barrier_check (tool_rank, tool_size, MPI_COMM_WORLD, rand_lists, RAND_TASKS_NUM);

  // mark random tasks locally
  for (k = 0; k < 2; k++) {
    for (i = 0; i < RAND_TASKS_NUM / 2; i++) {
      if (rand_lists[2*i+k] < tool_info[1])
        continue;
      if (rand_lists[2*i+k] >= tool_info[1] + tool_info[0])
        break;

      local_rand_tasks[k][local_rand_num[k]].rank = rand_lists[2*i+k];
      local_rand_num[k] += 1;
    }
  }

  free(rand_lists);
  //	h_barrier_check_target (tool_rank, tool_size, MPI_COMM_WORLD, local_rand_tasks, *local_rand_num);
}

void h_comm_split(int tool_rank, int *local_rand_num,
    int *color,
    MPI_Comm *new_comm, int *tool_rank_new, int *tool_size_new) {

  if (local_rand_num[0] == 0 && local_rand_num[1] == 0) {
    *color = OTHER_COLOR;
    //		free(local_rand_tasks);
  }
  else {
    *color = RAND_COLOR;
  }

  MPI_Comm_split (MPI_COMM_WORLD, *color, tool_rank, new_comm);
  MPI_Comm_rank (*new_comm, tool_rank_new);
  MPI_Comm_size (*new_comm, tool_size_new);

  //	printf("%5d%5d%5d\n", tool_rank, *tool_rank_new, *tool_size_new);
}

#endif
