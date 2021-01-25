#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "usrv.h"

#define CMD_INDENT  "       "
#define CMD_BH_SIZE 1024
#define CMD_BM_SIZE 512
#define CMD_BL_SIZE 256


struct command;
typedef int (*command_cb_t)(struct command *cmd, int argc, char *argv[], void *data);
struct command{
	char *name; /* command name */
	char *spec; /* command help information */
	char *usage; /* command usage */
	command_cb_t func; /*command execution function */
};

#define CMD_NEW(name, spec, usage, func)	\
	static struct command __##cmd_##name = {#name, spec, usage, func};\
	static struct command *__##pcmd_##name __attribute__((section("command"))) = &__##cmd_##name


int cmd_exe(struct client *cli, unsigned char *buff, unsigned int len);

#endif
