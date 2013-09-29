#ifndef __DEF_STRUCTS_H__
#define __DEF_STRUCTS_H__

#include <gmp.h>

struct userdef_work_t
{
  mpz_t num;
  mpz_t start;
  mpz_t end;
};

struct userdef_result_t
{
  mpz_t* f;
  unsigned int n;
};

#endif
