#ifndef __MAP_REDUCE_H__
#define __MAP_REDUCE_H__

#include <mpi.h>
#include "map_reduce_user_def.h"

/* type to apply the map function to */
struct userdef_map_t;

struct userdef_map_intermediate_t;

/* the key value for a reduction */
struct userdef_reduce_key_t;

/* a single value associated with a reduce key */
struct userdef_reduce_val_t;

/* the result of reducing */
struct userdef_result_t;

typedef struct userdef_map_t map_work_t;
typedef struct userdef_map_intermediate_t intermediate_t;
typedef struct userdef_reduce_key_t reduce_key_t;
typedef struct userdef_reduce_val_t reduce_val_t;
typedef struct userdef_result_t map_reduce_result_t;

typedef struct map_reduce_api_spec 
{
    map_work_t ** (* create_initial_work) (int argc, char ** argv);

    int (* map) (map_work_t * map_work, void (* emit_result)(intermediate_t *));

    map_reduce_result_t * (* reduce) (reduce_key_t * reduce_key);

    int work_sz, res_sz, intermediate_sz, intermediate_str_len;
} map_reduce_api_spec;

void MW_Run(int argc, char ** argv, struct map_reduce_api_spec * map_reduce_spec);

#endif
