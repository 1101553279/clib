#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <signal.h>

/* void (sa_sigaction)(int, siginfo_t *, void *); */
typedef void(*sig_cb_t)(int num, siginfo_t *info, void *context);

int sig_reg(int signum, sig_cb_t cb);


#endif
