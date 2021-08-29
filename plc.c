#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "init/init.h"

int main(int argc, char *argv[])
{
    init_all();

    while(1)
    {
        usleep(1000*1000);
        plog(RUN, "main run!\n");
    }

    return 0;
}
