#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "mw.h"
#include "map_reduce.h"
#include "map_reduce_user_def.h"
#include "debug.h"

#define DEBUG 1

static map_work_t map_work;
static reduce_key_t reduce_key;
static reduce_val_t reduce_val;
static map_reduce_result_t map_reduce_result;
static map_reduce_api_spec * f;

static float p = 1.0;

int random_fail()
{
  float r = ((float) rand())/RAND_MAX;
  return r > p;
}

int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int rank)
{
  if (0 && (rank == 5) || random_fail()) {      
    DEBUG_PRINT(("%d FAIIIIILLLLLL!!!!!!", rank));
    MPI_Finalize();
    exit (0);
    return 0;
  } else {
    return MPI_Send (buf, count, datatype, dest, tag, comm);
  }
}

void emit_intermediate(intermediate_t * result)
{
    //DEBUG_PRINT(("emiting result %s: %d", result->word, result->frequency));
    MPI_Send(result, sizeof(intermediate_t), MPI_CHAR, 0, INTERMEDIATE_RESULT_TAG, MPI_COMM_WORLD);
}

void do_map_work()
{
    DEBUG_PRINT(("Bout to do some map"));
    f->map(&map_work, emit_intermediate);
    DEBUG_PRINT(("Sending finished map signal"));
    MPI_Send("!", 1, MPI_CHAR, 0, FINISHED_MAP_TAG, MPI_COMM_WORLD);
}

void be_a_slave(int argc, char** argv, struct map_reduce_api_spec *_f)
{
  f = _f;
  map_reduce_result_t * computedResult;
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
  DEBUG_PRINT(("Being slave #%d!", rank));
  srand((unsigned)time(NULL) + rank);

  MPI_Request
    request_map_work,
    request_kill,
    request_finish_reduce,
    request_reduce_val,
    request_reduce_key;

  MPI_Status
    status_map_work,
    status_kill,
    status_finish_reduce,
    status_reduce_val,
    status_reduce_key;

  int
    flag_map_work,
    flag_kill,
    flag_finish_reduce,
    flag_reduce_key,
    flag_reduce_val,
    start_reducing = 0;

  char nothing;
  char kill;

  MPI_Irecv(&map_work, sizeof(map_work_t), MPI_CHAR, 0, MAP_WORK_TAG, MPI_COMM_WORLD, &request_map_work);
  MPI_Irecv(&reduce_key, sizeof(reduce_key_t), MPI_CHAR, 0, REDUCE_KEY_TAG, MPI_COMM_WORLD, &request_reduce_key);
  MPI_Irecv(&reduce_val, sizeof(reduce_val_t), MPI_CHAR, 0, REDUCE_VALUE_TAG, MPI_COMM_WORLD, &request_reduce_val);
  MPI_Irecv(&nothing, 1, MPI_CHAR, 0, FINISHED_REDUCE_TAG, MPI_COMM_WORLD, &request_finish_reduce);
  MPI_Irecv(&kill, 1, MPI_CHAR, 0, KILL_TAG, MPI_COMM_WORLD, &request_kill);

  while(1)
  {
    if(start_reducing)
    {
        MPI_Test(&request_reduce_val, &flag_reduce_val, &status_reduce_val); //THIS FAILS!!
        if(flag_reduce_val)
        {
            DEBUG_PRINT(("%d received value", rank));
            map_reduce_result.total_frequency += (long long) reduce_val.frequency;
            MPI_Irecv(&reduce_val, sizeof(reduce_val_t), MPI_CHAR, 0, REDUCE_VALUE_TAG, MPI_COMM_WORLD, &request_reduce_val);
        }
        else
        {
            //DEBUG_PRINT(("%d received no value", rank));
            MPI_Test(&request_finish_reduce, &flag_finish_reduce, &status_finish_reduce);
            if(flag_finish_reduce)
            {
                MPI_Send(&map_reduce_result, sizeof(map_reduce_result_t), MPI_CHAR, 0, FINISHED_REDUCE_TAG, MPI_COMM_WORLD);
                start_reducing = 0;
            }
        }
    }

    MPI_Test(&request_map_work, &flag_map_work, &status_map_work);
    if(flag_map_work)
    {
        do_map_work();
        MPI_Irecv(&map_work, sizeof(map_work_t), MPI_CHAR, 0, MAP_WORK_TAG, MPI_COMM_WORLD, &request_map_work);
    }

    MPI_Test(&request_reduce_key, &flag_reduce_key, &status_reduce_key);
    if(flag_reduce_key)
    {
        start_reducing = 1;
        strcpy(map_reduce_result.word, reduce_key.word);
        DEBUG_PRINT(("%d Reducing with key: %s", rank, reduce_key.word));
        MPI_Irecv(&reduce_key, sizeof(reduce_key_t), MPI_CHAR, 0, REDUCE_KEY_TAG, MPI_COMM_WORLD, &request_reduce_key);
    }

    MPI_Test(&request_kill, &flag_kill, &status_kill);
    if(flag_kill)
    {
        DEBUG_PRINT(("Slave killed :)"));
        return;
    }
  }
}
