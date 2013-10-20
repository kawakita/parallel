#include <stdio.h>
#include "mw.h"
#include "def_structs_part3.h"

void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
void kill_slave(int slave);
int get_total_units(mw_work_t ** work_list);

void print_work(mw_work_t * work)
{
	printf("Printing work unit at address: %lu\n", (unsigned long) work);
	printf("\tfilename: %s\n", work->filename);
	printf("\tfirst line: %d\n", work->first_line);
	printf("\tlines each: %d\n", work->lines_each);
}

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{

  DEBUG_PRINT("master starting");
  double start = MPI_Wtime();
	int number_of_slaves;

	MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
	
	mw_work_t ** work_list;

	printf("argc: %d\n", argc);
	int i;
	for(i=0;  i<argc; ++i)
	{
		printf("arg[%d]: %s\n",i, argv[i]);
	}
	DEBUG_PRINT("creating work list...");
	work_list = f->create(argc, argv);
	//print_work(work_list[0]);
	DEBUG_PRINT("created work!");

	int slave=1, num_work_units=0;

	num_work_units = get_total_units(work_list);
	mw_result_t * received_results =  malloc(f->res_sz * num_work_units);
	if (received_results == NULL)
	{
	  fprintf(stderr, "ERROR: insufficient memory to allocate received_results\n");
	  free(received_results);
	}

	int num_results_received = 0;

	i=0;
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
                //printf("%ul is my numba\n",(unsigned long)work_unit);
                //printf("%d is my first line\n", work_unit->lines_each);
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

	printf("i received %d results\n", num_results_received);
        for(slave=1; slave<number_of_slaves; ++slave)
	{
		DEBUG_PRINT("Murdering slave");
		kill_slave(slave);
	}
       
	int err_code = f->result(num_results_received, received_results);
        double elapsed_time = MPI_Wtime() - start;
        printf("%f was my elapsed time for %d work units\n", elapsed_time, num_results_received); 
}


void send_to_slave(mw_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
{
  DEBUG_PRINT("Sending!!");
  // printf("%ul\n",(unsigned long)work);
  //printf("%d is the first line \n", work->first_line);
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
