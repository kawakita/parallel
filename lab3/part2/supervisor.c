#include <stdio.h>
#include <math.h>
#include <string.h>

#include "mw.h"
#include "def_structs.h"
#include "linked_list.h"

#define DEBUG 0

void do_supervisor_as_master_stuff(int argc, char ** argv, struct mw_api_spec *f);
//void do_supervisor_as_master_stuff(int argc, char ** argv, struct mw_api_spec *f, double * assignment_time1, double * complete_time, double threshold, double tot_time, double sq_err, double mean, double stddev);

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
  // supervisor does non-blocking receive to get list of workers and their start times
  MPI_Irecv(assignment_time1, number_of_slaves, MPI_DOUBLE, 0, SUPERVISOR_TAG, MPI_COMM_WORLD, &request_started);

  while(!flag_started && MPI_Wtime()<(last_master_ping+master_threshold))
  {
    MPI_Test(&request_started, &flag_started, &status_started);
    if(flag_started) {
      last_master_ping = MPI_Wtime();
      DEBUG_PRINT(("testing %f", last_master_ping));
      break;
    }
  }
  // if we pass the threshold time consider master failed
  if(!flag_started) 
  {
    DEBUG_PRINT(("SUPERVISOR: MASTER FAILED before assigning work\n"));
    master_fail = 1;
    do_supervisor_as_master_stuff(argc, argv, f);
    return;
  }
  /** end accomodating master failure **/

  // determine how long each worker took
  double * complete_time = malloc(sizeof(double)*number_of_slaves);
  // booleans for failure
  int * failed_worker = calloc(sizeof(int), number_of_slaves);

  // calc approximate time diff between sup and master
  double master_time = assignment_time1[number_of_slaves-1];
  double mytime_off_by = MPI_Wtime() - master_time;

  DEBUG_PRINT(("supervisor knows when the workers started"));

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
      last_master_ping = MPI_Wtime();
      //DEBUG_PRINT(("got an update from master"));
    }
    else if(MPI_Wtime() > last_master_ping + master_threshold) {
      DEBUG_PRINT(("MASTER FAILED?!?!?\n"));
      // break and then call do_supervisor_as_master_stuff outside of loop
      break;
      //return;
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
  // call do_supervisor_as_master_stuff
  //do_supervisor_as_master_stuff(argc, argv, f, assignment_time1, complete_time, threshold, tot_time, sq_err, mean, stddev);
  do_supervisor_as_master_stuff(argc, argv, f);
  return;
}

