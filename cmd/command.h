#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "btype.h"
#include "blist.h"

struct cmd_sender{
    struct sockaddr_in addr;    /* sender ip address */
    socklen_t len;              /* sender ip addrees length */
};
struct cmd_cmder{
    int sfd;    /* server fd */
    u16_t sport;/* server port */
};
struct cmd_data{
    struct cmd_sender sender;  /* command sender information */
    struct cmd_cmder cmder;    /* command module information */
};

typedef int (*command_cb_t)(int argc, char *argv[], 
        char *buff, int len, struct cmd_data *cdata, void *user);

void cmd_init(void);
struct command *cmd_find(char *name);
int cmd_add(char *name, char *spec, char *usage, command_cb_t func, void *user);
int cmd_rmv(char *name);

#endif
