#include "mw_api.h"
#include "def_structs_part2.h"
#include <gmp.h>
#include <math.h>

#define LARGE_NUM "4294960296"

mw_work_t ** create_work(int argc, char ** argv)
{
  mpz_t num, sqrt_num_floor, num_begin, granule_sz, num_work_units, sum, one;
  mpf_t numf, sqrt_num, granule_szf, num_work_unitsf;

  mpz_init(num);
  mpz_init(sqrt_num_floor);
  mpz_init(num_begin);
  mpz_init(granule_sz);  
  mpz_init(num_work_units);
  mpz_init(sum);

  mpf_set_default_prec (10000);
  mpf_init(granule_szf);
  mpf_init(sqrt_num);
  mpf_init(num_work_unitsf);

  mpz_set_str(num, LARGE_NUM, 10);
  mpz_init_set_ui(num_begin, 2);
  mpz_init_set_ui(one, 1);
  mpf_set_z(numf, num);
  mpf_sqrt(sqrt_num, numf);
  mpz_set_f(sqrt_num_floor, sqrt_num);

  int num_granules = atoi(argv[1]);
  mpz_import(num_work_units, 1, -1, sizeof(int), 0, 0, &num_granules);
  mpf_set_z(num_work_unitsf, num_work_units);

  mpf_div(granule_szf, numf, nxum_work_unitsf);
  mpz_set_f(granule_sz, granule_szf);
  // substract 1 from granule_sz since it will be added to the start of each range
  mpz_sub(granule_sz, granule_sz, one);

  mw_work_t ** work_list = malloc(num_granules*sizeof(mw_work_t*));
  int i=0;
  for(i=0; i<num_granules; ++i)
  {
    work_list[i] = malloc(sizeof(mw_work_t));
    work_list[i]->num = num;
    work_list[i]->start = num_begin;
    if (i != num_granules-1)
    {
      mpz_add(sum, num_begin, granule_sz);    
      work_list[i]->end = sum;
    }    
    else
    {
      mpz_sub(sum, num, one);
      work_list[i]->end = sum;
    }
    mpz_add(num_begin, sum, one);
  }
  return work_list;
}

int process_results(int sz, mw_result_t * res)
{
  int * results = malloc(sizeof(int));
  unsigned int capacity = 0;
  unsigned int n = 0;
  int i;
  for(i=0; i<sz; ++i)
  {
    if (n+1 > capacity)
    {
      // determine length of factors in each work unit
      sizeof(
      if (capacity == 0)
        capacity = 32;
      else if (capacity <= (UINT_MAX/2))
        capacity *= 2;
    }

    
    realloc(results, capacity*sizeof(int));
    res[i].factors 
  }
}

mw_result_t * do_work(mw_work_t * work)
{
  mpz_t mod;
  mpz_init(mod);
  mpz_init_set_ui(zero,0);

  unsigned int capacity = 0;
  unsigned int n = 0;
  mpz_t* factors = malloc(capacity * sizeof(mpz_t));

  // check if divisors from start to end are factors of num
  for (mpz_init_set(i,work->start); mpz_cmp(i,mpz_work->end); mpz_add_ui(i,i,1))
  {
    mpz_mod(mod, mpz_work->num, i);
    if (mpz_cmp(mod, zero) == 0)
    {
      if (n + 1 > capacity)
      {
        if (capacity == 0)
          capacity = 32;
        else if (capacity <= (UINT_MAX/2))
          capacity *= 2;

        mpz_t* temp = realloc(factors, capacity * sizeof(mpz_t));
        factors = temp;
      }
      factors[n] = i;
      n++;
    }    
  }

  // minimize buffer
  mpz_t* minimized = malloc(n*sizeof(mpz_t));
  memcpy(minimized, factors, n);
  free(factors);

  // complete result
  mw_result_t * result = malloc(sizeof(mw_result_t));
  result->f = minimized;
  return result;
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
