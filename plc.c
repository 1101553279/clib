#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "util.h"
#include "command.h"
#include "log.h"
#include "sock.h"
#include "tick.h"

int main(int argc, char *argv[])
{
    /* must call first: init command module, because plog_init() function use it */
    cmd_init();

    /* init tick module */
    tick_init();

    /* init plog module */
    plog_init();

    /* init sock module */
    sock_init();

    while(1)
    {
        usleep(1000*1000);
        plog(RUN, "main run!\n");
    }


    return 0;
}
