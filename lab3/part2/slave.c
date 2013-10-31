#include "mw.h"
#include "mw_api.h"
#include "def_structs.h"
#include <time.h>
#include <stdlib.h>
#include <limits.h>

#define DEBUG 1

int random_fail(){
  int p = 997;
  int range = 1000;
  int r = rand();
  int r_mod = r % range;
  //printf("@@@ random_fail : r=%d, r_mod=%d, shouldfail=%d\n",r,r_mod,(r_mod>p));
  if(r_mod > p) {
    return 1;
  } else {
    return 0;
  }
}

int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int rank)
{
  if (rank == 0 || random_fail()) {      
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

  int p;
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

  mw_work_t work;
  MPI_Request request_master, request_sup;
  MPI_Status status_master, status_sup;
  int flag_master = 0, flag_sup = 0;
  mw_result_t * computedResult;

  MPI_Irecv(&work, f->work_sz, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request_master);
  MPI_Irecv(&work, f->work_sz, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &request_sup);

  int master_failed = 0;

  while(1)
  {
    MPI_Test(&request_master, &flag_master, &status_master);
    MPI_Test(&request_sup, &flag_sup, &status_sup);        

    if ((flag_master || flag_sup) && (status_master.MPI_TAG == KILL_TAG || status_sup.MPI_TAG == KILL_TAG))
        return;

    if (!master_failed && flag_sup && (status_sup.MPI_TAG == M_FAIL_TAG))
    {
      //printf("MASTER FAILED\n");
      master_failed = 1;
      MPI_Irecv(&work, f->work_sz, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &request_sup);
    }

    if (!master_failed && flag_master && status_master.MPI_TAG == WORK_TAG)
    {
      computedResult = f->compute(&work);
      F_Send(computedResult, f->res_sz, MPI_CHAR, 0, WORK_TAG, MPI_COMM_WORLD, rank);
      MPI_Irecv(&work, f->work_sz, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request_master);
    }
    else if (master_failed && flag_sup && status_sup.MPI_TAG == WORK_TAG)
    {
      //printf("MASTER FAILED SENDING RESULT\n");
      computedResult = f->compute(&work);
      F_Send(computedResult, f->res_sz, MPI_CHAR, 1, WORK_TAG, MPI_COMM_WORLD, rank);
      MPI_Irecv(&work, f->work_sz, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &request_sup);
    }      
  }
}
