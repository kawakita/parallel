#include <math.h>
#include <gmp.h>
#include "mw_api.h"
#include "def_structs_part3.h"
#include <string.h>

mw_work_t ** create_work(int argc, char ** argv)
{
  
  int lines_each;
  char infile[30]; //name of file with ints
  strcpy(infile,argv[1]);  
  lines_each = atoi(argv[2]);

  printf("%d ints in each work unit!\n",lines_each);
  
  FILE *f; //file with the ints
  
  int lines=0;
  //OPEN file
  f = fopen(infile,"r");
  while(EOF != (fscanf(f,"%*[^\n]"),fscanf(f,"%*c")))
    ++lines;
  //printf("%d lines\n",lines);

  if (lines < lines_each) lines_each = lines;
  int n_units = lines/lines_each;
  if (lines % lines_each) n_units++;

  mw_work_t ** work_list =  calloc(n_units+1, sizeof(mw_work_t*)); //array of work units

  int work_num = 0;
  printf("%d lines each from file %s \n",lines_each, infile);
  printf("%d units of work to do\n", n_units);
  while(work_num < n_units) {
    
    work_list[work_num] = malloc(sizeof(mw_work_t));
    strcpy(work_list[work_num]->filename, infile);
    //puts(work_list[work_num]->filename);
    work_list[work_num]->first_line = work_num*lines_each;
    work_list[work_num]->lines_each = lines_each;
    work_num++;
    //printf("worknum %d created\n",work_num); 
    
    }
  printf("alles clar\n");
  fclose(f);
  return work_list;
}

int process_results(int sz, mw_result_t * res)
{
  printf("starting to process results\n");
  int i;
  int max = res[0].max;
  printf("maybe the max is %d\n",max);
  for(i=1; i<sz; ++i)
  {
    if(res[i].max > max) max = res[i].max;
    
  }
  printf("The global max is %d\n",max);
}

mw_result_t * do_work(mw_work_t * work)
{
  int current;
  mw_result_t * result = malloc(sizeof(mw_result_t));
  FILE *f;
  f = fopen(work->filename,"r");
  printf("Doing work\n");
  int line=0;
  while(EOF != (fscanf(f,"%*d")) && line<work->first_line)
    ++line;
  while(EOF != fscanf(f,"%d",&current) && line<work->first_line+work->lines_each) 
  {
    if (result->max == NULL || result->max < current) result->max = current;
    ++line;
  }
  printf("my max is %d\n",result->max);
  fclose(f);
  return result;
}

int main (int argc, char **argv)
{
  struct mw_api_spec f;

  MPI_Init (&argc, &argv);  

  f.create = create_work;
  f.compute = do_work;
  f.result = process_results;
  f.work_sz = sizeof(struct userdef_work_t);
  f.res_sz = sizeof(struct userdef_result_t);
  
  MW_Run (argc, argv, &f);

  MPI_Finalize ();

  return 0;

}
