#ifndef __MW_H__
#define __MW_H__

#include "map_reduce.h"

void do_master_stuff(int argc, char ** argv, struct map_reduce_api_spec *f);

void do_supervisor_stuff(int argc, char ** argv, struct map_reduce_api_spec *f);

void be_a_slave(int argc, char ** argv, struct map_reduce_api_spec *f);

enum { MAP_WORK_TAG, 
       INTERMEDIATE_RESULT_TAG, 
       FINISHED_MAP_TAG, 
       FINISHED_REDUCE_TAG, 
       REDUCE_KEY_TAG, 
       REDUCE_VALUE_TAG, 
       KILL_TAG, 
       M_PING_TAG, 
       FAIL_TAG, 
       SUPERVISOR_TAG };

#endif
