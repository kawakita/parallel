#ifndef __MAP_REDUCE_USER_DEF_H__
#define __MAP_REDUCE_USER_DEF_H__

#include "map_reduce.h"

struct userdef_map_t
{
    char filename[150];
};

struct userdef_map_intermediate_t
{
    char word[100];
    int frequency;
};

struct userdef_reduce_key_t
{
    char word[100];
};

struct userdef_reduce_val_t
{
    int frequency;
};

struct userdef_result_t
{
    char word[100];
    long long total_frequency;
};

#endif
