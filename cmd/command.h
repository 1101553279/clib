#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "include/btype.h"
#include "util/blist.h"
#include "log/plog.h"



/* init command module */
void cmd_init(void);

/* command manager information */
int cmd_srv_fd(void);

/* sender(PC host) information */
int cmd_client(struct sockaddr_in *addr, socklen_t *len);
void cmd_uninit(void);

int cmd_dump(arg_dstr_t ds);
#endif
