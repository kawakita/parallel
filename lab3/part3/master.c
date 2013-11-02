#include <glib.h>
#include <string.h>
#include <mpi.h>
#include <assert.h>

#include "linked_list.h"
#include "map_reduce.h"
#include "mw.h"
#include "debug.h"

static char * result_file_name = "this_is_the_map_result_file";

static GHashTable 
    * finished,
    * intermediate_results;

static GList * table_keys;

static int 
    * are_you_down,
    * are_you_working,
    work_creation_time,
    number_of_slaves;

static double * assignment_time;

LinkedList ** assignment_ptrs;
reduce_key_t ** reduce_assignment_ptrs;

void send_map_to_slave(map_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
void send_reduce_to_slave(reduce_key_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm);
int get_total_units(map_work_t ** work_list);
LinkedList * create_map_work_list(map_reduce_api_spec * f, int argc, char ** argv);
void kill_all_slaves();

void a_process_finished_map(int worker_id)
{
    are_you_working[worker_id] = 0;
}

void a_process_finished_reduce(int worker_id)
{
    are_you_working[worker_id] = 0;
}

void handle_intermediate_result(intermediate_t * intermediate_result_buffer)
{
    //TODO replace contains with something from 2.28
    if(!g_hash_table_lookup(intermediate_results, intermediate_result_buffer->word))
    {
        char * new_key = malloc(strlen(intermediate_result_buffer->word) * sizeof(char));
        strcpy(new_key, intermediate_result_buffer->word);
        g_hash_table_insert(intermediate_results, 
                            new_key,
                            g_hash_table_new(g_int_hash, g_int_equal));
    }
    assert(intermediate_results != NULL);
    GHashTable * values = g_hash_table_lookup(intermediate_results, intermediate_result_buffer->word);
    assert(values != NULL);
    g_hash_table_insert(values, &(intermediate_result_buffer->frequency), &(intermediate_result_buffer->frequency));
}

int next_available_worker()
{
    int i;
    for(i=0; i<number_of_slaves; ++i)
    {
        if(!are_you_down[i] && !are_you_working[i])
        {
            return i;
        }
    }
    return -1;
}

void get_unfinished_reduce(char * key, GHashTable * vals, void * nothing)
{
    if(g_hash_table_contains(finished, key)) //fix this
    {
        return;
    }
    table_keys = g_list_append(table_keys, key); 
}

void try_to_assign_reduce()
{
    assert(intermediate_results != NULL);
    table_keys = NULL;
    g_hash_table_foreach(intermediate_results, (GHFunc) get_unfinished_reduce, NULL);
    GList * key = table_keys;
    if(key == NULL) { DEBUG_PRINT(("No unfinished values??")); }
    int worker_id = next_available_worker();
    if(key == NULL || worker_id == -1) return;
    DEBUG_PRINT(("assigning reduce %s to %d", key->data, worker_id+2));
    MPI_Send(key->data, sizeof(reduce_key_t), MPI_CHAR, worker_id+2, REDUCE_KEY_TAG, MPI_COMM_WORLD);
    are_you_working[worker_id] = 1;
    reduce_assignment_ptrs[worker_id] = key->data;
    assignment_time[worker_id] = MPI_Wtime();
    assert(intermediate_results != NULL);
    GHashTable * value_table = g_hash_table_lookup(intermediate_results, key->data);
    //DEBUG_PRINT(("Getting value from table %s", key->data));
    assert(value_table != NULL);
    GList * value = g_hash_table_get_keys(value_table); //fix this
    while(value)
    {
        //This is the one that is crashing on the slave side!
        //printf("sending value %d bytes: %d\n", *(value->data), sizeof(*(value->data)));
        MPI_Send(value->data, sizeof(*(value->data)), MPI_CHAR, worker_id+2, REDUCE_VALUE_TAG, MPI_COMM_WORLD);
        DEBUG_PRINT(("Sent a reduction value"));
        value = g_list_next(value);
    }
    char nothing;
    MPI_Send(&nothing, 1, MPI_CHAR, worker_id+2, FINISHED_REDUCE_TAG, MPI_COMM_WORLD);
    g_hash_table_add(finished, key->data);//fix this
    g_list_free(table_keys);
}

void try_to_assign_work(LinkedList ** next_work_node)
{
    int worker = next_available_worker();
    if(-1 == worker || NULL == *next_work_node)
    {
        return;
    }
    DEBUG_PRINT(("Assigning map work %s to %d", (*next_work_node)->data, worker+2));
    MPI_Send((*next_work_node)->data, sizeof(map_work_t), MPI_CHAR, worker+2, MAP_WORK_TAG, MPI_COMM_WORLD);
    are_you_working[worker] = 1; 
    assignment_ptrs[worker] = (*next_work_node);
    assignment_time[worker] = MPI_Wtime();
    (*next_work_node) = (*next_work_node)->next;
}

void do_master_stuff(int argc, char ** argv, struct map_reduce_api_spec *f)
{
  int 
    slave=1,
    num_map_work_units=0;

  LinkedList * work_list;

  double 
    start, 
    end, 
    start_create, 
    end_create, 
    start_results, 
    end_results;


  MPI_Comm_size(MPI_COMM_WORLD, &number_of_slaves);
  number_of_slaves -= 2;
  start = MPI_Wtime();
  DEBUG_PRINT(("Creating work list"));
  work_list = create_map_work_list(f, argc, argv);
  num_map_work_units = list_length(work_list);

  DEBUG_PRINT(("list length: %d", num_map_work_units));

  intermediate_results = g_hash_table_new(g_str_hash, g_str_equal);
  finished = g_hash_table_new(g_str_hash, g_str_equal);

  DEBUG_PRINT(("created a hash table"));

  assignment_ptrs = malloc(sizeof(LinkedList*) * (number_of_slaves));
  reduce_assignment_ptrs = malloc(sizeof(reduce_key_t*) * (number_of_slaves));
  assignment_time = malloc(sizeof(double) * (number_of_slaves));
  are_you_down = malloc(sizeof(double) * (number_of_slaves));
  are_you_working = malloc(sizeof(double) * (number_of_slaves));
  assert(assignment_ptrs && assignment_time && are_you_down && are_you_working);

  LinkedList
    * next_work_node = work_list,
    * list_end = NULL;

  DEBUG_PRINT(("First work unit: %s", next_work_node->data->filename));

  for(slave=0; slave<number_of_slaves; ++slave)
  {
    are_you_down[slave] = 0;
    are_you_working[slave] = 0;
    assignment_time[slave] = 0;
    assignment_ptrs[slave] = NULL;
    reduce_assignment_ptrs[slave] = NULL;
  }

  MPI_Request 
    request_finished_mapping,
    request_intermediate,
    request_fail;

  MPI_Status
    status_finished_mapping,
    status_intermediate,
    status_fail;

  int 
    failure_id,
    flag_finished_mapping,
    flag_intermediate,
    flag_fail;

  intermediate_t * intermediate_result_buffer = malloc(sizeof(char) * sizeof(intermediate_t));
  assert(intermediate_result_buffer != NULL);
  char nothing;

  MPI_Irecv(intermediate_result_buffer, sizeof(intermediate_t), MPI_CHAR, MPI_ANY_SOURCE, INTERMEDIATE_RESULT_TAG, MPI_COMM_WORLD, &request_intermediate);
  MPI_Irecv(&nothing, 1, MPI_CHAR, MPI_ANY_SOURCE, FINISHED_MAP_TAG, MPI_COMM_WORLD, &request_finished_mapping);
  MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);

  int total_maps_processed = 0;
  while(total_maps_processed < num_map_work_units)
  {
    MPI_Test(&request_fail, &flag_fail, &status_fail);
    if(flag_fail)
    {
      //handle a failure
      //create new irecv
    }

    MPI_Test(&request_intermediate, &flag_intermediate, &status_intermediate);
    if(flag_intermediate)
    {
      handle_intermediate_result(intermediate_result_buffer);
      MPI_Irecv(intermediate_result_buffer, sizeof(intermediate_t), MPI_CHAR, MPI_ANY_SOURCE, INTERMEDIATE_RESULT_TAG, MPI_COMM_WORLD, &request_intermediate);
    }

    MPI_Test(&request_finished_mapping, &flag_finished_mapping, &status_finished_mapping);
    if(flag_finished_mapping)
    {
        int worker_id = status_finished_mapping.MPI_SOURCE - 2;
        a_process_finished_map(worker_id);
        total_maps_processed++;
        MPI_Irecv(&nothing, 1, MPI_CHAR, MPI_ANY_SOURCE, FINISHED_MAP_TAG, MPI_COMM_WORLD, &request_finished_mapping);
    }

    if(next_work_node)
    {
      try_to_assign_work(&next_work_node);
    }
  }

  DEBUG_PRINT(("FINISHED MAPPING!"));

  map_reduce_result_t map_reduce_result;
  MPI_Request request_finished_reduce;
  MPI_Status status_finished_reduce;
  int flag_finished_reduce;

  MPI_Irecv(&failure_id, 1, MPI_INT, 1, FAIL_TAG, MPI_COMM_WORLD, &request_fail);
  MPI_Irecv(&map_reduce_result, sizeof(map_reduce_result_t), MPI_CHAR, MPI_ANY_SOURCE, FINISHED_REDUCE_TAG, MPI_COMM_WORLD, &request_finished_reduce);

  int 
    total_reduces = g_hash_table_size(intermediate_results),
    total_reduces_processed = 0;

  FILE * result_file = fopen(result_file_name, "w");

  while(total_reduces_processed < total_reduces)
  {
      DEBUG_PRINT(("%d out of %d done", total_reduces_processed, total_reduces));

      MPI_Test(&request_fail, &flag_fail, &status_fail);
      if(flag_fail)
      {
        //handle a failure
        //create new irecv
      }

      MPI_Test(&request_finished_reduce, &flag_finished_reduce, &status_finished_reduce);
      if(flag_finished_reduce)
      {
          int worker_id = status_finished_reduce.MPI_SOURCE - 2;
          a_process_finished_reduce(worker_id);
          fprintf(result_file, "%s: %lld\n", map_reduce_result.word, map_reduce_result.total_frequency);
          total_reduces_processed++;
          MPI_Irecv(&map_reduce_result, sizeof(map_reduce_result_t), MPI_CHAR, MPI_ANY_SOURCE, FINISHED_REDUCE_TAG, MPI_COMM_WORLD, &request_finished_reduce);
      }
      
      try_to_assign_reduce();
  }

  kill_all_slaves();

  fclose(result_file);

  DEBUG_PRINT(("ALL DONE! processed %d reductions", total_reduces_processed));
}

