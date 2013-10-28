#include <math.h>
#include <gmp.h>
//#include <glib.h>
#include "mw_api.h"
#include "def_structs.h"
#include "debug.h"

#define DEBUG 0

mw_work_t ** create_work(int argc, char ** arv)
{
  const int total_work = 25;
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
  DEBUG_PRINT(("processing results!"));
  int i;
  for(i=0; i<sz; ++i)
  {
    DEBUG_PRINT(("%f", res[i].k));
    DEBUG_TEST(res[i].k == M_PI);
  }
  return 0;
}

mw_result_t * do_work(mw_work_t * work)
{
  mw_result_t * result = malloc(sizeof(mw_result_t));
  result->k = M_PI;
  //system("sleep 0.1");
  DEBUG_PRINT(("created result %f\n", M_PI));
  return result;
}

int main (int argc, char **argv)
{
  DEBUG_PRINT(("starting :)"));
  struct mw_api_spec f;

  MPI_Init (&argc, &argv);

  //GHashTable * table = g_hash_table_new(g_str_hash, g_str_equal);

  f.create = create_work;
  f.result = process_results;
  f.compute = do_work;
  f.work_sz = sizeof(struct userdef_work_t);
  f.res_sz = sizeof(struct userdef_result_t);

  MW_Run (argc, argv, &f);

  MPI_Finalize ();

  return 0;

}
