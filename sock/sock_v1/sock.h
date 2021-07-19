#ifndef __SOCK_H__
#define __SOCK_H__
#include "tcps.h"
#include "btype.h"

int sock_init(void);

u16_t sock_tcps_dump(char *buff, u16_t len);
#endif
