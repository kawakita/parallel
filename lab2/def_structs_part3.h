#ifndef __DEF_STRUCTS_H__
#define __DEF_STRUCTS_H__

struct userdef_work_t
{
  char filename[30];
  int first_line;
  int lines_each;
};

struct userdef_result_t
{
  int max;
};

#endif
