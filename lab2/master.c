#include <stdio.h>

#include "mw.h"
#include "def_structs.h"

void kill_slave(int slave);
int get_total_units(mw_work_t ** work_list);

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f)
{
	int number_of_slaves;

	MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
	
	mw_work_t ** work_list;

	work_list = f->create(argc, argv);

	int i=0, slave=1, num_work_units=0;

	num_work_units = get_total_units(work_list);

	mw_result_t * received_results =  malloc(f->res_sz * num_work_units);

	int num_results_received = 0;

	for(slave=1; slave<number_of_slaves; ++slave)
	{
		mw_work_t * work_unit = work_list[i];
		i++;
		if(work_unit == NULL)
		{
			break;
		}
		MPI_Send(work_unit, f->work_sz, MPI_CHAR, slave, WORK_TAG, MPI_COMM_WORLD);
	}

	while(work_list[i] != NULL)
	{
		MPI_Status status;
		MPI_Recv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("received result %f\n", received_results[num_results_received].k);
		num_results_received++;
		MPI_Send(work_list[i], f->work_sz, MPI_CHAR, status.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);
		i++;
	}

	while(num_results_received < num_work_units)
	{
		MPI_Status status;
		printf("received result %f\n", received_results[num_results_received].k);
		MPI_Recv(&received_results[num_results_received], f->res_sz, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		num_results_received++;
	}

	for(slave=1; slave<number_of_slaves; ++slave)
	{
		kill_slave(slave);
	}

	int err_code = f->result(num_results_received, received_results);
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

