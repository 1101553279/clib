#include "cbasic.h"
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "blist.h"

static int help_command(int argc, char *argv[], char *buff, int len, void *user);

/* init basic command module */
void cbasic_init(void)
{
#if 1
    cmd_add("help", "list all commands information", CMD_INDENT"help [cmd]\n", help_command, NULL);
#endif
    return;
}
/* for get each command usage & spec information */
int help_iterate_cb(struct command *c, char *buff, int len)
{
    int ret = 0;

    ret += snprintf(buff+ret, len-ret, "%-6s %-s\n", CMD_NAME(c), CMD_SPEC(c));

    return ret;
}

/* help command function */
static int help_command(int argc, char *argv[], char *buff, int len, void *user)
{
    int ret = 0;
    struct command *c = NULL;

    if(argc < 3)
    {
        if(1 == argc)   /* help -> get all comamnd usage & spec */
            ret += cmd_info_iterate(buff+ret, len-ret, help_iterate_cb, "/****** all commands ******/\n");
        else if(2 == argc)  /*eg: help plog */
        {
            c = cmd_find(argv[1]);
            if(c)
                ret += snprintf(buff+ret, len-ret, "%-s\n%s", "usage:", CMD_USAGE(c));
            else
                ret += snprintf(buff+ret, len-ret, "command '%s' not found\n", argv[1]);
        }
    }

    return ret;
}
