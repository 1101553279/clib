#ifndef __CFG_H__
#define __CFG_H__

#include "btype.h"

void cfg_init(void);
int cfg_read(char *sec, char *key, void *value);

u16_t cfg_dump(char *buff, u16_t len);
#endif
