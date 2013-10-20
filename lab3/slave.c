#include "mw.h"
#include "mw_api.h"
#include "def_structs.h"

int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  if (random_fail()) {      MPI_Finalize();
    exit (0);
    return 0;
  } else {
    return MPI_Send (buf, count, datatype, dest, tag, comm);
  }
}

void be_a_slave(struct mw_api_spec *f)
{
  mw_work_t work;
  mw_result_t * computedResult;
  MPI_Status status;

  while(1)
  {
    MPI_Recv(&work, f->work_sz, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	if(status.MPI_TAG == KILL_TAG)
	{
		return;
	}

    computedResult = f->compute(&work);

    DEBUG_PRINT("Sending result back!");
    F_Send(computedResult, f->res_sz, MPI_CHAR, 0, WORK_TAG, MPI_COMM_WORLD);
    DEBUG_PRINT("result sent");
  }
}
