#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "usrv.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "btype.h"

struct cmd_sender{
    struct sockaddr_in addr;    /* sender ip address */
    socklen_t len;              /* sender ip addrees length */
};
struct cmd_cmder{
    int sfd;    /* server fd */
    u16_t sport;/* server port */
};
struct cmd_data{
    void *user;     /* command register information */
    struct cmd_sender *sender;  /* command sender information */
    struct cmd_cmder *cmder;    /* command module information */
};

struct command;
typedef int (*command_cb_t)(struct command *cmd, int argc, char *argv[], 
        char *buff, int len, struct cmd_data *cdata);
struct command{
	char *name; /* command name */
	char *spec; /* command help information */
	char *usage; /* command usage */
	command_cb_t func; /*command execution function */
};

void cmd_init(void);
int cmd_add(char *name, char *spec, char *usage, command_cb_t func, void *user);
int cmd_rmv(char *name);

#endif
