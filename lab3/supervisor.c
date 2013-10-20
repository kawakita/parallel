#include <stdio.h>

#include "mw.h"
#include "def_structs.h"

// upon first 50%, computes mean and std dev

// sleeps

// as loop, does non-blocking recv for other workers

// sends failures to master

void do_supervisor_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
  
  
  DEBUG_PRINT("supervisor starting");
  
  int number_of_slaves;
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  MPI_Status status;
  number_of_slaves = number_of_slaves - 2;
  
  // who has which unit of work
  int * assignment_number = malloc(sizeof(int)*number_of_slaves);
  // keep track of start times
  double * assignment_time = malloc(sizeof(double)*number_of_slaves);
  // determine how long each worker took
  double * complete_time = malloc(sizeof(double)*number_of_slaves);

  // supervisor does blocking receive to get list of workers and their start times
  MPI_Recv(&assignment_number, number_of_slaves, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  MPI_Recv(&assignment_time, number_of_slaves, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  
  // calc approximate time diff between sup and master
  double mytime = MPI_Wtime();
  double master_time = assignment_time[number_of_slaves-1];
  double mytime_off_by = mytime - master_time;

  DEBUG_PRINT("supervisor knows what the workers are doing and when they started");

  int units_received = 0;
  int slave;

  // does non-blocking recv for pings from workers for first 50%
  while(units_received < number_of_slaves/2) 
  {
    for(slave = 0; slave<number_of_slaves; slave++) 
    {
      int pid = slave+2;
      int msg = 0;
      MPI_IRecv(msg, 1, MPI_INT, pid, PING_TAG, MPI_COMM_WORLD);
      if(msg == 1) 
      {
        double time_received = MPI_Wtime();
        units_received++;
        complete_time[slave] = 
      }
    }
    
  }


}


void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
{
  //gmp_printf("sending work %Zd to %d\n", work->start, slave);
	MPI_Send(work, size, datatype, slave, tag, comm);
	DEBUG_PRINT("Sent!");
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

