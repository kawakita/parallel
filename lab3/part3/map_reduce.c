#include <stdlib.h>
#include <mpi.h>
#include "map_reduce.h"
#include "debug.h"

void MW_Run(int argc, char **argv, struct map_reduce_api_spec * f)
{	
  int myid;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  if(0 == myid)
  {
    do_master_stuff(argc, argv, f);
  }
  else if(1 == myid)
  {
    //do_supervisor_stuff(argc, argv, f);
  }
  else
  {
    be_a_slave(argc, argv, f);
  }
}

