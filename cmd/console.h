#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "usrv.h"
//#include "cmd.h"

struct console{
    struct usrv usrv;   /* udp port */
};

void cs_init(struct console *cs);

#endif