void do_supervisor_as_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
  DEBUG_PRINT(("supervisor taking over"));

  int number_of_nonslaves = 2;

  int number_of_slaves;
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  number_of_slaves = number_of_slaves - number_of_nonslaves;

  DEBUG_PRINT(("NUMBER OF SLAVES %d", number_of_slaves));

  /** slave failure detection **/
  // keep track of start times
  //if (assignment_time1 == NULL)
  //double * assignment_time2 = malloc(sizeof(double)*number_of_slaves);

  // determine how long each worker took
  //if (complete_time == NULL)
  double * complete_time = malloc(sizeof(double)*number_of_slaves);

  // initialize threshold to 0.1
  double threshold = 0.1, tot_time = 0.0, sq_err = 0.0, mean = 0.0, stddev = 0.0;
    
  /** end slave failure detection **/
  
  double start, end, start_create, end_create, start_results, end_results;

  start = MPI_Wtime();

  DEBUG_PRINT(("creating work list..."));
  start_create = MPI_Wtime();
  // save work_array separately so we can find index later on
  mw_work_t ** work_array = f->create(argc, argv);
  // create work_list later
  end_create = MPI_Wtime();
  DEBUG_PRINT(("created work in %f seconds!", end_create - start_create));

  int num_work_units=0;

  num_work_units = get_total_units(work_array);

  DEBUG_PRINT(("num_work_units %d\n", num_work_units));

  mw_result_t * received_results = calloc(num_work_units, f->res_sz);
  if (received_results == NULL)
  {
    fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
    exit(0);
  }

  int * has_result_array = calloc(num_work_units, sizeof(int));

  int num_results_received = 0;

  /** read through contents of file **/
  FILE *file = fopen("recovery.txt","r");
  if (file != NULL) //there are results to process
  {
    int result_index = 0;
    char str[1000];
    while(fscanf(file, "%d %s", &result_index, str) != EOF)
    {
      //printf("%d %s\n", result_index, str);          
      // update received results  
      mw_result_t * result = f->from_str(str);
      //printf("here\n");
      received_results[result_index] = *result;
      //printf("now here\n");
      // update has_results_array
      has_result_array[result_index] = 1;
      // update num_results_received
      num_results_received++;
    }
  }

  DEBUG_PRINT(("num_results_received %d\n", num_results_received));

  // create linked list of indices not in the results array
  LinkedList * work_list = new_linkedlist_node();
  LinkedList * next_work_node = work_list;
  LinkedList * head = work_list;

  // cycle through has_result_array to find indices not in results array
  int i;
  int num_results_needed = 0;
  for (i = 0; i < num_work_units; i++)
  {
    if (has_result_array[i] == 0)
    {
      next_work_node->index = i;
      next_work_node->data = work_array[i];
      if (num_results_needed < (num_work_units-num_results_received)-1)
        addNode(next_work_node);
      if (next_work_node->next == NULL);
        next_work_node = next_work_node->next;
      num_results_needed++;
    }
  }
  DEBUG_PRINT(("num_results_needed %d", num_results_needed));

  // reset next_work_node to head
  next_work_node = head;

  // tell slaves to send to supervisor now
  int slave;
  for(slave=number_of_nonslaves; slave<(number_of_slaves+number_of_nonslaves); ++slave) {
    DEBUG_PRINT(("Telling slave"));
    MPI_Send(0, 0, MPI_CHAR, slave, M_FAIL_TAG, MPI_COMM_WORLD);
  }

  // make array keeping track of pointers for work that's active
  LinkedList* assignment_ptrs[number_of_slaves];

  // create array of start times
  double assignment_time[number_of_slaves];

  // create array of start times
  int assignment_indices[number_of_slaves];

  // create array indicating if slaves are down
  int are_you_down[number_of_slaves];

  // pointer for end of list
  LinkedList * list_end = NULL;

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

    DEBUG_PRINT(("work %d sent to slave %d", assignment_indices[slave-number_of_nonslaves], slave));
  }

  // no need to send time array to supervisor

  MPI_Status status_res;
  MPI_Request request_res;
  int flag_res = 0;

  // receive result from workers as non-blocking recv
  MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_res);

  // don't clear out file; will append new results to recovery.txt
  FILE * fptr;

  // send units of work while haven't received all results
  while(num_results_received < num_work_units)
  {
    // check for flag_res
    MPI_Test(&request_res, &flag_res, &status_res);

    // send work if have failures or got results

      /** slave failure detection **/
      // check if slave has not responded for a long time
      for(i=0; i<number_of_slaves; i++)
      {
        // not failed and not idle
        if (!are_you_down[i] && assignment_time[i] != 0.0)
        {
          if (i == 4)
            DEBUG_PRINT(("NOT FAILED NOT IDLE rank %d %f",i+2, MPI_Wtime()-assignment_time[i]));
          if(threshold>0 && MPI_Wtime() - assignment_time[i] > threshold)
          {
            DEBUG_PRINT(("methinks someone is slacking of rank %d", i+2));
            are_you_down[i] = 1;
            assignment_time[i] = 0.0;
            assignment_indices[i] = -1;

            // get work_unit that needs to be reassigned
            LinkedList * work_unit = assignment_ptrs[i];

            if (work_unit == NULL)
              DEBUG_PRINT(("work_unit is NULL"));

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

          }
        }
      }
      /** end slave failure detection **/

    
    // find an idle process to assign work to
    int idle_process = -1, i;
    for(i=0; i<number_of_slaves; ++i)
    {
        if(assignment_time[i] == 0.0 && !are_you_down[i])
        {
            idle_process = i;
            break;
        }
    }

    // assign idle process unit of work
    if(next_work_node != NULL && idle_process > -1)
    {
        send_to_slave(next_work_node->data, f->work_sz, MPI_CHAR, idle_process+number_of_nonslaves, WORK_TAG, MPI_COMM_WORLD);

        /** slave failure detection **/
        //a previously idle worker got assigned something
        DEBUG_PRINT(("Worker of rank %d just got off his lazy ass", i+2));
        /** end slave failure detection **/

        assignment_ptrs[idle_process] = next_work_node;
        if (idle_process == 4)
          DEBUG_PRINT(("changing rank 6 time idle"));
        assignment_time[idle_process] = MPI_Wtime();
        assignment_indices[idle_process] = next_work_node->index;
        //MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
        DEBUG_PRINT(("Gave an assignment to previously idle process rank %d, assignment at %p", idle_process+number_of_nonslaves, next_work_node));
        if(next_work_node->next == NULL)
        {
            list_end = next_work_node;
        }
        next_work_node = next_work_node->next;
    }

    if (flag_res)
    {

      int worker_number = status_res.MPI_SOURCE-number_of_nonslaves;

      DEBUG_PRINT(("Got result from rank %d", worker_number+number_of_nonslaves));
     
      if(!are_you_down[worker_number]) //If this slave is marked dead, just ignore him
      {
        // save index and result received to file
        char * str = f->to_str(received_results[num_results_received]);
        fptr = fopen("recovery.txt", "a");
        fprintf(fptr, "%d %s\n", assignment_indices[worker_number], str);
        fclose(fptr);

        // update number of results received
        num_results_received++;

        /** slave failure detection **/
        //DEBUG_PRINT(("supervisor is impressed by his good worker %d", i));
        int i = worker_number;
        complete_time[i] = MPI_Wtime() - assignment_time[i];
        tot_time += complete_time[i];
        mean = tot_time/num_results_received;
        sq_err += pow(complete_time[i] - mean, 2);
        stddev = sqrt(sq_err/num_results_received);
        //we have enough data to update threshold
        if(num_results_received >= number_of_slaves/2)
        {
          //DEBUG_PRINT(("the stddev is %f", stddev));
          threshold = mean + 10*stddev + 0.1;
          //DEBUG_PRINT(("the threshold is %f", threshold));
        }
        //assignment_time1[i] = assignment_time2[i];
        //found_change = 1;
        /** end slave failure detection **/

        //DEBUG_PRINT(("num results received %d\n", num_results_received));

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
          
          //DEBUG_PRINT(("Sending new unit of work"));

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
          if (status_res.MPI_SOURCE == 4)
            DEBUG_PRINT(("changing process of rank 6 time in res recv"));

          assignment_time[status_res.MPI_SOURCE-number_of_nonslaves] = MPI_Wtime();
          assignment_indices[status_res.MPI_SOURCE-number_of_nonslaves] = next_work_node->index;
          // send updated array of times to supervisor
          //MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
          //DEBUG_PRINT(("SENT TIME TO SUP"));
          next_work_node = next_work_node->next;
          if(next_work_node == NULL)
          {
              DEBUG_PRINT(("Reached the end of the work list, should get idle processors after this"));
          }
        }
        else
        {
            DEBUG_PRINT(("Worker of rank %d is now idle, I ain't got shit for him to do", worker_number+2));
            if (worker_number == 4)
              DEBUG_PRINT(("changing processof rank 6 time in else"));
            assignment_time[worker_number] = 0.0;
            assignment_ptrs[worker_number] = NULL;
            assignment_indices[worker_number] = -1;
            assert(!are_you_down[worker_number]);

            //MPI_Send(assignment_time, number_of_slaves, MPI_DOUBLE, 1, SUPERVISOR_TAG, MPI_COMM_WORLD);
        }
      }


      // continue to receive results from workers as non-blocking recv
      MPI_Irecv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &request_res);      
    }
  }

  // send kill signal to other processes
  for(slave=number_of_nonslaves; slave<number_of_slaves+number_of_nonslaves; ++slave)
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

  // remove recovery file since it is no longer useful
  remove("recovery.txt");

}
