#include "init.h"
#include "util/util.h"
#include "cfg/cfg.h"
#include "cmd/command.h"
#include "log/log.h"
#include "sock/sock.h"
#include "tick/tick.h"

typedef void (* init_t)(void *arg);

void init_first(void *arg)
{
    /* init configure module */
    cfg_init();

    /* must call first: init command module, because plog_init() function use it */
    cmd_init();

    /* init tick module */
    tick_init();

    /* init plog module */
    plog_init();
    log_init();

    /* init sock module */
    sock_init();

    return;
}

void init_last(void *arg)
{
    plog_init_append();
    log_init_append();
    cfg_init_append();
    tick_init_append();
    sock_init_append();

    return;
}

static init_t init_table[] = {init_first, init_last};

void init_all(void)
{
    int i = 0;

    for(i=0; i < ARY_SIZE(init_table); i++)
        init_table[i](NULL);

    return;
}
