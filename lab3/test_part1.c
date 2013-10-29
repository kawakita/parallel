#include <math.h>
#include <gmp.h>
#include "mw_api.h"
#include "def_structs.h"

mw_work_t ** create_work(int argc, char ** arv)
{
  const int total_work = 100;
  mw_work_t ** work_list = calloc(total_work, sizeof(mw_work_t*) + 1);
  int i=0;
  for(i=0; i<total_work; ++i)
  {
    work_list[i] = malloc(sizeof(mw_work_t));
    work_list[i]->x = i;
  }
  return work_list;
}

int process_results(int sz, mw_result_t * res)
{
  int i;
  for(i=0; i<sz; ++i)
  {
    printf("%f\n", res[i].k);
    if(res[i].k == M_PI)
      printf("Found some pi!\n");
    else
      printf("No pi here :(\n");
  }
  return 0;
}

mw_result_t * do_work(mw_work_t * work)
{
  mw_result_t * result = malloc(sizeof(mw_result_t));
  result->k = M_PI;
  printf("created result %f\n", M_PI);
  return result;
}


char * result_to_str(mw_result_t result)
{
  char s[32];
  double d = result.k;

  sprintf(s, "%f", d);
  return s;
}

mw_result_t str_to_result(char * s)
{
  double d = atof(s);
  mw_result_t result;
  result.k = d;
  return result;
}

int main (int argc, char **argv)
{
  printf("hello\n");
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


