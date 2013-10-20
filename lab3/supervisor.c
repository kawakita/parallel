#include <stdio.h>

#include "mw.h"
#include "def_structs.h"

// sleeps

// as loop, does non-blocking recv for other workers

// sends failures to master

void do_supervisor_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
  
  
  DEBUG_PRINT("supervisor starting");
  
  int number_of_slaves;
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  number_of_slaves = number_of_slaves - 2;
  
  MPI_Status status;
  MPI_Request request;
  
  // who has which unit of work
  int * assignment_number = malloc(sizeof(int)*number_of_slaves);
  // keep track of start times
  double * assignment_time = malloc(sizeof(double)*number_of_slaves);
  // determine how long each worker took
  double * complete_time = malloc(sizeof(double)*number_of_slaves);
  int * completed = calloc(sizeof(int), number_of_slaves);

  // supervisor does blocking receive to get list of workers and their start times
  MPI_Recv(&assignment_number, number_of_slaves, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  MPI_Recv(&assignment_time, number_of_slaves, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  
  // calc approximate time diff between sup and master
  double master_time = assignment_time[number_of_slaves-1];
  double mytime_off_by = MPI_Wtime() - master_time;

  DEBUG_PRINT("supervisor knows what the workers are doing and when they started");

  int units_received = 0;
  int msg = 0, flag=0;
  
  MPI_IRecv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, MPI_COMM_WORLD, &request);
  
  double tot_time;

  // does non-blocking recv for pings from workers for first 50%
  while(units_received < number_of_slaves/2) 
  {
    MPI_Test(&request, &flag, &status);
    if(flag)
    {
      int pid = status.MPI_SOURCE;
      int slave = pid - 2;

      if(complete_time[slave] == NULL) 
      {
        //actual time it takes to complete, relative to when master sent
        complete_time[slave] = MPI_Wtime() - mytime_off_by - assignment_time[slave];
        completed[slave] = 1;
        units_received++;
        tot_time += complete_time[slave];
      }
    } 
    
  }
  
  double start_to_wait = MPI_Wtime();

  // upon first 50%, computes mean and std dev
  double mean_time = tot_time/units_received;
  double standard_dev = stdDev(complete_time);
  
  while(MPI_Wtime() <= start_to_wait + 3*standard_dev) 
  {
    MPI_Test(&request, &flag, &status);
    if(flag)
    {
      pid = status.MPI_SOURCE;
      completed[slave] = 1;
    }
  }
  
  
}

double stdDev(double nums[])
{
  double sum = 0;
  double mean = average(nums);
  int i;

  for(i = 0; i < maxNums; i++) sum += pow(nums[i]-mean, 2);

  return sqrt(sum/maxNums);
}
