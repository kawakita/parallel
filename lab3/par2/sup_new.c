#include <stdio.h>
#include <math.h>
#include <string.h>

#include "mw.h"
#include "def_structs.h"
#include "linked_list.h"

#define DEBUG 1

void do_supervisor_as_master_stuff(int argc, char ** argv, struct mw_api_spec *f);

void do_supervisor_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
  
  DEBUG_PRINT(("supervisor starting"));
  
  int number_of_slaves;
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  number_of_slaves = number_of_slaves - 2;
  
  MPI_Status status, status1, status2;
  MPI_Request request1, request2;
  
  // keep track of start times
  double * assignment_time1 = malloc(sizeof(double)*number_of_slaves);
  double * assignment_time2 = malloc(sizeof(double)*number_of_slaves);

  /** accomodating master failure **/
  double master_threshold = 1; //give a lot of time to create work
  double last_master_ping = MPI_Wtime();
  
  // supervisor does nonblocking receive for first set of assignment times
  // this follows the initial assignment of work
  MPI_Request request_started;
  MPI_Status status_started;
  int flag_started = 0;

  // set bool for master failing
  int master_fail = 0;

  DEBUG_PRINT(("We're here!"));
  // supervisor does blocking receive to get list of workers and their start times
  MPI_Recv(assignment_time1, number_of_slaves, MPI_DOUBLE, 0, SUPERVISOR_TAG, MPI_COMM_WORLD, &request_started);
  while(flag_started == 0 && MPI_Wtime()<(last_master_ping+master_threshold))
  {
    MPI_Test(&request_started, &flag_started, &status_started);
    if(flag_started) last_master_ping = MPI_Wtime();
  }
  // if we pass the threshold time consider master failed
  if(!flag_started) 
  {
    DEBUG_PRINT(("SUPERVISOR: MASTER FAILED before assigning work\n"));
    master_fail = 1;
    do_supervisor_as_master_stuff(argc, argv, f);
  }
  /** end accomodating master failure **/

  // determine how long each worker took
  double * complete_time = malloc(sizeof(double)*number_of_slaves);
  // booleans for failure
  int * failed_worker = calloc(sizeof(int), number_of_slaves);

  // supervisor does blocking receive to get list of workers and their start times
  MPI_Recv(assignment_time1, number_of_slaves, MPI_DOUBLE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  //assignment_time2 = assignment_time1;

  // calc approximate time diff between sup and master
  double master_time = assignment_time1[number_of_slaves-1];
  double mytime_off_by = MPI_Wtime() - master_time;

  //DEBUG_PRINT(("supervisor knows when the workers started"));

  int units_received = 0;
  int i;
  int kill_msg, flag1, received_update;
  
  double tot_time=0, sq_err=0, mean=0, stddev=0, threshold=0;
  
  //set up receivers for kill and master updates
  MPI_Irecv(&kill_msg, 1, MPI_INT, 0, KILL_TAG, MPI_COMM_WORLD, &request1);
  MPI_Irecv(assignment_time2, number_of_slaves, MPI_DOUBLE, 0, SUPERVISOR_TAG, MPI_COMM_WORLD, &request2);

  //waiting for updates on start times from master
  while(1) 
  {
    //kill myself if the master says so  
    MPI_Test(&request1, &flag1, &status1);
    if(flag1)
    {
      //DEBUG_PRINT(("SUPERVISOR SUICIDE!"));
      return;
    }
    
    //get a new start time array from master
    MPI_Test(&request2, &received_update, &status2);
    if(received_update)
    {
      //DEBUG_PRINT(("got an update from master"));
    }
    
    //DEBUG_PRINT(("supervisor is about to check if the slaves are different"));

    int found_change = 0;
    //check for differences in working slaves
    for(i=0; i<number_of_slaves; i++) 
    {
      if(failed_worker[i] != 0)
      {
        continue;
      }
      if(assignment_time1[i] == 0.0 && assignment_time2[i] != 0.0)
      {
        //a previously idle worked got assigned something
        assignment_time1[i] = assignment_time2[i];
        found_change = 1;
        DEBUG_PRINT(("Worker %d just got off his lazy ass", i));
        continue;
      }
      if(assignment_time2[i] == 0.0)
      {
        //worker is currently idle, not dead
        if(assignment_time1[i] != 0.0)
        {
          DEBUG_PRINT(("Just found out %d is idle", i));
          assignment_time1[i] = 0.0;
          found_change = 1;
        }
        continue;
      }

      //we have a good worker!
      if(assignment_time1[i] != assignment_time2[i] && received_update)
      {
        //DEBUG_PRINT(("supervisor is impressed by his good worker %d", i));
        complete_time[i] = assignment_time2[i] - assignment_time1[i];
        units_received++;
        tot_time += complete_time[i];
        mean = tot_time/units_received;
        sq_err += pow(complete_time[i] - mean, 2);
        stddev = sqrt(sq_err/units_received);
        //we have enough data to update threshold
        if(units_received >= number_of_slaves/2)
        {
          //DEBUG_PRINT(("the stddev is %f", stddev));
          threshold = mean + 10*stddev + 0.1;
          //DEBUG_PRINT(("the threshold is %f", threshold));
        }
        assignment_time1[i] = assignment_time2[i];
        found_change = 1;
        
      }
      //if we have enough data, we can tell if we have a bad worker :(
      else if(threshold>0 && assignment_time1[i]==assignment_time2[i] && MPI_Wtime() - mytime_off_by - assignment_time1[i] > threshold)
      {
        assert(assignment_time1[i] != 0.0);
        DEBUG_PRINT(("methinks someone is slacking %d", i));
        MPI_Send(&i, 1, MPI_INT, 0, FAIL_TAG, MPI_COMM_WORLD);
        failed_worker[i] = 1;
      }
    }
    if(received_update)
    {
      if(found_change == 0)
      {
        DEBUG_PRINT(("Received an update without any change :("));
      }
      MPI_Irecv(assignment_time2, number_of_slaves, MPI_DOUBLE, 0, SUPERVISOR_TAG, MPI_COMM_WORLD, &request2);
    } 
  }
}

