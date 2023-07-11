#include "udp_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cmd/command.h"
#include <time.h>
#include "log/log.h"
#include "cmd/argtable3.h"
#include "util/util.h"
#include "util/blist.h"
#include "udp_driver.h"
#include "udp_device.h"
#include "argudp.h"

static int logudp_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds);

void log_udp_init(void)
{
    log_udp_drv_init();
    
    arg_cmd_register("logudp", logudp_arg_cmdfn, "manage log udp module");

    return;
}

/************************************************************
 logudp [-h]
 logudp --drv
 logudp --dev [-i id]                               # output device information
 logudp --dev -c|--create [--srv ip --port port]    # create
 logudp --dev -d|--del -i id                        # delete
 logudp --dev -s|--set -i id                        # set
 logudp --dev -r|--reset -i id                      # reset

*************************************************************/
static int logudp_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds)
{
    int ret;
    void *argtable[] = 
    {
        argudp_arg_h           = arg_lit0("h", "help",     "output help information about udp log"),
        argudp_arg_c           = arg_lit0("c", "create",   "create a log udp device"),
        argudp_arg_d           = arg_lit0("d", "del",      "delete a log udp device"),
        argudp_arg_s           = arg_lit0("s", "set",      "set parameters for log udp device"),
        argudp_arg_r           = arg_lit0("r", "reset",    "reset a log udp device"),
        argudp_arg_a           = arg_lit0("a", "all",      "all udp devices"),
        argudp_arg_drv         = arg_lit0(NULL, "drv",     "manage log udp driver part"),
        argudp_arg_dev         = arg_lit0(NULL, "dev",     "manage log udp device part"),
        argudp_arg_srv         = arg_str0(NULL, "srv",     "ip", "server ip"),
        argudp_arg_port        = arg_int0(NULL, "port",    "port", "server port"),
        argudp_arg_i           = arg_intn("i", NULL,       "id", 0, 5, "device id"),
        argudp_end_arg         = arg_end(10),
    };

    ret = argtable_parse(argc, argv, argtable, argudp_end_arg, ds, argv[0]);
    if(0 != ret)
        goto to_parse;

    if(argudp_arg_h->count > 0)
    {
        log_argudp_help(argtable, ds, argv[0]);
        goto to_h;
    }
    
    if(argudp_arg_drv->count > 0)
    {
        logudp_cmd_drv(ds);
        goto to_drv;
    }
    
    if(argudp_arg_dev->count > 0)
    {
        logudp_cmd_dev(ds);
        goto to_dev;
    }

    log_argudp_help(argtable, ds, argv[0]);
to_dev:
to_drv:
to_h:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}
