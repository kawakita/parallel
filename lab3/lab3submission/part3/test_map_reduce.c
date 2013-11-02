#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <assert.h>

#include "map_reduce_user_def.h"
#include "map_reduce.h"
#include "debug.h"

static char * filenames[] = { "moby_dick_part_aa",
                              "moby_dick_part_ab",
                              "moby_dick_part_ac",
                              "moby_dick_part_ad",
                              "moby_dick_part_ae",
                              "moby_dick_part_af",
                              "moby_dick_part_ag",
                              "moby_dick_part_ah",
                              "moby_dick_part_ai",
                              "moby_dick_part_aj",
                              "moby_dick_part_ak",
                              "moby_dick_part_al" };
    
map_work_t ** create_work(int argc, char ** argv)
{
    int list_length = sizeof(filenames) / sizeof(char*);
    DEBUG_PRINT(("List len: %d", list_length));
    map_work_t ** work_array = malloc(sizeof(map_work_t*) * (list_length + 1));
    assert(work_array);
    int i;
    for(i=0; i<list_length; ++i)
    {
        work_array[i] = malloc(sizeof(map_work_t));
        assert(work_array[i]);
        strcpy(work_array[i]->filename, filenames[i]);
        DEBUG_PRINT(("created work %s", filenames[i]));
    }
    work_array[list_length] = NULL;
    return work_array;
}

int map_function(map_work_t * map_work, void (*emit_result)(intermediate_t *))
{
    char 
        full_filename[500],
        directory[150];

    getcwd(directory, 150);
    sprintf(full_filename, "%s/%s", directory, map_work->filename);
    DEBUG_PRINT(("About to process file [%s] (strlen=%d)", full_filename, strlen(full_filename)));
    FILE * text_file = fopen(full_filename, "r");
    if(text_file == NULL)
    {
        DEBUG_PRINT(("Failed to open file [%s]", full_filename));
        return 1;
    }
    else
    {
        DEBUG_PRINT(("File [%s] opened!", full_filename));
    }
    char
        * delimiters = " ;:/[]()!@#$%^&*.,-\\\"\'\t\n",
        line[500];
   

    while(fgets(line, 500, text_file) != NULL)
    {
        char * token = strtok(line, delimiters);
        while(token != NULL)
        {
            intermediate_t intermediate_result;
            strcpy(intermediate_result.word, token);
            intermediate_result.frequency = 1;
            emit_result(&intermediate_result);
            token = strtok(NULL, delimiters);
        }
    }
    DEBUG_PRINT(("Finshed map"));
    return 0;
}

int main(int argc, char ** argv)
{
    MPI_Init(&argc, &argv);

    struct map_reduce_api_spec map_reduce_spec;

    map_reduce_spec.create_initial_work = create_work;
    map_reduce_spec.map = map_function;

    MW_Run(argc, argv, &map_reduce_spec);

    MPI_Finalize();

    return 0;
}