void do_supervisor_as_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
  DEBUG_PRINT(("supervisor taking over"));

  int number_of_nonslaves = 2;

  int number_of_slaves;
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  number_of_slaves = number_of_slaves - number_of_nonslaves;
  
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

  num_work_units = get_total_units(work_array);

  mw_result_t * received_results = malloc(f->res_sz * num_work_units);
  if (received_results == NULL)
  {
    fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
    exit(0);
  }

  int num_results_received = 0;

  /** read through contents of file **/
  FILE *file = fopen("recovery.txt","r");
  if (file != NULL) //there are results to process
  {
    int result_index = 0;
    char str[1000];
    while(fscanf(file, "%d %s", &result_index, str) != EOF)
    {
      printf("%d %s\n", result_index, str);          
      // update received results  
      mw_result_t * result = f->from_str(str);
      received_results[result_index] = *result;
      // update num_results_received   
      num_results_received++;
    }
    DEBUG_PRINT(("Finished loading file contents"));
  }


  // tell slaves to send to supervisor now
  int master_fail = 0;
  for(slave=number_of_nonslaves; slave<(number_of_slaves+number_of_nonslaves); ++slave)
  {
    MPI_Send(&master_fail, 1, MPI_INT, slave, M_FAIL_TAG, MPI_COMM_WORLD);
  }
  return;
