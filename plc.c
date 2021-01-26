#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "util.h"
#include "command.h"
#include "log.h"



int main(int argc, char *argv[])
{
    /* must call first: init command module, because plog_init() function use it */
    cmd_init();

    /* init plog module */
    plog_init();

    while(1)
    {
        sleep(1);
//        printf("main run!\r\n");
        plog(RUN, "main run!\n");
        log_whi("main run\n");
    }


    return 0;
}
