#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "util.h"
#include "command.h"
#include "log.h"



int main(int argc, char *argv[])
{
    /* init plog module */
    plog_init();
    
    /* init command module */
    cmd_init();

    while(1)
    {
        sleep(1);
//        printf("main run!\r\n");
        plog(RUN, "main run!\n");
        log_whi("main run\n");
    }


    return 0;
}
