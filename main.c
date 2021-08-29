#include <stdio.h>
#include <unistd.h>
#include "plog.h"
#include "init/init.h"
#include "cfg.h"

int main(int argc, char *argv[])
{
    struct cfg_section *sec;
    struct cfg_proper *pro;

    init_all();

//    cfg_print();
    sec = cfg_section_find("cmd");
    cfg_section_print(sec);
    sec = cfg_section_find("cmd2");
    if(NULL == sec)
        printf("section(cmd2) is not found!\r\n");

    pro = cfg_proper_find("cmd", "name");
    cfg_proper_print(pro);
    pro = cfg_proper_find("plog", "name");
    cfg_proper_print(pro);
    pro = cfg_proper_find("plog2", "name");
    if(NULL == sec)
        printf("proper(%s:%s) is not found!\r\n", "plog2", "name");


    while(1)
    {
        usleep(1000*1000);
        plog(RUN, "main run!\n");
    }

    return 0;
}
