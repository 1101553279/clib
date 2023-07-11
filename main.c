#include <stdio.h>
#include <unistd.h>
#include "log/plog.h"
#include "init/init.h"
#include "cfg/cfg.h"
#include "util/dstr.h"
#define LOG_CATEGORY LOGC_RUN
#include "log/log.h"

int main(int argc, char *argv[])
{
    struct cfg_section *sec;
    struct cfg_proper *pro;

    init_all();

//    cfg_print();
    sec = cfg_section_find("cmd");
    cfg_section_print(sec);

    pro = cfg_proper_find("cmd", "name");
    cfg_proper_print(pro);
    pro = cfg_proper_find("plog", "name");
    cfg_proper_print(pro);


    while(1)
    {
        usleep(1000*1000);
        plog(RUN, "main run!\n");
        log_debug("main is running!\n");
    }

    return 0;
}
