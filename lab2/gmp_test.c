
#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#define LARGE_NUM "4294967296"
 
int main(void)
{
  mpz_t x;
  mpz_t y;
  mpz_t result;
 
  mpz_init(x);
  mpz_init(y);
  mpz_init(result);
 
  mpz_set_str(x, LARGE_NUM, 10);
  mpz_set_str(y, "9263591128439081", 10);
 
  mpz_mul(result, x, y);
  gmp_printf("\n    %Zd\n*\n    %Zd\n--------------------\n%Zd\n\n", x, y, result);

  mpf_t sq_me, sq_out, test;
  mpf_set_default_prec (10000);
  mpf_init(sq_me);
  mpf_init(sq_out);
  mpf_init(test);
  mpf_set_str (sq_me, "4294967296", 10);

  mpf_sqrt(sq_out, sq_me);
  mpf_mul(test,sq_out,sq_out);

  gmp_printf ("Input:       %Ff\n\n", sq_me);
  gmp_printf ("Square root: %.200Ff\n\n", sq_out);
  gmp_printf ("Re-squared:  %Ff\n\n", test);

  mpz_init_set_ui(x,1);
  mpz_init_set_ui(y,1);
  if (mpz_cmp(x,y) == 0)
    printf("EQUAL!\n");
  else
    printf("DIDN'T WORK\n");    
 
  /* free used memory */
  mpz_clear(x);
  mpz_clear(y);
  mpz_clear(result);
  return EXIT_SUCCESS;
}