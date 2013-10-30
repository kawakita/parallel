#include "mw_api.h"
#include "def_structs.h"
#include <gmp.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#define LARGE_NUM "68719476736"
//#define LARGE_NUM "100"

char* mpz_to_buffer(char* buf, mpz_t* nums, unsigned int n)
{
  unsigned int i;
  char * next_mpz_str = buf;
  for (i=0; i<n; i++)
  {
    mpz_get_str(next_mpz_str, 10, nums[i]);
    next_mpz_str += (strlen(next_mpz_str)+1);
  }
  return buf;
}

mpz_t* buffer_to_mpz(char* buf, unsigned int n)
{
  mpz_t * mpzs = malloc(sizeof(mpz_t) * n);
  int i = 0;
  while(i < n)
  {
  	mpz_init_set_str(mpzs[i], buf, 10);
	buf += strlen(buf) + 1;
	++i;
  }
  return mpzs;
}

mw_work_t ** create_work(int argc, char ** argv)
{
  mpz_t 
  	zero, 
	one, 
	large_num, 
	num_begin, 
	num_end, 
    num_elt_per_work_unit, 
	mod,
    num_work_units, 
	num_end_min_num_begin, 
	sum;
  
  mpf_t      
  	large_numf, 
	sqrt_large_num;

  // initialize 0 and 1
  mpz_init_set_ui(zero, 0);
  mpz_init_set_ui(one, 1);

  // take in command line arg for number of elements per unit of work
  unsigned int num_elt_per_work = (unsigned int) atoi(argv[1]);
  mpz_init_set_ui(num_elt_per_work_unit, num_elt_per_work);

  // set initial LARGE_NUM to find factors
  mpz_init_set_str(large_num, LARGE_NUM, 10);

  gmp_printf("%Zd\n", large_num);

  // start possible factors at 2
  mpz_init_set_ui(num_begin, 2);   

  // end possible factors at sqrt(large_num)
  mpf_set_default_prec (10000);
  mpf_init(large_numf);
  mpf_set_z(large_numf, large_num);
  mpf_init(sqrt_large_num);
  mpf_sqrt(sqrt_large_num, large_numf);
  mpz_init(num_end);
  mpz_set_f(num_end, sqrt_large_num);

  // determine number of work units required for complete list of work
  // given num elts per work unit
  mpz_init(num_end_min_num_begin);
  mpz_sub(num_end_min_num_begin, num_end, num_begin);
  // divide num_end-num_begin by num_elt_per_work_unit to get num_work_units
  mpz_init(num_work_units);
  mpz_div(num_work_units, num_end_min_num_begin, num_elt_per_work_unit);

  // determine if an extra work unit is needed when large_num isn't divisible by num_elt_per_work_unit
  mpz_init(mod);
  mpz_mod(mod, num_end_min_num_begin, num_elt_per_work_unit);
  // if mod not zero, add one
  if (mpz_cmp(mod, zero) > 0)
    mpz_add(num_work_units, num_work_units, one);

  unsigned int num_work = mpz_get_ui(num_work_units);
  
  // sum for incrementing num_begin and num_end
  mpz_init(sum);

  // allocate work list plus 1 for NULL
  mw_work_t ** work_list = malloc((num_work+1)*sizeof(mw_work_t*));
  if (work_list == NULL)
  {
    free(work_list);
    return NULL;
  }

  // decrement  num_elt_per_work_unit by 1
  mpz_sub(num_elt_per_work_unit, num_elt_per_work_unit, one);

  unsigned int i=0;
  for(i=0; i<num_work; i++)
  {
    work_list[i] = malloc(sizeof(mw_work_t));
    if (work_list[i] == NULL)
    {
      free(work_list[i]);
      return NULL;
    }

    mpz_t nums[3];
    mpz_init_set(nums[0], large_num);
    mpz_init_set(nums[1], num_begin);
    mpz_add(sum, num_begin, num_elt_per_work_unit);    
    mpz_init_set(nums[2], sum);

    if (i == (num_work-1))
    {
      mpz_set(nums[2], num_end);
    }

    mpz_to_buffer(work_list[i]->nums, nums, 3);
	
    // reset num_begin to one more than num_end for next work unit
    mpz_add(num_begin, sum, one);
  }
  work_list[i] = NULL;

  return work_list;
}

