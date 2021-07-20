#ifndef __TICK_H__
#define __TICK_H__

#include "btype.h"


typedef void (*tknode_cb_t)(void *udata);

void tick_init(void);
int tick_add(char *name, tknode_cb_t cb, void *udata, u16_t div);
int tick_rmv(char *name);
u16_t tick_dump(char *buff, u16_t len);

#endif

