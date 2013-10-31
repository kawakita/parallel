#ifndef __MW_H__
#define __MW_H__

#include "mw_api.h"

void do_master_stuff(int argc, char ** argv, struct mw_api_spec *f);

void do_supervisor_stuff(int argc, char ** argv, struct mw_api_spec *f);

void be_a_slave(int argc, char ** argv, struct mw_api_spec *f);

enum { WORK_TAG, KILL_TAG, M_PING_TAG, FAIL_TAG, SUPERVISOR_TAG };

#endif