#ifndef __MW_H__
#define __MW_H__

#include "mw_api.h"

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f);
void be_a_slave(struct mw_api_spec *f);

enum { WORK_TAG, KILL_TAG };

#endif
