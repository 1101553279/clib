#ifndef __LOG_ARGUDP_H__
#define __LOG_ARGUDP_H__

#include "cmd/argtable3.h"




extern struct arg_lit *argudp_arg_h;
extern struct arg_lit *argudp_arg_dev;
extern struct arg_lit *argudp_arg_drv;
extern struct arg_lit *argudp_arg_c;
extern struct arg_int *argudp_arg_i;
extern struct arg_str *argudp_arg_srv;
extern struct arg_int *argudp_arg_port;
extern struct arg_lit *argudp_arg_d;
extern struct arg_lit *argudp_arg_s;
extern struct arg_lit *argudp_arg_r;
extern struct arg_lit *argudp_arg_a;
extern struct arg_end *argudp_end_arg;

arg_dstr_t log_argudp_help(void *argtable, arg_dstr_t ds, const char *name);
#endif
