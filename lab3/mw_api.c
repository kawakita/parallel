#include "mw_api.h"
#include "mw.h"

void MW_Run(int argc, char **argv, struct mw_api_spec *f)
{	
  int myid;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  
  if(0 == myid)
  {
    do_master_stuff(argc, argv, f);
  }
  else if(1 == myid)
  {
    do_supervisor_stuff(argc, argv, f);
  }
  else
  {
    be_a_slave(argc, argv, f);
  }
}

