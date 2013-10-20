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
        
        MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
        MPI_Status status;
        
        
        int * assignment_number = malloc(sizeof(int)*number_of_slaves);
        // keep track of start times
        double * assignment_time = malloc(sizeof(double)*number_of_slaves);
        double * complete_time = malloc(sizeof(double)*number_of_slaves);

        // supervisor does blocking receive to get list of workers and their start times
       	MPI_Recv(&assignment_number, number_of_slaves, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv(&assignment_time, number_of_slaves, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	
        DEBUG_PRINT("supervisor knows what the workers are doing and when they started");

        int units_received = 0;
        int slave;
        
        // does non-blocking recv for pings from workers for first 50%
        while(units_received < number_of_slaves/2) 
        {
          for(
        }

	
	
	mw_work_t ** work_list;

        double start, end, start_create, end_create, start_results, end_results;

        start = MPI_Wtime();

	DEBUG_PRINT("creating work list...");
        start_create = MPI_Wtime();
	work_list = f->create(argc, argv);
        end_create = MPI_Wtime();
	DEBUG_PRINT("created work!");

	int i=0, slave=1, num_work_units=0;

	num_work_units = get_total_units(work_list);

	mw_result_t * received_results =  malloc(f->res_sz * num_work_units);
	if (received_results == NULL)
	{
	  fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
	  free(received_results);
	}

	int num_results_received = 0;

	for(slave=1; slave<number_of_slaves; ++slave)
	{
		DEBUG_PRINT("assigning work to slave");
		mw_work_t * work_unit = work_list[i];
		i++;
		if(work_unit == NULL)
		{
			DEBUG_PRINT("reached the end of the work, breaking!");
			break;
		}
		send_to_slave(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);
		//MPI_Send(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);
		DEBUG_PRINT("work sent to slave");
	}

	while(work_list[i] != NULL)
	{
		DEBUG_PRINT("Waiting to receive a result...");
		MPI_Status status;
		MPI_Recv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		num_results_received++;
		send_to_slave(work_list[i], f->work_sz, MPI_CHAR, status.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);
		i++;
	}

	while(num_results_received < num_work_units)
	{
		DEBUG_PRINT("Waiting to receive a result...");
		MPI_Status status;
		MPI_Recv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		num_results_received++;
	}

	for(slave=1; slave<number_of_slaves; ++slave)
	{
		DEBUG_PRINT("Murdering slave");
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

