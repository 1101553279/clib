#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "btype.h"
#include "blist.h"

#define CMD_INDENT  "       "

struct command;
typedef int (*command_cb_t)(int argc, char *argv[], 
        char *buff, int len, void *user);

#define CMD_NAME(c)     ((c)->name)
#define CMD_SPEC(c)     ((c)->spec)
#define CMD_USAGE(c)    ((c)->usage)

/* help command need this structure */
struct command{
    struct list_head head;  /* double list */
	char *name;             /* command name */
	char *spec;             /* command help information */
	char *usage;            /* command usage */
	command_cb_t func;      /* command execution function */
    void *user;             /* user data */
};

/* init command module */
void cmd_init(void);
/* find a command accordding to command name */
struct command *cmd_find(char *name);

/* command manager information */
int cmd_srv_fd(void);

/* sender(PC host) information */
int cmd_client(struct sockaddr_in *addr, socklen_t *len);

/* eg: for help command */
typedef u16_t (*command_info_iterate_cb_t)(struct command *c, char *buff, u16_t len);
u16_t cmd_info_iterate(char *buff, u16_t len, command_info_iterate_cb_t cb, char *title);
/* add a command */
int cmd_add(char *name, char *spec, char *usage, command_cb_t func, void *user);
int cmd_rmv(char *name);

u16_t cmd_dump(char *buff, u16_t len);
#endif
