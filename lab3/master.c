#include <stdio.h>
#include <assert.h>

#include "mw.h"
#include "def_structs.h"
#include "linked_list.h"

void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
void kill_slave(int slave);
int get_total_units(mw_work_t ** work_list);

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{

  DEBUG_PRINT(("master starting"));

  int number_of_slaves;

  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  
  LinkedList * work_list;

  double start, end, start_create, end_create, start_results, end_results;

  start = MPI_Wtime();

  DEBUG_PRINT(("creating work list..."));
  start_create = MPI_Wtime();
  work_list = listFromArray(f->create(argc, argv));
  end_create = MPI_Wtime();
  DEBUG_PRINT(("created work in %f seconds!", end_create - start_create));

  int slave=1, num_work_units=0;

  num_work_units = list_length(work_list);

  mw_result_t * received_results =  malloc(f->res_sz * num_work_units);
  if (received_results == NULL)
  {
    fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
	exit(0);
  }

  int num_results_received = 0;

  // make array keeping track of pointers for work that's active
  LinkedList* assignment_ptrs[number_of_slaves-2];

  // make array of binary indicators for inactive workers
  // initially all workers are active and 0
  //unsigned int inactive_workers[number_of_slaves-2];

  // create array of start times
  double assignment_time[number_of_slaves-2];

  // current pointer
  LinkedList
    * next_work_node = work_list,
    * list_end = NULL;

  // have supervisor so starting at 2
  for(slave=2; slave<number_of_slaves; ++slave)
  {
    DEBUG_PRINT(("assigning work to slave"));

    if(next_work_node == NULL)
    {
      DEBUG_PRINT(("reached the end of the work, breaking!"));
      break;
    }

    mw_work_t * work_unit = next_work_node->data;

    send_to_slave(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);

    // save next_work_node to assigned work
    assignment_ptrs[slave-2] = next_work_node;
    assert(assignment_ptrs[slave-2] != NULL);
    
    // save start time
    assignment_time[slave-2] = MPI_Wtime();

    // update next_work_node
    if(next_work_node->next == NULL)
    {
        list_end = next_work_node;
    }
    next_work_node=next_work_node->next;

    DEBUG_PRINT(("work sent to slave"));
  }

  // send time array to supervisor
  DEBUG_PRINT(("Sending supervisor first time update"));
  MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);

  // failure id
  int failure_id;

  MPI_Status status_fail, status_res;
  MPI_Request request_fail, request_res;
  int flag_fail = 0, flag_res = 0;

  // receive failure from supervisor as non-blocking recv
  MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);

  // receive result from workers as non-blocking recv
  MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_res);

  // send units of work while haven't received all results
  while(num_results_received < num_work_units)
  {
    // check for flag_fail again
    MPI_Test(&request_fail, &flag_fail, &status_fail);

    // check for flag_res again
    MPI_Test(&request_res, &flag_res, &status_res);
    
    // send work if have failures or got results
    if (flag_fail)
    {
        // change inactive workers array
        //inactive_workers[status_fail.MPI_SOURCE-2] = 1;
        DEBUG_PRINT(("received failure from supervisor"));

        // get work_unit that needs to be reassigned
        LinkedList * work_unit = assignment_ptrs[failure_id];

        if(work_unit == NULL)
        {
            DEBUG_PRINT(("failed work unit is NULL?"));
        }
        else if(next_work_node == NULL && work_unit != NULL && work_unit->next == NULL)
        {
            next_work_node = work_unit;
        }
        else
        {
            move_node_to_end(work_unit);
            assignment_ptrs[failure_id] = NULL;
        }

        flag_fail = 0;

        // continue to receive failures from supervisor as non-blocking recv
        MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);
    }

    if (flag_res)
    {
      // update number of results received
      num_results_received++;

      //DEBUG_PRINT(("Received result %d", num_results_received));

      if(next_work_node == NULL && list_end != NULL && list_end->next != NULL)
      {
          next_work_node = list_end->next;
          list_end = NULL;
      }
      if(next_work_node != NULL)
      {
          // get work_unit
          mw_work_t* work_unit = next_work_node->data;

          // send new unit of work
          send_to_slave(work_unit, f->work_sz, MPI_CHAR, status_res.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);        

          // update pointer
          if(next_work_node->next == NULL)
          {
              list_end = next_work_node;
          }

          if(next_work_node != NULL)
          {
              // update work index for new_pid
              assignment_ptrs[status_res.MPI_SOURCE-2] = next_work_node;
              assert(assignment_ptrs[status_res.MPI_SOURCE-2] != NULL);
              assignment_time[status_res.MPI_SOURCE-2] = MPI_Wtime();

              // send updated array of times to supervisor
              MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
              DEBUG_PRINT(("SENT TIME TO SUP"));
          }
          next_work_node = next_work_node->next;
      }

      // continue to receive results from workers as non-blocking recv
      MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &request_res);      
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
  
  DEBUG_PRINT(("all %f s\n", end-start));
  DEBUG_PRINT(("create %f s\n", end_create-start_create));
  DEBUG_PRINT(("process %f s\n", end_results-start_results));
}


void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
{
  DEBUG_PRINT(("Sent! %d", slave));
  MPI_Send(work, size, datatype, slave, tag, comm);
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
