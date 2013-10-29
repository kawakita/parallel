#include <assert.h>

#include "mw.h"
#include "def_structs.h"
#include "linked_list.h"
#include <time.h>
#include <stdlib.h>
#include <limits.h>

void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
void kill_slave(int slave);
int get_total_units(mw_work_t ** work_list);
int random_fail();
int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int rank);

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{

  DEBUG_PRINT(("master starting"));

  int number_of_slaves;

  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);

  // Stuff for F_Send
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  DEBUG_PRINT(("Seeded srand with %u", (unsigned) time(NULL) + rank));
  srand((unsigned)time(NULL) + rank);

  LinkedList * work_list;

  double start, end, start_create, end_create, start_results, end_results;

  start = MPI_Wtime();

  DEBUG_PRINT(("creating work list..."));
  start_create = MPI_Wtime();
  // save work_array separately so we can find index later on
  mw_work_t ** work_array = f->create(argc, argv);
  work_list = listFromArray(work_array);
  end_create = MPI_Wtime();
  DEBUG_PRINT(("created work in %f seconds!", end_create - start_create));

  int slave=1, num_work_units=0;

  num_work_units = get_total_units(work_list);

  mw_result_t * received_results =  malloc(f->res_sz * num_work_units);
  if (received_results == NULL)
  {
    fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
    exit(0);
  }

  int num_results_received = 0;

  // make array keeping track of pointers for work that's active
  LinkedList* assignment_ptrs[number_of_slaves-2];

  // make array keeping track of indices for work that's active
  int assignment_indices[number_of_slaves-2];

  // make array of binary indicators for inactive workers
  // initially all workers are active and 0
  //unsigned int inactive_workers[number_of_slaves-2];

  // create array of start times
  double assignment_time[number_of_slaves-2];

  int are_you_down[number_of_slaves-2];

  // current pointer
  LinkedList
    * next_work_node = work_list,
    * list_end = NULL;

  // have supervisor so starting at 2
  for(slave=2; slave<number_of_slaves; ++slave)
  {
    are_you_down[slave-2] = 0; //slaves are all working in the beginning
    DEBUG_PRINT(("assigning work to slave"));

    if(next_work_node == NULL)
    {
      DEBUG_PRINT(("reached the end of the work, breaking!"));
      break;
    }

    mw_work_t * work_unit = work_array[next_work_node->index];

    send_to_slave(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);

    // save next_work_node to assigned work
    assignment_indices[slave-2] = next_work_node->index;
    assignment_ptrs[slave-2] = next_work_node;
    assert(assignment_indices[slave-2] != -1);
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
  //F_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD, rank);

  // failure id
  int failure_id;

  MPI_Status status_fail, status_res;
  MPI_Request request_fail, request_res;
  int flag_fail = 0, flag_res = 0;

  // receive failure from supervisor as non-blocking recv
  MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);

  // receive result from workers as non-blocking recv
  MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_res);
  
  int ping_sup = 0;

  // send units of work while haven't received all results
  while(num_results_received < num_work_units)
  {
    // send ping to supervisor
    //F_Send(&ping_sup, 1, MPI_INT, 1, M_PING_TAG, MPI_COMM_WORLD, rank);
    MPI_Send(&ping_sup, 1, MPI_INT, 1, M_PING_TAG, MPI_COMM_WORLD);

    // check for flag_fail again
    MPI_Test(&request_fail, &flag_fail, &status_fail);

    // check for flag_res again
    MPI_Test(&request_res, &flag_res, &status_res);
    
    // send work if have failures or got results
    if (flag_fail)
    {
      // change inactive workers array
      //inactive_workers[status_fail.MPI_SOURCE-2] = 1;
      DEBUG_PRINT(("received failure from supervisor, process %d", failure_id));

      // get work_unit that needs to be reassigned
      LinkedList * work_unit = assignment_ptrs[failure_id];

      if(work_unit != NULL)
      {
        DEBUG_PRINT(("Moving assignment at %p to end of the queue", work_unit));
        move_node_to_end(work_unit);
        if(next_work_node == NULL)
        {
          next_work_node = work_unit;
        }
        assert(next_work_node != NULL);
      }
      if(assignment_indices[failure_id] == -1)
      {
        DEBUG_PRINT(("Failure on idle process %d. WTF??", failure_id));
      } 
      if(assignment_time[failure_id] == 0.0)
      {
        DEBUG_PRINT(("Failure on idle process %d. WTF??", failure_id));
      }
      if(are_you_down[failure_id] == 1)
      {
        DEBUG_PRINT(("Failure on a process which is already failed. WTF??"));
      }
      are_you_down[failure_id] = 1; //this slave is considered dead :(
      assignment_indices[failure_id] = -1;
      assignment_ptrs[failure_id] = NULL;
      assignment_time[failure_id] = 0.0;
      MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
      flag_fail = 0;
      // continue to receive failures from supervisor as non-blocking recv
      MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);
    }
    
    int idle_process = -1, i;
    for(i=0; i<number_of_slaves-2; ++i)
    {
      if(assignment_time[i] == 0.0 && !are_you_down[i])
      {
        idle_process = i;
        break;
      }
    }

    if(next_work_node != NULL && idle_process > -1)
    {
      send_to_slave(work_array[next_work_node->index], f->work_sz, MPI_CHAR, idle_process+2, WORK_TAG, MPI_COMM_WORLD);
      assignment_ptrs[idle_process] = next_work_node;
      assignment_time[idle_process] = MPI_Wtime();
      MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
      DEBUG_PRINT(("Gave an assignment to previously idle process %d, assignment at %p", idle_process, next_work_node));
      if(next_work_node->next == NULL)
      {
        list_end = next_work_node;
      }
      next_work_node = next_work_node->next;
    }

    if (flag_res)
    {
      int worker_number = status_res.MPI_SOURCE-2;
      if(!are_you_down[worker_number]) //If this slave is marked dead, just ignore him
      {
        // save index and result received to file
        FILE * fptr;
        fptr = fopen("recovery.txt", "w");
        int index = assignment_indices[worker_number];
        //char *s = f->result_to_str(received_results[num_results_received]);
        DEBUG_PRINT(("str %f", received_results[num_results_received].k));
        fprintf(fptr, "%d\n", index);
        fclose(fptr);

        // update number of results received
        num_results_received++;

        if(next_work_node == NULL && list_end != NULL && list_end->next != NULL)
        {
          DEBUG_PRINT(("Found more work to do, now an idle process can get an assignment"));
          next_work_node = list_end->next;
          list_end = NULL;
        }
        if(next_work_node != NULL)
        {
          // get work_unit
          mw_work_t* work_unit = work_array[next_work_node->index];

          // send new unit of work
          F_Send(work_unit, f->work_sz, MPI_CHAR, status_res.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD, rank);        
          //send_to_slave(work_unit, f->work_sz, MPI_CHAR, status_res.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);        

          // update pointer
          if(next_work_node->next == NULL)
          {
            list_end = next_work_node;
          }

          // update work index for new_pid
          assignment_ptrs[status_res.MPI_SOURCE-2] = next_work_node;
          assert(assignment_ptrs[status_res.MPI_SOURCE-2] != NULL);
          assignment_indices[status_res.MPI_SOURCE-2] = next_work_node->index;
          assert(assignment_indices[status_res.MPI_SOURCE-2] != -1);
          assignment_time[status_res.MPI_SOURCE-2] = MPI_Wtime();
          // send updated array of times to supervisor
          MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
          DEBUG_PRINT(("SENT TIME TO SUP"));
          next_work_node = next_work_node->next;
          if(next_work_node == NULL)
          {
            DEBUG_PRINT(("Reached the end of the work list, should get idle processors after this"));
          }
        }
        else
        {
          DEBUG_PRINT(("Worker %d is now idle, I ain't got shit for him to do", worker_number));
          assignment_time[worker_number] = 0.0;
          assignment_ptrs[worker_number] = NULL;
          assignment_indices[worker_number] = -1;
          assert(!are_you_down[worker_number]);
          MPI_Send(assignment_time, number_of_slaves-2, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
        }
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

/*
int random_fail()
{
  float r = ((float) rand())/RAND_MAX;
  return r > p;
}

int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int rank)
{
  if (rank == 0 || random_fail()) {      
    DEBUG_PRINT(("%d FAIIIIILLLLLL!!!!!!", rank));
    MPI_Finalize();
    exit (0);
    return 0;
  } else {
  return MPI_Send (buf, count, datatype, dest, tag, comm);
  }
}*/
