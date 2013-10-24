#include <stdio.h>

#include "mw.h"
#include "def_structs.h"
#include "linked_list.h"

void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
void kill_slave(int slave);
int get_total_units(mw_work_t ** work_list);
int create_new_slave(mw_work_t *); //return new PID

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{

  DEBUG_PRINT(("master starting"));

  int number_of_slaves;

  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  
  mw_work_t ** work_list;

  double start, end, start_create, end_create, start_results, end_results;

  start = MPI_Wtime();

  DEBUG_PRINT(("creating work list..."));
  start_create = MPI_Wtime();
  work_list = f->create(argc, argv);
  end_create = MPI_Wtime();
  DEBUG_PRINT(("created work in %f seconds!", end_create - start_create));

  int i=0, slave=1, num_work_units=0;

  num_work_units = get_total_units(work_list);

  mw_result_t * received_results =  malloc(f->res_sz * num_work_units);
  if (received_results == NULL)
  {
    fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
    free(received_results);
	exit(0);
  }

  int num_results_received = 0;

  // make array keeping track of pointers for work that's active
  unsigned int assignment_ptrs[number_of_slaves-2];

  // make array of binary indicators for inactive workers
  // initially all workers are active and 0
  unsigned int inactive_workers[number_of_slaves-2];

  // create array of start times
  double assignment_time[number_of_slaves-2];

  // current pointer
  // TODO
  LinkedList* ptr = work_list; 

  // have supervisor so starting at 2
  for(slave=2; slave<number_of_slaves; ++slave)
  {
    DEBUG_PRINT(("assigning work to slave"));

    // TODO
    mw_work_t * work_unit = ptr->data;

    if(work_unit == NULL)
    {
      DEBUG_PRINT(("reached the end of the work, breaking!"));
      break;
    }

    send_to_slave(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);

    // save ptr to assigned work
    assignment_ptrs[slave-2] = ptr;
    
    // save start time
    assignment_time[slave-2] = MPI_Wtime();

    // update ptr
    // TODO
    ptr++;

    DEBUG_PRINT(("work sent to slave"));
  }

  // send time array to supervisor
  MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);

  // failure id
  int failure_id;

  MPI_Status status_fail, status_res;
  MPI_Request request_fail, request_res;
  int flag_fail = 0, flag_res = 0;

  // receive failure from supervisor as non-blocking recv
  MPI_Irecv(&failure_id, 1, MPI_INT, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &request_fail);

  // receive result from workers as non-blocking recv
  MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_res);

  // probe for failures
  MPI_Test(&request_fail, &flag_fail, &status_fail);
  MPI_Test(&request_res, &flag_res, &status_res);

  // send units of work while haven't received all results
  while(num_results_received < num_work_units)
  {
    // send work if have failures or got results
    while(flag_fail || flag_res)
    {
      if (flag_fail)
      {
          // change inactive workers array
          inactive_workers[status_fail.MPI_SOURCE-2] = 1;

          // get work_unit that needs to be reassigned
          mw_work_t* work_unit = assignment_ptrs[failure_id];

          // move failed unit of work to end of work list
          // TODO

          // continue to receive failures from supervisor as non-blocking recv
          MPI_Irecv(&failure_id, 1, MPI_INT, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &request_fail);
      }

      if (flag_res)
      {
        // update number of results received
        num_results_received++;

        // get work_unit
        // TODO
        mw_work_t* work_unit = ptr->data;

        // send new unit of work
        send_to_slave(work_unit, f->work_sz, MPI_CHAR, status.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);        

        // update pointer
        ptr++;

        // update work index for new_pid
        // TODO: update ptr stored in array index
        assignment_ptr[status.MPI_SOURCE-2] = ptr;
        assignment_time[status.MPI_SOURCE-2] = MPI_Wtime();

        // send updated array of times to supervisor
        MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
      }

      // check again for failure or result
      MPI_Test(&request_fail, &flag_fail, &status_fail);
      MPI_Test(&request_res, &flag_res, &status_res);
    }
  }

  // send kill signal to other processes, including supervisor
  for(slave=1; slave<number_of_slaves; ++slave)
  {
    DEBUG_PRINT(("Murdering slave"));
    kill_slave(slave);
  }
        
  start_results = MPI_Wtime();
  int err_code = f->result(num_results_received, received_results);
  end_results = MPI_Wtime();

  end = MPI_Wtime();
  
  printf("all %f s\n", end-start);
  printf("create %f s\n", end_create-start_create);
  printf("process %f s\n", end_results-start_results);
}


void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
{
  MPI_Send(work, size, datatype, slave, tag, comm);
  DEBUG_PRINT(("Sent!"));
}

int get_total_units(mw_work_t ** work_list)
{
  mw_work_t ** work_unit_counter = work_list;

  while(*work_unit_counter != NULL)
  work_unit_counter++;
  
  return work_unit_counter - work_list;
}

void kill_slave(int slave)
{
  MPI_Send(0, 0, MPI_CHAR, slave, KILL_TAG, MPI_COMM_WORLD);
}

int create_new_slave(mw_work_t * work)
{
}
