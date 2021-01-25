#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "console.h"
#include "util.h"

struct plc{
    struct console cs;
};

struct plc plc_obj;

int main(int argc, char *argv[])
{
    /* init plog module */
    plog_init();
    /* init console module */
    cs_init(&(plc_obj).cs); 


    while(1)
    {
        sleep(1);
//        printf("main run!\r\n");
        plog(RUN, "main run!\n");
    }


    return 0;
}
