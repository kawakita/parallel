#include <stdio.h>
#include <math.h>

#include "mw.h"
#include "def_structs.h"


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

  // determine how long each worker took
  double * complete_time = malloc(sizeof(double)*number_of_slaves);
  // booleans for failure
  int * failed_worker = calloc(sizeof(int), number_of_slaves);

  // supervisor does blocking receive to get list of workers and their start times
  MPI_Recv(&assignment_time1, number_of_slaves, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  
  // calc approximate time diff between sup and master
  double master_time = assignment_time1[number_of_slaves-1];
  double mytime_off_by = MPI_Wtime() - master_time;

  DEBUG_PRINT(("supervisor knows when the workers started"));

  int units_received = 0;
  int i;
  int kill_msg, flag1, flag2;
  
  double tot_time=0, sq_err=0, mean=0, stddev=0, threshold=0;
  
  //waiting for updates on start times from master
  while(1) 
  {
    //kill myself if the master says so
    MPI_IRecv(&kill_msg, 1, MPI_INT, 0, KILL_TAG, MPI_COMM_WORLD, &request1);
    MPI_Test(&request1, &flag1, &status1);
    if(flag1)
    {
      exit(0);
    }
    
    //get a new start time array from master
    MPI_IRecv(&assignment_time2, 1, MPI_INT, 0, SUPERVISOR_TAG, MPI_COMM_WORLD, &request2);
    MPI_Test(&request2, &flag2, &status2);
    if(!flag2){
      continue;
    }

    //check for differences in working slaves
    for(i=0; i<number_of_slaves; i++) 
    {
      if(failed_worker[i] == 0)
      {
        //we have a good worker!
        if(assignment_time1[i] != assignment_time2[i])
        {
          complete_time[i] = assignment_time2[i] - assignment_time1[i];
          units_received++;
          tot_time += complete_time[i];
          mean = tot_time/units_received;
          sq_err += pow(complete_time[i] - mean, 2);
          stddev = sqrt(sq_err/units_received);

          //we have enough data to update threshold
          if(units_received >= number_of_slaves/2)
          {
            threshold = mean + 2*stddev;
          }
          
        }
        //if we have enough data, we can tell if we have a bad worker :(
        else if(threshold>0 && MPI_Wtime() - mytime_off_by - assignment_time1[i] > threshold)
        {
          MPI_Send(&i, 1, MPI_INT, 0, FAIL_TAG, MPI_COMM_WORLD);
          failed_worker[i] = 1;
        }
        //else assume the best and update the array
        assignment_time1 = assignment_time2;
      }
      
    }
    
  }
  
  
}


