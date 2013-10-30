#include <assert.h>
#include "mw.h"
#include "def_structs.h"
#include "linked_list.h"

void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
void kill_slave(int slave);
int get_total_units(mw_work_t ** work_list);

#define DEBUG 1

void do_backupmaster_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
  DEBUG_PRINT(("backupmaster starting"));

  int number_of_nonslaves = 3;

  int number_of_slaves;
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  number_of_slaves = number_of_slaves - number_of_nonslaves;

  // needed for F_Send
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  DEBUG_PRINT(("Seeded srand with %u", (unsigned) time(NULL) + rank));
  srand((unsigned)time(NULL) + rank);
  
  LinkedList * work_list;

  double start, end, start_create, end_create, start_results, end_results;

  start = MPI_Wtime();
  
  //alternate TA needs to know all the work
  DEBUG_PRINT(("alternate TA creating work list..."));
  start_create = MPI_Wtime();

  // save work_array separately so we can find index later on
  mw_work_t ** work_array = f->create(argc, argv);
  work_list = listFromArray(work_array);
  end_create = MPI_Wtime();
  DEBUG_PRINT(("alternate TA created work in %f seconds!", end_create - start_create));

  int slave=1, num_work_units=0;

  num_work_units = get_total_units(work_array);
  
  //alternate TA needs to have all the results
  mw_result_t * received_results = malloc(f->res_sz * num_work_units);
  if (received_results == NULL)
  {
    fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
    exit(0);
  }

  int num_results_received = 0;

  // make array keeping track of pointers for work that's active
  LinkedList* assignment_ptrs[number_of_slaves];

  // create array of start times
  double* assignment_time = malloc(number_of_slaves*sizeof(double));

  // create array indicating if slaves are down
  int are_you_down[number_of_slaves];

  // current pointer
  LinkedList
    * next_work_node = work_list,
    * list_end = NULL;

  // get setup to recv START_TAG from supervisor in event of master failure
  MPI_Status status_master_fail;
  MPI_Request request_master_fail;
  int flag_master_fail = 0;
  
  //professor may need me to take over the homework tracking 
  //would only be received once
  MPI_Irecv(assignment_time, number_of_slaves, MPI_DOUBLE, 1, START_TAG, MPI_COMM_WORLD, &request_master_fail);

  // have supervisor so starting at number_of_nonslaves
  for(slave=number_of_nonslaves; slave<(number_of_slaves+number_of_nonslaves); ++slave)
  {
    if(!flag_master_fail) MPI_Test(&request_master_fail, &flag_master_fail, &status_master_fail);

    are_you_down[slave-number_of_nonslaves] = 0; //slaves are all working in the beginning
    //DEBUG_PRINT(("assigning work to slave"));

    if(next_work_node == NULL)
    {
      DEBUG_PRINT(("reached the end of the work, breaking!\n"));
      break;
    }

    mw_work_t * work_unit = next_work_node->data;

    if (flag_master_fail)
    {
      send_to_slave(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);
      DEBUG_PRINT(("work sent to slave\n"));
    }
    // save next_work_node to assigned work
    assignment_ptrs[slave-number_of_nonslaves] = next_work_node;
    assert(assignment_ptrs[slave-number_of_nonslaves] != NULL);
    
    // save start time
    //assignment_time[slave-number_of_nonslaves] = MPI_Wtime();

    // update next_work_node
    if(next_work_node->next == NULL)
    {
        list_end = next_work_node;
    }
    next_work_node=next_work_node->next;
    
    //MPI_Irecv(assignment_time, 1, MPI_INT, 1, START_TAG, MPI_COMM_WORLD, &request_master_fail);
  }

   
  // send time array to supervisor  
  if (flag_master_fail) 
  {
    MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
    DEBUG_PRINT(("Sending supervisor first time update\n"));
  }

  // failure id
  int failure_id, kill_signal;

  MPI_Status status_fail, status_res, status_kill;
  MPI_Request request_fail, request_res, request_kill;
  int flag_fail = 0, flag_res = 0, flag_kill = 0;

  // receive failure from supervisor as non-blocking recv
  MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);

  // receive result from workers as non-blocking recv
  MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_res);

  // receive kill from supervisor as non-blocking recv
  MPI_Irecv(&kill_signal, 1, MPI_INT, 1, KILL_TAG, MPI_COMM_WORLD, &request_kill);

  int ping_sup = 0;

  // send units of work while haven't received all results
  while(num_results_received < num_work_units)
  {
    // check for flag_fail
    MPI_Test(&request_fail, &flag_fail, &status_fail);

    // check for flag_res
    MPI_Test(&request_res, &flag_res, &status_res);

    // check for flag_kill
    MPI_Test(&request_kill, &flag_kill, &status_kill);

    // check for flag_master_fail
    MPI_Test(&request_master_fail, &flag_master_fail, &status_master_fail);

    // send ping to supervisor
    //if (flag_master_fail) MPI_Send(&ping_sup, 1, MPI_INT, 1, M_PING_TAG, MPI_COMM_WORLD);
    
    // send work if have failures or got results
    if (flag_fail)
    {
        DEBUG_PRINT(("received failure from supervisor, process %d\n", failure_id));

        // get work_unit that needs to be reassigned
        LinkedList * work_unit = assignment_ptrs[failure_id];

        if(work_unit != NULL)
        {
            DEBUG_PRINT(("Moving assignment at %p to end of the queue\n", work_unit));
            move_node_to_end(work_unit);
            if(next_work_node == NULL)
            {
                next_work_node = work_unit;
            }
            assert(next_work_node != NULL);
        }

        /*if(assignment_time[failure_id] == 0.0)
        {
            DEBUG_PRINT(("Failure on idle process %d. WTF??", failure_id));
        }
        if(are_you_down[failure_id] == 1)
        {
            DEBUG_PRINT(("Failure on a process which is already failed. WTF??"));
        }*/

        are_you_down[failure_id] = 1; //this slave is considered dead :(
        assignment_ptrs[failure_id] = NULL;
        assignment_time[failure_id] = 0.0;

        if (flag_master_fail) MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
        
        flag_fail = 0;
        // continue to receive failures from supervisor as non-blocking recv
        MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);
    }
    
    int idle_process = -1, i;
    for(i=0; i<number_of_slaves; ++i)
    {
        if(assignment_time[i] == 0.0 && !are_you_down[i])
        {
            idle_process = i;
            break;
        }
    }

    if(next_work_node != NULL && idle_process > -1)
    {
        if (flag_master_fail)
        {
          send_to_slave(next_work_node->data, f->work_sz, MPI_CHAR, idle_process+number_of_nonslaves, WORK_TAG, MPI_COMM_WORLD);
          assignment_ptrs[idle_process] = next_work_node;
          assignment_time[idle_process] = MPI_Wtime();
          MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
          DEBUG_PRINT(("alt TA gave an assignment to previously idle process %d, assignment at %p\n", idle_process, next_work_node));
        }
        
        if(next_work_node->next == NULL)
        {
            list_end = next_work_node;
        }
        next_work_node = next_work_node->next;
    }
    
    //DEBUG_PRINT(("alternative TA checking if students sent assignments"));
    if (flag_res)
    {
      int worker_number = status_res.MPI_SOURCE-number_of_nonslaves;
      
      if(!are_you_down[worker_number]) //If this slave is marked dead, just ignore him
      {
        // update number of results received
        num_results_received++;

        if(next_work_node == NULL && list_end != NULL && list_end->next != NULL)
        {
            DEBUG_PRINT(("Found more work to do, now an idle process can get an assignment\n"));
            next_work_node = list_end->next;
            list_end = NULL;
        }
        if(next_work_node != NULL)
        {
          // get work_unit
          mw_work_t* work_unit = next_work_node->data;

          // send new unit of work
          if (flag_master_fail)
            send_to_slave(work_unit, f->work_sz, MPI_CHAR, status_res.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);        

          // update pointer
          if(next_work_node->next == NULL)
          {
              list_end = next_work_node;
          }

          // update work index for new_pid
          assignment_ptrs[status_res.MPI_SOURCE-number_of_nonslaves] = next_work_node;
          assert(assignment_ptrs[status_res.MPI_SOURCE-number_of_nonslaves] != NULL);
          assignment_time[status_res.MPI_SOURCE-number_of_nonslaves] = MPI_Wtime();
          // send updated array of times to supervisor
          if (flag_master_fail) 
          {
            MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
            DEBUG_PRINT(("SENT TIME TO SUP\n"));
          }
          
          next_work_node = next_work_node->next;
          if(next_work_node == NULL)
          {
              DEBUG_PRINT(("Reached the end of the work list, should get idle processors after this\n"));
          }
        }
        else
        {
            
//            assignment_time[worker_number] = 0.0;
            assignment_ptrs[worker_number] = NULL;
            assert(!are_you_down[worker_number]);
            if (flag_master_fail) 
            {
              MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
              DEBUG_PRINT(("Worker %d is now idle, I ain't got shit for him to do\n", worker_number));
            }
        }
      }
      
      // continue to receive results from workers as non-blocking recv
      
      if(num_results_received<num_work_units) MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &request_res);      
      DEBUG_PRINT(("%d is num_results_received\n", num_results_received));
    }

    if (flag_kill)
    {
      DEBUG_PRINT(("alt TA done\n"));
      return;
    }
  }

  // send kill signal to other processes, including supervisor

  if (flag_master_fail)
  {
    DEBUG_PRINT(("Murdering slave"));
    kill_slave(1);
    for(slave=3; slave<number_of_slaves+number_of_nonslaves-1; ++slave)
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
}


/*void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
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
}*/
