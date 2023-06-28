#ifndef __LOG_UDP_DRIVER_H__
#define __LOG_UDP_DRIVER_H__

#include "include/btype.h"
#include "cmd/argtable3.h"

void log_udp_drv_init(void);

arg_dstr_t logudp_cmd_drv(arg_dstr_t ds);

arg_dstr_t logudp_drv_help(arg_dstr_t ds, const char *name);

#endif
