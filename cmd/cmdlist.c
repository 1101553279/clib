#include "command.h"
#include <string.h>
#include <stdio.h>
#include "usrv.h"
#include "plog.h"

extern struct command *__start_command;
extern struct command *__stop_command;
static int help_command(struct command *cmd, int argc, char *argv[], void *data)
{
    char buff[CMD_BH_SIZE];
    int ret = 0;
	struct command **iter = &__start_command;	

    if(argc < 3)
    {
        if(1 == argc)
            ret += snprintf(buff+ret, sizeof(buff)-ret, "/****** all commands ******/\n");

        for(;iter < &__stop_command; iter++)
        {
            /* output all commands specification */
            if(1 == argc)
            {
                ret += snprintf(buff+ret, sizeof(buff)-ret, "%-6s %-s\n", (*iter)->name, (*iter)->spec);
//                printf("name = %s\r\n", (*iter)->name);
        //        ret += snprintf(buff+ret, BUFFER_SIZE-ret, "%-s", (*iter)->usage);
            }/* output one command specification */
            else if(2==argc && 0==strcmp((*iter)->name, argv[1]))
            {
                ret += snprintf(buff+ret, sizeof(buff)-ret, "%-s\n%s", 
                        "usage:", (*iter)->usage);
                break;
            }
        }
        /* can not find command */
        if(0 == ret)
            ret += snprintf(buff+ret, sizeof(buff)-ret, "unrecognize this command!\n");
    }
    else
        ret += snprintf(buff+ret, sizeof(buff)-ret, "parameter is too many!\r\n");
 
    /* send all information to client */
    if(ret)
        usrv_client_reply((struct client*)data, buff, ret);
    
	return 0;
}
CMD_NEW(help, "list all commands information", CMD_INDENT"help [cmd]\n", help_command);

int plog_command(struct command *cmd, int argc, char *argv[], void *data)
{
    char buff[CMD_BL_SIZE];
    int ret = 0;
    struct client *c = (struct client *)data;
    int i = 0;
    char mbuff[100];    /* mod buffer */
    int mret = 0;   /* mod return length */
    u32_t key = 0;
    char *arg;
#if 0
    for(i=0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");
#endif
    if(argc < 2)
    {
        ret += snprintf(buff+ret, sizeof(buff)-ret, "parameter < 2\n");
        usrv_client_reply(c, buff, ret);
        return -1;
    }
    memset(buff, 0, sizeof(buff)); 
    memset(mbuff, 0, sizeof(buff));
    if(0!=strcmp("on", argv[argc-1]) && 0!=strcmp("off", argv[argc-1]))
    {
        ret += snprintf(buff+ret, sizeof(buff)-ret, "cmd error: the lastest parameter must be on or off\n");
        usrv_client_reply(c, buff, ret);
        return -1;
    }
    /* match mods */
    for(i=1; i < argc-1; i++)
    {
        arg = argv[i];
        if(0 == strcmp("run", arg))
            key |= PLOG_RUN;
        else
            break;  /* match fail, parameter error */
        mret += snprintf(mbuff+mret, sizeof(mbuff)-mret, "%s ", arg);
    }

    if(0 == strcmp("on", argv[i]))
    {
        plog_con_on(c->sfd, &c->addr, c->len);
        if(0 != key)
            plog_key_on(key);
        ret += snprintf(buff+ret, sizeof(buff)-ret, "/*** plog %s on ***/\n", 0==strlen(mbuff)? "key" :mbuff);
    }
    else
    {
        if(0 == key)
            plog_con_off();
        else
            plog_key_off(key);
        ret += snprintf(buff+ret, sizeof(buff)-ret, "/*** plog %s off ***/\n", 0==strlen(mbuff) ?"all" :mbuff);
    }

    ret += snprintf(buff+ret, sizeof(buff)-ret, "\n");
    
    /* send all information to client */
    if(ret)
        usrv_client_reply(c, buff, ret);

    return 0;
}
CMD_NEW(plog, "plog execution procedure", CMD_INDENT"plog [key...] on/off\n", plog_command);

int dump_command(struct command *cmd, int argc, char *argv[], void *data)
{
    char buff[CMD_BL_SIZE];
    int ret = 0;
    struct client *c = (struct client *)data;

    /* get module information*/
    if(argc == 2)
    {
        if(0 ==strcmp(argv[1], "plog"))
            ret += plog_dump(buff+ret, sizeof(buff)-ret);
    }
    else
        ret += snprintf(buff+ret, sizeof(buff)-ret, "parameter error!\n");

    if(ret)
        usrv_client_reply(c, buff, ret);
    
    return 0;
}
CMD_NEW(dump, "dump module information", CMD_INDENT"dump plog|...\n", dump_command);
