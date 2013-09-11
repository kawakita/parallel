#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERATIONS 1
#define TAG 1
#define MSG_MAX 1000

void run_process_even(int);
void run_process_odd(int);
void compute_avg_bandwidth(double ** bandwidths, int my_id);

FILE * fptr;

int main (int argc, char **argv)
{
  fptr = fopen("bandwidth.txt", "w");
  fprintf(fptr, "---------\n");

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
    start_time,
    end_time,
    diff,
    * result,
    ** bandwidths = malloc(sizeof(double *) * MSG_MAX);

  int k;
  for(k=0; k<MSG_MAX; ++k)
  {
    bandwidths[k] = malloc(sizeof(double) * ITERATIONS);
  }  

  MPI_Status status;

  int msg_sz;
  for (msg_sz=0; msg_sz<MSG_MAX; ++msg_sz)
  { 
    double *messages = malloc(sizeof(double) * (msg_sz+1));
    result = malloc(sizeof(double) * (msg_sz+1));
    int i;
    for(i=0; i<ITERATIONS; ++i)
    {
      int j;

      start_time = MPI_Wtime();
      for(j=0; j<msg_sz+1; ++j)
        messages[j] = start_time;

      messages[0] = MPI_Wtime();
      
      MPI_Send(messages, msg_sz+1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD); 
      MPI_Recv(result,    msg_sz+1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD, &status);
      end_time = MPI_Wtime();
      diff = end_time - result[0];
      bandwidths[msg_sz][i] = diff;
    }
    free(messages);
    free(result);
  }

  compute_avg_bandwidth(bandwidths, my_id);
  int i;
  for(i=0; i<MSG_MAX; ++i)
    free(bandwidths[i]);
  free(bandwidths);
}


void run_process_odd(int my_id)
{
  double 
    start_time,
    end_time,
    diff,
    * result,
    ** bandwidths = malloc(sizeof(double *) * MSG_MAX);

  int k;
  for(k=0; k<MSG_MAX; ++k)
  {
    bandwidths[k] = malloc(sizeof(double) * ITERATIONS);
  }  

  MPI_Status status;

  int msg_sz;
  for (msg_sz=0; msg_sz<MSG_MAX; ++msg_sz)
  { 
    double *messages = malloc(sizeof(double) * (msg_sz+1));
    result = malloc(sizeof(double) * (msg_sz+1));
    int i;
    for(i=0; i<ITERATIONS; ++i)
    {
      int j;
      start_time = MPI_Wtime();
      for(j=0; j<msg_sz+1; ++j)
        messages[j] = start_time;

      messages[0] = MPI_Wtime();
      
      MPI_Recv(result,   msg_sz+1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD, &status);
      MPI_Send(messages, msg_sz+1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD); 
      end_time = MPI_Wtime();
      diff = end_time - result[0];
      bandwidths[msg_sz][i] = diff;
    }
    free(messages);
    free(result);
  }

  compute_avg_bandwidth(bandwidths, my_id);
  int i;
  for(i=0; i<MSG_MAX; ++i)
    free(bandwidths[i]);
  free(bandwidths);
}

void compute_avg_bandwidth(double ** bandwidths, int my_id)
{
  double total = 0.0;
  fptr = fopen("bandwidth.txt", "a");

  int msg_sz;
  for(msg_sz=0; msg_sz<MSG_MAX; ++msg_sz)
  {
    total = 0.0;
    int i;
    for (i=0; i<ITERATIONS; ++i)
    {
      total += bandwidths[msg_sz][i];
    }
    printf("Average bandwidth for p%d sz %d: %e bytes/s\n", my_id, (msg_sz+1), 
      (msg_sz+1) * sizeof(double) / (total / ITERATIONS));
    fprintf(fptr, "%d \t %e\n", (msg_sz+1)*sizeof(double), (msg_sz+1)*sizeof(double)/(total / ITERATIONS));
  }
  fclose(fptr);
}
