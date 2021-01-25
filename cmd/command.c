#include "command.h"
#include <stdio.h>
#include <string.h>
#include "util.h"

#define CMD_MAX_ARGC	10
enum{
    FAIL_PARSE,
    FAIL_MATCH,
    FAIL_DO,
    MAX_FAIL,
};

static const char *cmd_err[MAX_FAIL] = 
{
	"command parse fail!\r\n",
    "command match fail!\r\n",
    "command do fail!\r\n",
};

extern struct command *__start_command;
extern struct command *__stop_command;
static struct command *cmd_match(char *name)
{
	struct command **iter = &__start_command;	
	for(;iter < &__stop_command; iter++)
	{
//		printf("%s : %s : %s: %p\r\n", (*iter)->name, (*iter)->help, (*iter)->spec, (*iter)->func);
        if(0 == strcmp((*iter)->name, name))
            return *iter;
	}	

	return NULL;
}

static int cmd_do(struct command *cmd, char *argv[], int argc, void *data)
{
    if(cmd)
        cmd->func(cmd, argc, argv, data);
    else
        return -1;
    return 0;
}

extern int usrv_client_reply(struct client *cli, unsigned char *msg, int len);
int cmd_exe(struct client *cli, unsigned char *buff, unsigned int len)
{
	int argc;
	char *argv[CMD_MAX_ARGC];
	int ret = 0;
	int i = 0;
    struct command *pcmd = NULL;

	/* command parse */
	argc = str_parse(buff, argv, ARY_SIZE(argv));
	if(argc <= 0)
	{
		usrv_client_reply(cli, (unsigned char *)cmd_err[FAIL_PARSE], strlen(cmd_err[FAIL_PARSE]));
		return -1;
	}
#if 0
	for(i=0; i < argc; i++)
		printf("%lu:%s | ", strlen(argv[i]), argv[i]);
	printf("\n");
#endif
	
	/* command match */
    pcmd = cmd_match(argv[0]);
    if(NULL == pcmd)
    {
        usrv_client_reply(cli, (unsigned char *)cmd_err[FAIL_MATCH], strlen(cmd_err[FAIL_MATCH]));
        return -1;
    }

	/* command do */
    ret = cmd_do(pcmd, argv, argc, cli);
    if(ret < 0)
    {
        usrv_client_reply(cli, (unsigned char *)cmd_err[FAIL_DO], strlen(cmd_err[FAIL_DO]));
        return -1;
    }

	return 0;
}


