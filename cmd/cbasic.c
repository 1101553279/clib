#include "cbasic.h"
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "blist.h"
#include "plog.h"
#include "tick.h"
#include "sock.h"
#include "cfg.h"

static int test_command(int argc, char *argv[], char *buff, int len, void *user);
static int test_command_replace(int argc, char *argv[], char *buff, int len, void *user);
static int help_command(int argc, char *argv[], char *buff, int len, void *user);
static int dump_command(int argc, char *argv[], char *buff, int len, void *user);

/* init basic command module */
void cbasic_init(void)
{
#if 1
    cmd_add("help", "list all commands information", CMD_INDENT"help [cmd|plog|help]", help_command, NULL);
//    cmd_add("test", "test command function", CMD_INDENT"test", test_command, NULL);
    cmd_add("dump", "dump module information", CMD_INDENT"dump [cmd|plog]", dump_command, NULL);
#endif
#if 0
    cmd_tree_print();   /* for debug */
    cmd_add_force("test", "test command function", CMD_INDENT"test\n", test_command_replace, NULL);
    cmd_tree_print();   /* for debug */
    cmd_del("test");
    cmd_tree_print();
    cmd_del("dump");
    cmd_tree_print();
#endif
    return;
}
static int test_command_replace(int argc, char *argv[], char *buff, int len, void *user)
{
    u16_t ret = 0;
    int i = 0;

    ret += snprintf(buff+ret, len-ret, "test command: replace");
    for(i=0; i < argc; i++)
        ret += snprintf(buff+ret, len-ret, "%s | ", argv[i]);
    ret += snprintf(buff+ret, len-ret, "\r\n");

    return ret;
}

static int test_command(int argc, char *argv[], char *buff, int len, void *user)
{
    u16_t ret = 0;
    int i = 0;

    ret += snprintf(buff+ret, len-ret, "test command: ");
    for(i=0; i < argc; i++)
        ret += snprintf(buff+ret, len-ret, "%s | ", argv[i]);
    ret += snprintf(buff+ret, len-ret, "\r\n");

    return ret;
}

/* for get each command usage & spec information */
u16_t help_iterate_cb(struct command *c, char *buff, u16_t len)
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
                ret += snprintf(buff+ret, len-ret, "%-s\r\n%s\r\n", "usage:", CMD_USAGE(c));
            else
                ret += snprintf(buff+ret, len-ret, "command '%s' not found\r\n", argv[1]);
        }
    }

    return ret;
}

/* dump command function */
static int dump_command(int argc, char *argv[], char *buff, int len, void *user)
{
    int ret = 0;

    /* get module information*/
    if(argc == 2)
    {
        if(0 ==strcmp(argv[1], "plog"))
            ret += plog_dump(buff+ret, len-ret);
        else if(0 == strcmp(argv[1], "cmd"))
            ret += cmd_dump(buff+ret, len-ret);
        else if(0 == strcmp(argv[1], "tick"))
            ret += tick_dump(buff+ret, len-ret);
        else if(0 == strcmp(argv[1], "cfg"))
            ret += cfg_dump(buff+ret,len-ret);
        else
            ret += snprintf(buff+ret, len-ret, "%s %s <- no this module\n", argv[0], argv[1]);
    }
    else if(3 == argc)
    {
        if(0 == strcmp(argv[1], "sock"))
        {
            if(0 == strcmp(argv[2], "tcps"))
                ret += sock_tcps_dump(buff+ret, len-ret);
        }
        else
            ret += snprintf(buff+ret, len-ret, "%s %s %s <- no this module\n", argv[0], argv[1], argv[2]);
    }
    else
        ret += snprintf(buff+ret, len-ret, "parameter error!\n");

    return ret;
}

