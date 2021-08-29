#ifndef __CFG_H__
#define __CFG_H__

#include "btype.h"


/* init cfg module */
void cfg_init(void);
void cfg_init_append(void);
int cfg_read(const char *sec, const char *key, char** value);

/* for debug */
u16_t cfg_dump(char *buff, u16_t len);
#endif
