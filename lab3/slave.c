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
  if (rank == 0 || rank == 5 || random_fail()) {      
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
  int work_size = f->work_sz;
  mw_work_t work;
  mw_result_t * computedResult;
  //int ping = 1;
  MPI_Status status;
  MPI_Request request;

  // parse command line arg for success probability
  if (argc == 3)
  {
    float temp = atof(argv[2]);
    if (temp > .0 && temp < 1.)
      p = temp;
  }
  
  int master_id = 0;
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  DEBUG_PRINT(("Seeded srand with %u", (unsigned) time(NULL) + rank));
  srand((unsigned)time(NULL) + rank);  
   
  while(1)
  {
    // wait for some sort of message
    MPI_Recv(&work, work_size, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if(status.MPI_SOURCE == 1 && status.MPI_TAG == NEW_MASTER_TAG)
    {
      master_id = 2;
    }
    
    else if(status.MPI_TAG == KILL_TAG && status.MPI_SOURCE == master_id)
    {
      return;
    }
    
    else if(status.MPI_SOURCE == master_id && status.MPI_TAG == WORK_TAG)
    {

      // check for kill signal for non-blocking recv
      computedResult = f->compute(&work);

      //DEBUG_PRINT(("Result computed!"));
      // send unit of work to master with probability p
      F_Send(computedResult, f->res_sz, MPI_CHAR, 0, WORK_TAG, MPI_COMM_WORLD, rank);
      F_Send(computedResult, f->res_sz, MPI_CHAR, 2, WORK_TAG, MPI_COMM_WORLD, rank);

    }

  }
}
