#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERATIONS 1000
#define TAG 1
// EXP_MAX should be multiple of INCREMENT.
#define EXP_MAX 6
#define INCREMENT 3

void run_process_even(int);
void run_process_odd(int);
void compute_avg_bandwidth(double ** bandwidths, int my_id);

FILE * fptr;

int main (int argc, char **argv)
{
  fptr = fopen("bandwidth.txt", "w");

  int sz, myid;

  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &sz);

  if (sz % 2 != 0)
  {
    printf("Even number of processes only. Try again.\n");
    MPI_Finalize();
    exit (-1);
  }

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
    send_time,
    recv_time,
    diff,
    * result,
    ** bandwidths = malloc(sizeof(double *) * (EXP_MAX/INCREMENT));

  int m;
  for(m=0; m<(EXP_MAX/INCREMENT); ++m)
  {
    bandwidths[m] = malloc(sizeof(double) * ITERATIONS);
  }  

  MPI_Status status;

  int msg_sz, msg_sz_index = 0;
  for (msg_sz=0; msg_sz<(EXP_MAX/INCREMENT); msg_sz+=INCREMENT)
  { 
    // Fill in array of appropriate length with values.
    //double *messages = malloc(sizeof(double) * (1<<msg_sz));
    int num_double = 1<<msg_sz;
    double messages[num_double];    
    int j;
    for(j=0; j<(1<<msg_sz); ++j)
      messages[j] = 0.0;

    int i;
    for(i=0; i<ITERATIONS; ++i)
    {
      // First element is time right before send.
      messages[0] = MPI_Wtime();
      MPI_Send(&messages, 1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD); 
      MPI_Recv(&result,   1, MPI_DOUBLE, my_id+1, TAG, MPI_COMM_WORLD, &status);
      recv_time = MPI_Wtime();
      printf("result %f\n", result[0]);
      diff = recv_time - result[0];
      //bandwidths[msg_sz_index][i] = diff;
    }
    msg_sz_index++;
    //free(messages);
  }

  //compute_avg_bandwidth(bandwidths, my_id);
  free(bandwidths);
}

void run_process_odd(int my_id)
{
  double 
    send_time,
    recv_time,
    diff,
    * result,
    ** bandwidths = malloc(sizeof(double *) * (EXP_MAX*INCREMENT));

  int m;
  for(m=0; m<(EXP_MAX/INCREMENT); ++m)
  {
    bandwidths[m] = malloc(sizeof(double) * ITERATIONS);
  }  

  MPI_Status status;

  int msg_sz, msg_sz_index = 0;
  for (msg_sz=0; msg_sz<(EXP_MAX/INCREMENT); msg_sz+=INCREMENT)
  { 
    // Fill in array of appropriate length with values.
    //double *messages = malloc(sizeof(double) * (1<<msg_sz));
    int num_double = 1<<msg_sz;
    double messages[num_double];
    int j;
    for(j=0; j<(1<<msg_sz); ++j)
      messages[j] = send_time;

    int i;
    for(i=0; i<ITERATIONS; ++i)
    {
      // First element is time right before send.
      messages[0] = MPI_Wtime();
      MPI_Recv(&result,   1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD, &status);
      MPI_Send(&messages, 1, MPI_DOUBLE, my_id-1, TAG, MPI_COMM_WORLD); 
      recv_time = MPI_Wtime();
      diff = recv_time - result[0];
      //bandwidths[msg_sz_index][i] = diff;
    }
    msg_sz_index++;
    free(messages);
  }

  //compute_avg_bandwidth(bandwidths, my_id);
  free(bandwidths);
}

void compute_avg_bandwidth(double ** bandwidths, int my_id)
{
  double total = 0.0;

  fptr = fopen("bandwidth.c", "a");

  int msg_sz_index = 0, msg_sz_exp = 0;
  for(msg_sz_index=0; msg_sz_index<(EXP_MAX/INCREMENT); ++msg_sz_index)
  {
    total = 0.0;
    int i;
    for (i=0; i<ITERATIONS; ++i)
    {
      total += bandwidths[msg_sz_index][i];
    }
    double avg = total / ITERATIONS;
    int num_bytes =  sizeof(double)*(1<<msg_sz_exp);
    printf("Average bandwidth for p%d of msg_sz %4d bytes: %e bytes/s\n", 
	   my_id, num_bytes, (num_bytes / avg));
    msg_sz_index++;
    msg_sz_exp += INCREMENT;
  }

  fclose(fptr);
}