int process_results(int sz, mw_result_t * res)
{
  int 
  	num_factors = 0,
	i=0;
	
  for(i=0; i<sz; ++i)
  {
  	num_factors += res[i].n;
  }

  mpz_t * all_factors = malloc(sizeof(mpz_t) * num_factors);

  int current_factor = 0;
  for(i=0; i<sz; ++i)
  {
    mpz_t * factors = buffer_to_mpz(res[i].nums, res[i].n);
	int j;
	for(j=0; j<res[i].n; ++j)
	  mpz_init_set(all_factors[current_factor++], factors[j]);
  }

  for(i=0; i<num_factors; ++i)
  {
    gmp_printf("%Zd\n", all_factors[i]);
  }
}

mw_result_t * do_work(mw_work_t * work)
{
  mpz_t* nums = buffer_to_mpz(work->nums, 3);
  mpz_t num, start, end; 
  mpz_init_set(num, nums[0]);
  mpz_init_set(start, nums[1]);
  mpz_init_set(end, nums[2]);

  mpz_t mod, zero, i;
  mpz_init(mod);
  mpz_init_set_ui(zero,0);

  unsigned int capacity = 32;
  unsigned int n = 0;

  mpz_t* factors = malloc(capacity * sizeof(mpz_t));
  if (factors == NULL)
  {
    free(factors);
    return;
  }

  // check if divisors from start to end are factors of num
  mpz_init_set(i, start);
  for (; mpz_cmp(i,end); mpz_add_ui(i,i,1))
  {
    mpz_mod(mod, num, i);
    if (mpz_cmp(mod, zero) == 0)
    {
      // see if capacity needs to change
      if (n + 1 > capacity)
      {
        if (capacity <= (UINT_MAX/2))
          capacity *= 2;

        mpz_t* temp = realloc(factors, capacity * sizeof(mpz_t));
        if (temp == NULL)
        {
          free(temp);
          return NULL;
        }
        factors = temp;
      }
      // add factor to list and update n
      mpz_init_set(factors[n], i);
      n++;
    }    
  }

  mw_result_t * result = malloc(sizeof(mw_result_t));
  result->n = n;

  if(n != 0)
  {
	  factors = realloc(factors, sizeof(mpz_t) * n);
	  mpz_to_buffer(result->nums, factors, n);
  }
  return result;
}

char * result_to_str(mw_result_t result)
{
  char * s = malloc(sizeof(char)*1000);
  memcpy(s, result.nums, 1000);

  // if no results, make it begin with null
  if (result.n == 0)
  {
    s[0] = '\0';
  }  
  else
  {
    // only replace commas for number of results-1 to maintain null terminus
    int i = 0, num_null = 0;
    while(num_null < (result.n-1))
    {
      if(s[i] == '\0')
      {
        s[i] = ',';
        num_null++;
      }
      i++;
    }
  }
  return s;
}

mw_result_t* str_to_result(char * s)
{
  mw_result_t* result = malloc(sizeof(mw_result_t));
  int i = 0, num_commas = 0;
  if (s[0] == '\0')
  {  
    strcpy(result->nums,s);
    result->n = 0;
  }
  else
  {
    while(s[i] != '\0')
    {
      if (s[i] == ',')
      {
        s[i] = '\0';
        num_commas++;
      }
      i++;
    }
    strcpy(result->nums,s);
    result->n = num_commas + 1;
  }

  return result;
}


int main (int argc, char **argv)
{
  struct mw_api_spec f;

  // check command line args
  if (argc != 2)
  {
    printf("Invalid input. Provide the granularity, number of elements per unit of work.\n");
    return -1;
  }

  MPI_Init (&argc, &argv);

  f.create = create_work;
  f.result = process_results;
  f.compute = do_work;
  f.to_str = result_to_str;
  f.from_str = str_to_result;
  f.work_sz = sizeof(struct userdef_work_t);
  f.res_sz = sizeof(struct userdef_result_t);

  double start, end;

  MW_Run (argc, argv, &f);

  MPI_Finalize ();

  return 0;
}

