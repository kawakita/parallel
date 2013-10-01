#include <math.h>
#include <gmp.h>
#include "mw_api.h"
#include "def_structs_part3_max.h"
#include <string.h>


mw_work_t ** create_work(int argc, char ** argv)
{

  int n_work_units = atoi(argv[1]); //number of units of work the user wishes to divide into
  int line_size = 6; //assume no ints are longer than 6 digits
  int n_ints = 0; //number of lines in the file
  int sz = 2;
  int *all_ints = malloc(sz*sizeof(int)); //array of all ints in the file
  char line[line_size];  //current line in the file
  char *infile = argv[2]; //name of file with ints
  mw_work_t ** work_list = calloc(n_work_units, sizeof(mw_work_t*)); //array of work units
  FILE *f; //file with the ints

  //OPEN file
  f = fopen(infile,"r");
  if (f == NULL) {
    printf("File not found");
    return work_list;
  }

  //read lines into an array
  //array is a resizing array which doubles to grow
  while((fgets(line, line_size, f) != NULL)) {
    if (n_ints>sz) {
      all_ints = realloc(all_ints, sizeof(int*)*sz*2);
      sz = sz*2;
    }
    all_ints[n_ints] = atoi(strdup(line));
    n_ints++;
  }

  //if the user specifies too many units of work relative to the file size
  if (n_ints/2 < n_work_units)
    n_work_units = n_ints/2;

  int ints_per_work_unit = ceil(n_ints/(float)n_work_units);

  //place ints into units of work
  int work_num = 0;

  while(work_num<n_work_units) {

    work_list[work_num]->inp = all_ints;
    work_list[work_num]->start = work_num*ints_per_work_unit;
    if ((work_num+1)*ints_per_work_unit < n_ints)
      work_list[work_num]->end = (work_num+1)*ints_per_work_unit - 1;
    else 
      work_list[work_num]->end = n_ints - 1;
    work_num++;
  }

  return work_list;
}


mw_result_t * do_work(mw_work_t * work)
{
   mw_result_t * result = malloc(sizeof(mw_result_t));
   int *a = work->inp;
   int start = work->start;
   int end = work->end;
   int i;
   int max = a[start];
   for (i = start+1; i<= end; i++)
     if (a[i] > max)
	max = a[i];
   result->max = max;
   return result;
}



int process_results(int sz, mw_result_t * res)
{
  int i;
  int max = res[0].max;
  for(i=1; i<sz; i++)
  {
	 if (res[i].max > max)
	 	max = res[i].max;
  }

  printf("%d",max);
  return max;
}



int main (int argc, char **argv)
{
  struct mw_api_spec f;

  MPI_Init (&argc, &argv);
  double start_time = MPI_Wtime();

  f.create = create_work;
  f.result = process_results;
  f.compute = do_work;
  f.work_sz = sizeof(struct userdef_work_t);
  f.res_sz = sizeof(struct userdef_result_t);

  MW_Run (argc, argv, &f);

  MPI_Finalize ();
  double elapsed_time = MPI_Wtime() - start_time;

  return 0;

}

