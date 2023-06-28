#ifndef __TICK_H__
#define __TICK_H__

#include "include/btype.h"
#include "cmd/argtable3.h"

typedef void (*tknode_cb_t)(void *udata);

void tick_init(void);
void tick_init_append(void);
int tick_add(char *name, tknode_cb_t cb, void *udata, u16_t div);
int tick_rmv(char *name);
int tick_dump(arg_dstr_t ds);

#endif

