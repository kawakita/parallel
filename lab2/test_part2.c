#include "mw_api.h"
#include "def_structs_part2.h"
#include <gmp.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#define LARGE_NUM "100"

char* mpz_to_buffer(char* buf, mpz_t num, mpz_t begin, mpz_t end)
{
  char* num_str = mpz_get_str(buf, 10, num);
  gmp_printf("mpz num: %Zd\n", num);
  printf("buf send: %s\n",num_str);
  char* begin_str = mpz_get_str(buf+strlen(num_str)+1, 10, begin);
  printf("beg send: %s\n",begin_str);
  char* end_str = mpz_get_str(buf+strlen(num_str)+strlen(begin_str)+2, 10, end);
  printf("end send: %s\n",end_str);
  return buf;
}

mpz_t* buffer_to_mpz(char* buf)
{
  int i = 0;
  while (buf[i] != '\0')
  {
    i++;
  }
  char* num_str = malloc((i+1)*sizeof(char));
  strncpy(num_str, buf, i);
  num_str[i] = '\0';
  printf("num_str: %s\n", num_str);

  i++;
  int j = 0;
  while (buf[i+j] != '\0')
  {
    j++;
  }
  char* begin_str = malloc((j+1)*sizeof(char));
  strncpy(begin_str, buf+i+1, j);
  begin_str[j] = '\0';

  j++;
  int k = 0;
  while (buf[i+j+k] != '\0')
  {
    k++;
  }
  char* end_str = malloc((k+1)*sizeof(char));
  strncpy(end_str, buf+i+j+1, k);
  end_str[k] = '\0';

  mpz_t* nums = malloc(3*sizeof(mpz_t));
  mpz_init_set_str(nums[0], num_str, 10);
  mpz_init_set_str(nums[1], begin_str, 10);
  mpz_init_set_str(nums[2], end_str, 10);

  free(num_str);
  free(begin_str);
  free(end_str);

  return nums;
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

  gmp_printf("total num_work_units: %Zd\n", num_work_units);

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
  for(i=0; i<=num_work; i++)
  {
    DEBUG_PRINT("creating a new work unit");
    work_list[i] = malloc(sizeof(mw_work_t));
    if (work_list[i] == NULL)
    {
      free(work_list[i]);
      return NULL;
    }

    if (i == (num_work-1))
    {
      mpz_to_buffer(work_list[i]->nums, large_num, num_begin, num_end);
    }
    // create null-terminated work
    else if (i == num_work)
    {
      work_list[i] = NULL;
    }
    else
    {
      mpz_add(sum, num_begin, num_elt_per_work_unit);    
      mpz_to_buffer(work_list[i]->nums, large_num, num_begin, sum);
    }

    //gmp_printf("num_begin: %Zd\n", num_begin);
    //gmp_printf("sum: %Zd\n", sum);

    // reset num_begin to one more than num_end for next work unit
    mpz_add(num_begin, sum, one);
  }
  gmp_printf("created %Zd work units!\n", num_work_units);
  return work_list;
}

int process_results(int sz, mw_result_t * res)
{
  /*
  mpz_t* results = malloc(sizeof(mpz_t));
  if (results == NULL)
  {
    free(results);
    return 0;
  }

  unsigned int capacity = 0;
  unsigned int n = 0;
  unsigned int i;
  for(i=0; i<sz; ++i)
  {
    // see if capacity needs to change
    if ((n + 1 > capacity) || (res->n > capacity))
    {
      if (capacity == 0)
        capacity = 32;
      else if (capacity <= (UINT_MAX/2))
        capacity *= 2;

      mpz_t* temp = realloc(results, capacity * sizeof(mpz_t));
      if (temp == NULL)
      {
        free(temp);
        return 0;
      }
      results = temp;
    }
    
    // add res->f to results array
    memcpy(results, res->f, res->n);
    n += res->n;
  }

  // minimize buffer to n
  mpz_t* minimized = malloc(n*sizeof(mpz_t));
  memcpy(minimized, results, n);
  free(results);
*/
  return 1;
}

mw_result_t * do_work(mw_work_t * work)
{
  DEBUG_PRINT("Doing work...");
  mpz_t* nums = buffer_to_mpz(work->nums);
  mpz_t num, start, end; 
  mpz_init_set(num, nums[0]);
  mpz_init_set(start, nums[1]);
  mpz_init_set(end, nums[2]);

  gmp_printf("num %Zd\n", num);
  gmp_printf("start %Zd\n", start);
  gmp_printf("end %Zd\n", end);

  mpz_t mod, zero, i;
  mpz_init(mod);
  mpz_init_set_ui(zero,0);

  unsigned int capacity = 0;
  unsigned int n = 0;

  mpz_t* factors = malloc(capacity * sizeof(mpz_t));
  if (factors == NULL)
  {
    free(factors);
    return;
  }

  // check if divisors from start to end are factors of num
  DEBUG_PRINT("It's not null");
  mpz_init_set(i, start);
  DEBUG_PRINT("Searching for factors");
  for (; mpz_cmp(i,end); mpz_add_ui(i,i,1))
  {
  	DEBUG_PRINT("computing mod");
    mpz_mod(mod, num, i);
  	DEBUG_PRINT("computed mod");
    if (mpz_cmp(mod, zero) == 0)
    {
	  DEBUG_PRINT("Found a factor!");
      // see if capacity needs to change
      if (n + 1 > capacity)
      {
        if (capacity == 0)
          capacity = 32;
        else if (capacity <= (UINT_MAX/2))
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
      mpz_set(factors[n], i);
      n++;
    }    
  }

  // minimize buffer to n
  mpz_t* minimized = malloc(n*sizeof(mpz_t));
  memcpy(minimized, factors, sizeof(mpz_t) * n);
  free(factors);

  // complete result
  mw_result_t * result = malloc(sizeof(mw_result_t));
  result->f = minimized;
  result->n = n;
  return result;
}

int main (int argc, char **argv)
{
  struct mw_api_spec f;

  // check command line args
  if (argc != 2)
  {
    printf("Invalid input. Provide the granularity, number of elements per unit of work.\n");
    return;
  }

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