void kill_all_slaves()
{
  int i;
  for(i=0; i<number_of_slaves; ++i)
  {
    char nothing;
    MPI_Send(&nothing, 1, MPI_CHAR, i+2, KILL_TAG, MPI_COMM_WORLD);
  }
}

void send_map_to_slave(map_work_t * work, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
{
  DEBUG_PRINT(("Sent! %d", slave));
  MPI_Send(work, size, datatype, slave, tag, comm);
}

void send_reduce_to_slave(reduce_key_t * key, int size, MPI_Datatype datatype, int slave, int tag, MPI_Comm comm)
{
  DEBUG_PRINT(("Sent! %d", slave));
  MPI_Send(key, size, datatype, slave, tag, comm);
}

int get_total_units(map_work_t ** work_list)
{
  map_work_t ** work_unit_counter = work_list;

  while(*work_unit_counter != NULL)
  work_unit_counter++;
  
  return work_unit_counter - work_list;
}

LinkedList * create_map_work_list(map_reduce_api_spec * f, int argc, char ** argv)
{
  DEBUG_PRINT(("creating map work list..."));

  int start_time = MPI_Wtime();
  LinkedList * work_list = listFromArray(f->create_initial_work(argc, argv));
  int end_time = MPI_Wtime();
  work_creation_time = start_time - end_time;

  DEBUG_PRINT(("created map work in %f seconds!", work_creation_time));
  return work_list;
}
