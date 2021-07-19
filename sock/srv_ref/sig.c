#include "sig.h"
#include <stdio.h>
#include "log.h"


int sig_reg(int signum, sig_cb_t cb)
{
    struct sigaction act;
    act.sa_handler = NULL;
    act.sa_sigaction = cb;
    sigemptyset(&act.sa_mask);

    /* install a action for one signal */
    sigaction(signum, &act, NULL);
    
    log_grn("signal \'%d\' been installed!\r\n", signum);
    return 0;
}

