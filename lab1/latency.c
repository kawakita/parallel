#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERATIONS 10000
#define TAG 1

void run_process_even(int);
void run_process_odd(int);
void compute_avg_latency(double * latencies, int id_from, int id_to);

FILE * fptr;

int main (int argc, char **argv)
{
  fptr = fopen("latency.txt", "w");

  int sz, myid;

  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &sz);

  if (sz % 2 != 0)
  {
    printf("Even number of processes only! Try again.\n");
    MPI_Finalize ();
    exit (-1);
  }

  MPI_Comm_rank (MPI_COMM_WORLD, &myid);

  // Processes with even myid communicate with myid+1.
  // Processes with odd  myid communicate with myid-1.
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
    sent_time,
    recv_time,
    diff,
    result,
    * latencies = malloc(sizeof(double) * ITERATIONS);

  MPI_Status status;

  int i;
  for(i=0; i<ITERATIONS; ++i)
  {
    // Send sent_time from my_id   to my_id+1. 
    sent_time = MPI_Wtime();
    MPI_Send(&sent_time, 1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD);

    // Recv sent_time from my_id+1 to my_id.
    MPI_Recv(&result,     1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD, &status);
    recv_time = MPI_Wtime();

    // Calculate latency from my_id+1 to my_id.
    diff = recv_time - result;
    latencies[i] = diff;
  }

  compute_avg_latency(latencies, my_id+1, my_id);

  free(latencies);
}

void run_process_odd(int my_id)
{
  double 
    sent_time,
    recv_time,
    diff,
    result,
    * latencies = malloc(sizeof(double) * ITERATIONS);

  MPI_Status status;

  int i;
  for(i=0; i<ITERATIONS; ++i)
  {
    // Recv sent_time from my_id-1 to my_id.
    MPI_Recv(&result,     1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD, &status);
    recv_time = MPI_Wtime();

    // Send sent_time from my_id   to my_id-1. 
    sent_time = MPI_Wtime();
    MPI_Send(&sent_time, 1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD);

    // Calculate latency from my_id-1 to my_id.
    diff = recv_time - result;
    latencies[i] = diff;
  }

  compute_avg_latency(latencies, my_id-1, my_id);

  free(latencies);
}

void compute_avg_latency(double * latencies, int id_from, int id_to)
{
  double total = 0.0;

  int i;
  for(i=0; i<ITERATIONS; ++i)
  {
    total += latencies[i];
  }
  double avg =  total / ITERATIONS;
  printf("Average latency from p%d to p%d: %e s\n", id_from, id_to, avg);

  fptr = fopen("latency.txt", "a");
  fprintf(fptr, "p%d \t p%d \t %e\n", id_from, id_to, avg); 
  fclose(fptr);
}
