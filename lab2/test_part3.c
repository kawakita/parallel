#include <math.h>
#include <gmp.h>
#include "mw_api.h"
#include "def_structs_part3.h"

mw_work_t ** create_work(int argc, char ** argv)
{
  
  int n_work_units = atoi(argv[1]); //number of units of work the user wishes to divide into
  int line_size = 6; //assume no ints are longer than 6 digits
  int n_ints = 0; //number of lines in the file
  int sz = 2;
  int **all_ints = malloc(sz*sizeof(int)); //array of all ints in the file
  char line[line_size];  //current line in the file
  char infile = argv[2]; //name of file with ints
  mw_work_t ** work_list = calloc(n_work_units, sizeof(mw_work_t*)); //array of work units
  FILE *f; //file with the ints
  int ** some_ints = NULL; //array of nums that comprises a unit of work

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
      all_ints = (int**)realloc(all_ints, sizeof(int*)*sz*2);
      sz = sz*2;
    }
    all_ints[n_ints] = atoi(strdup(line));
    n_ints++;
  }
  
  int ints_per_work_unit = max(2,ceil(n_ints/(float)n_work_units));
  
  //place ints into units of work
  int int_num = 0;
  int work_num = 0;
  some_ints = malloc(ints_per_work_unit*sizeof(int));
  while(int_num<n_ints && work_num<n_work_units) {
    int i = 0;
    while(i<ints_per_work_unit && int_num<n_ints) {
      some_ints[i] = all_ints[int_num];
      i++;
      int_num++;
    }
    work_list[work_num]->inp = some_ints; 
    work_list[work_num]->size = i;
    work_num++;
  }

  return work_list;
}

int process_results(int sz, mw_result_t * res)
{
  int i;
  for(i=0; i<sz; ++i)
  {
    printf("%f\n", res[i].res);
    
  }
}

mw_result_t * do_work(mw_work_t * work)
{
	mw_result_t * result = malloc(sizeof(mw_result_t));
        int size = work->size;
        
	int ** a = malloc(size*sizeof(int));
        a = work->inp;
	return a;
}

int main (int argc, char **argv)
{
  struct mw_api_spec f;

  MPI_Init (&argc, &argv);

  f.create = create_work;
  f.result = process_results;
  f.compute = do_work;
  f.work_sz = sizeof(struct userdef_work_t);
  f.res_sz = sizeof(struct userdef_result_t);

  MW_Run (argc, argv, &f);

  MPI_Finalize ();

  return 0;

}
