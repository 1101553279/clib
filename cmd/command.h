#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "usrv.h"



struct command;
typedef int (*command_cb_t)(struct command *cmd, int argc, char *argv[], void *data);
struct command{
	char *name; /* command name */
	char *spec; /* command help information */
	char *usage; /* command usage */
	command_cb_t func; /*command execution function */
};

#if 0
#define CMD_NEW(name, spec, usage, func)	\
	static struct command __##cmd_##name = {#name, spec, usage, func};\
	static struct command *__##pcmd_##name __attribute__((section("command"))) = &__##cmd_##name
#endif

int cmd_exe(struct client *cli, unsigned char *buff, unsigned int len);

#endif
