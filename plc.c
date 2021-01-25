#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "util.h"



int main(int argc, char *argv[])
{
    /* init plog module */
    plog_init();


    while(1)
    {
        sleep(1);
//        printf("main run!\r\n");
        plog(RUN, "main run!\n");
    }


    return 0;
}
