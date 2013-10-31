#include "mw.h"
#include "mw_api.h"
#include "def_structs.h"
#include <time.h>
#include <stdlib.h>
#include <limits.h>

#define DEBUG 0

// success probability
static float p = 1.95;

// implement random_fail()
int random_fail()
{
  float r = ((float) rand())/RAND_MAX;
  return r > p;
}

int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int rank)
{
  if (rank ==0 || random_fail()) {      
    DEBUG_PRINT(("%d FAIIIIILLLLLL!!!!!!", rank));
    MPI_Finalize();
    exit (0);
    return 0;
  } else {
    return MPI_Send (buf, count, datatype, dest, tag, comm);
  }
}

void be_a_slave(int argc, char** argv, struct mw_api_spec *f)
{
  mw_work_t work;
  mw_result_t * computedResult;
  //int ping = 1;
  MPI_Status status;

  // parse command line arg for success probability
  if (argc == 3)
  {
    float temp = atof(argv[2]);
    if (temp > .0 && temp < 1.)
      p = temp;
  }

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  DEBUG_PRINT(("Seeded srand with %u", (unsigned) time(NULL) + rank));
  srand((unsigned)time(NULL) + rank);

  MPI_Request request_master_fail;
  MPI_Status status_master_fail;
  int flag_master_fail = 0;
  int master_fail = 0;

  MPI_Irecv(&master_fail, 1, MPI_INT, 1, M_FAIL_TAG, MPI_COMM_WORLD, &request_master_fail);

  while(1)
  {
    MPI_Test(&request_master_fail, &flag_master_fail, &status_master_fail);
    // don't Irecv again because only need to recv once

    if (flag_master_fail)
    {
      MPI_Recv(&work, f->work_sz, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      if(status.MPI_TAG == KILL_TAG)
      {
        return;
      }

      computedResult = f->compute(&work);
      F_Send(computedResult, f->res_sz, MPI_CHAR, 1, WORK_TAG, MPI_COMM_WORLD, rank);
    }
    else {      
      MPI_Recv(&work, f->work_sz, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      if(status.MPI_TAG == KILL_TAG)
      {
        return;
      }

      computedResult = f->compute(&work);

      F_Send(computedResult, f->res_sz, MPI_CHAR, 0, WORK_TAG, MPI_COMM_WORLD, rank);
    }
  }
}
