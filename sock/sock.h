#ifndef __SOCK_H__
#define __SOCK_H__
#include "tcps.h"
#include "include/btype.h"
#include "cmd/argtable3.h"

int sock_init(void);
void sock_init_append(void);

int sock_tcps_dump(arg_dstr_t ds);
#endif
