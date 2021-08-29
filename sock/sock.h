#ifndef __SOCK_H__
#define __SOCK_H__
#include "tcps.h"
#include "btype.h"

int sock_init(void);
void sock_init_append(void);

u16_t sock_tcps_dump(char *buff, u16_t len);
#endif
