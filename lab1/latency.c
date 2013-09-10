#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERATIONS 10000
#define TAG 1

void run_process_even(int);
void run_process_odd(int);
void compute_avg_latency(double * latencies, int my_id);

int main (int argc, char **argv)
{
  int sz, myid;

  MPI_Init (&argc, &argv);

  MPI_Comm_rank (MPI_COMM_WORLD, &myid);

  if (myid % 2 == 0)
    run_process_even(myid);
  else
    run_process_odd(myid);

  MPI_Finalize ();
  exit (0);
}

void run_process_even(int my_id)
{
  double 
    start_time,
    end_time,
    diff,
    result,
    * latencies = malloc(sizeof(double) * ITERATIONS);

  MPI_Status status;

  int i;
  for(i=0; i<ITERATIONS; ++i)
  {
    start_time = MPI_Wtime();
    MPI_Send(&start_time, 1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD); 
    MPI_Recv(&result, 1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD, &status);
    end_time = MPI_Wtime();
    diff = end_time - result;
    latencies[i] = diff;
  }

  compute_avg_latency(latencies, my_id);

  free(latencies);
}

void run_process_odd(int my_id)
{
  double 
    start_time,
    end_time,
    diff,
    result,
    * latencies = malloc(sizeof(double) * ITERATIONS);

  MPI_Status status;

  int i;
  for(i=0; i<ITERATIONS; ++i)
  {
    MPI_Recv(&result, 1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD, &status);
    end_time = MPI_Wtime();
    start_time = MPI_Wtime();
    MPI_Send(&start_time, 1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD); 
    diff = end_time - result;
    latencies[i] = diff;
  }

  compute_avg_latency(latencies, my_id);

  free(latencies);
}

void compute_avg_latency(double * latencies, int my_id)
{
  double total = 0.0;

  int i;
  for(i=0; i<ITERATIONS; ++i)
  {
    total += latencies[i];
  }
  printf("Average latency for p%d: %e s\n", my_id, total / ITERATIONS);
}