/*
      //receive all initial messages
      for(i=0; i<num_work_units; i++) {
        //MPI_Irecv(received_results[i], f->res_sz, MPI_CHAR, WORK_TAG, MPI_COMM_WORLD, &slave_request);
      }

  // make array keeping track of pointers for work that's active
  LinkedList* assignment_ptrs[number_of_slaves];

  // create array of start times
  double assignment_time[number_of_slaves];

  // create array of start times
  int assignment_indices[number_of_slaves];

  // create array indicating if slaves are down
  int are_you_down[number_of_slaves];

  // current pointer
  LinkedList
  * next_work_node = work_list,
  * list_end = NULL;

  // have supervisor so starting at number_of_nonslaves
  for(slave=number_of_nonslaves; slave<(number_of_slaves+number_of_nonslaves); ++slave)
  {
    are_you_down[slave-number_of_nonslaves] = 0; //slaves are all working in the beginning
    DEBUG_PRINT(("assigning work to slave"));

    if(next_work_node == NULL)
    {
      DEBUG_PRINT(("reached the end of the work, breaking!"));
      break;
    }

    mw_work_t * work_unit = next_work_node->data;

    send_to_slave(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);

    // save next_work_node to assigned work
    assignment_ptrs[slave-number_of_nonslaves] = next_work_node;
    assert(assignment_ptrs[slave-number_of_nonslaves] != NULL);
    
    // save start time
    assignment_time[slave-number_of_nonslaves] = MPI_Wtime();

    // save assignment indices
    assignment_indices[slave-number_of_nonslaves] = next_work_node->index;

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
  MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);

  // failure id
  int failure_id;

  MPI_Status status_fail, status_res;
  MPI_Request request_fail, request_res;
  int flag_fail = 0, flag_res = 0;

  // receive failure from supervisor as non-blocking recv
  MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);

  // receive result from workers as non-blocking recv
  MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_res);

  //int ping_sup = 0;

  // clear out file
  FILE * fptr;
  fptr = fopen("recovery.txt", "w");
  fclose(fptr);

  // send units of work while haven't received all results
  while(num_results_received < num_work_units)
  {
    // send ping to supervisor
    //MPI_Send(&ping_sup, 1, MPI_INT, 1, M_PING_TAG, MPI_COMM_WORLD);

    // check for flag_fail
    MPI_Test(&request_fail, &flag_fail, &status_fail);

    // check for flag_res
    MPI_Test(&request_res, &flag_res, &status_res);

    // send work if have failures or got results
    if (flag_fail)
    {
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
        if(assignment_time[failure_id] == 0.0)
        {
            DEBUG_PRINT(("Failure on idle process %d. WTF??", failure_id));
        }
        if(are_you_down[failure_id] == 1)
        {
            DEBUG_PRINT(("Failure on a process which is already failed. WTF??"));
        }
        if(assignment_indices[failure_id] == -1)
        {
            DEBUG_PRINT(("Failure on a process which is already failed. WTF??"));
        }
        are_you_down[failure_id] = 1; //this slave is considered dead :(
        assignment_ptrs[failure_id] = NULL;
        assignment_time[failure_id] = 0.0;
        assignment_indices[failure_id] = -1;
        MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
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
        send_to_slave(next_work_node->data, f->work_sz, MPI_CHAR, idle_process+number_of_nonslaves, WORK_TAG, MPI_COMM_WORLD);
        assignment_ptrs[idle_process] = next_work_node;
        assignment_time[idle_process] = MPI_Wtime();
        assignment_indices[idle_process] = next_work_node->index;
        MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
        DEBUG_PRINT(("Gave an assignment to previously idle process %d, assignment at %p", idle_process, next_work_node));
        if(next_work_node->next == NULL)
        {
            list_end = next_work_node;
        }
        next_work_node = next_work_node->next;
    }

    if (flag_res)
    {
      int worker_number = status_res.MPI_SOURCE-number_of_nonslaves;
     
      if(!are_you_down[worker_number]) //If this slave is marked dead, just ignore him
      {
        // save index and result received to file

        char * str = f->to_str(received_results[num_results_received]);
        fptr = fopen("recovery.txt", "a");
        fprintf(fptr, "%d|%s\n", assignment_indices[worker_number], str);
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
          mw_work_t* work_unit = next_work_node->data;

          // send new unit of work
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
          assignment_indices[status_res.MPI_SOURCE-number_of_nonslaves] = next_work_node->index;
          // send updated array of times to supervisor
          MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
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
            MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
        }
      }
      // continue to receive results from workers as non-blocking recv
      MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &request_res);      
    }
  }

  // send kill signal to other processes, including supervisor
  for(slave=1; slave<number_of_slaves+number_of_nonslaves; ++slave)
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
*/
}
