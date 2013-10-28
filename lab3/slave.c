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
  if (rank == 5 || random_fail()) {      
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
  int ping = 1;
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

  while(1)
  {
    // recv unit of work from master
    MPI_Recv(&work, f->work_sz, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if(status.MPI_TAG == KILL_TAG)
    {
      return;
    }

    // check for kill signal for non-blocking recv
    computedResult = f->compute(&work);

    //DEBUG_PRINT(("Result computed!"));
    // send unit of work to master with probability p
    F_Send(computedResult, f->res_sz, MPI_CHAR, 0, WORK_TAG, MPI_COMM_WORLD, rank);
    //DEBUG_PRINT(("result sent"));

    // send ping after unit of work is possibly sent
    MPI_Send(&ping, 1, MPI_INT, 1, PING_TAG, MPI_COMM_WORLD);
  }
}
