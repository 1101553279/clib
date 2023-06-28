#include "argudp.h"
#include "udp_device.h"
#include "udp_driver.h"

/************************************************************
 logudp [-h]
 logudp --drv
 logudp --dev [-i id]                               # output device information
 logudp --dev -c|--create [--srv ip --port port]    # create
 logudp --dev -d|--del -i id                        # delete
 logudp --dev -s|--set -i id                        # set
 logudp --dev -r|--reset -i id                      # reset
 logudp --dev -a            # all devices

*************************************************************/
struct arg_lit *argudp_arg_h;
struct arg_lit *argudp_arg_c;
struct arg_lit *argudp_arg_d;
struct arg_lit *argudp_arg_s;
struct arg_lit *argudp_arg_r;
struct arg_lit *argudp_arg_a;
struct arg_lit *argudp_arg_drv;
struct arg_lit *argudp_arg_dev;
struct arg_str *argudp_arg_srv;
struct arg_int *argudp_arg_port;
struct arg_int *argudp_arg_i;
struct arg_end *argudp_end_arg;

arg_dstr_t log_argudp_help(void *argtable, arg_dstr_t ds, const char *name)
{
/* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", name);
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    logudp_drv_help(ds, name);    
    logudp_dev_help(ds, name);
    arg_print_glossary_ds(ds, argtable, "\t%-15s %s\r\n");
}
