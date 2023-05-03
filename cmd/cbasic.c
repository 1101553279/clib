#include "cbasic.h"
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "blist.h"
#include "plog.h"
#include "tick.h"
#include "sock.h"
#include "cfg.h"
#include "argtable3.h"
#include "util.h"

static int test_command(int argc, char *argv[], char *buff, int len, void *user);
static int test_command_replace(int argc, char *argv[], char *buff, int len, void *user);
static int help_command(int argc, char *argv[], char *buff, int len, void *user);
static int dump_command(int argc, char *argv[], char *buff, int len, void *user);

/* init basic command module */
void cbasic_init(void)
{ 
#if 1
    cmd_add("help", "list all commands information\r\n","help [cmd|plog|help]\r\n", help_command, NULL);
//    cmd_add("test", "test command function\r\n","test\r\n", test_command, NULL);
    
    cmd_add("dump", "dump module information\r\n","dump [cmd|plog]\r\n", dump_command, NULL);
#endif
#if 0
    cmd_tree_print();   /* for debug */
    cmd_add_force("test", "test command function\r\n","test\r\n", test_command_replace, NULL);
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

    ret += snprintf(buff+ret, len-ret, "%-6s %-s", CMD_NAME(c), CMD_SPEC(c));

    return ret;
}



/* help command function */
static int help_command(int argc, char *argv[], char *buff, int len, void *user)
{
    int ret = 0;
    struct arg_str *arg_c;
    struct arg_lit *arg_a;
    struct arg_end *arg_end_flag;
    int i = 0;
    void *argtable[] = 
    {
        arg_c           = arg_strn("c", "cmd", "[name]", 0, 10, "which command for helping"),
        arg_a           = arg_lit0("a", "all",                  "get all commands for helping"),
        arg_end_flag    = arg_end(5),
    };
    int nerrors;
    const char *name = argv[0];
    struct command *c = cmd_find((char *)name);
    arg_dstr_t ds = arg_dstr_create();

    if(0 != arg_nullcheck(argtable))
    {
        ret += snprintf(buff+ret, len-ret, "%s: insufficient memory\r\n", CMD_NAME(c));
        goto cmd_find;
    }

    nerrors = arg_parse(argc, argv, argtable);
    /* for : input error message */
    if(nerrors > 0)
    {
//        printf("nerrors = %d\r\n", nerrors);
        arg_print_errors_ds(ds, arg_end_flag, "help");
        ret += snprintf(buff+ret, len-ret, "%s", arg_dstr_cstr(ds));
        goto arg_parse;    
    }

    /* for : help -c name0 -c name1 -c name2 ..... */
    if(arg_c->count > 0)
    {
        for(i=0; i < arg_c->count; i++)
        {
            name = arg_c->sval[i];
            c = cmd_find((char *)name);
            if(c)
            {
                ret += snprintf(buff+ret, len-ret, "/****** %s usage summary ******/\r\n", name);
                ret += snprintf(buff+ret, len-ret, "%s", CMD_USAGE(c));
            }
            else
                ret += snprintf(buff+ret, len-ret, "command '%s' not found\r\n", name);
        }
        goto arg_c;
    }
    /* for : help -a */ 
    if(arg_a->count > 0)
    {
        ret += cmd_info_iterate(buff+ret, len-ret, help_iterate_cb, "/****** all commands ******/\r\n");
        goto arg_a;
    }

    /* help usage for it's self */
    arg_dstr_catf(ds, "Usage: %s", "help");
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-20s %s\r\n");
    ret += snprintf(buff+ret, len-ret, "%s", arg_dstr_cstr(ds));

arg_a:
arg_c:
arg_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
cmd_find:
    arg_dstr_destroy(ds);
    return ret;

#if 0
    if(argc < 3)
    {
        if(1 == argc)   /* help -> get all comamnd usage & spec */
            ret += cmd_info_iterate(buff+ret, len-ret, help_iterate_cb, "/****** all commands ******/\r\n");
        else if(2 == argc)  /*eg: help plog */
        {
            c = cmd_find(argv[1]);
            if(c)
            {
                ret += snprintf(buff+ret, len-ret, "/****** %s usage summary ******/\r\n", CMD_NAME(c));
                ret += snprintf(buff+ret, len-ret, "%s", CMD_USAGE(c));
            }
            else
                ret += snprintf(buff+ret, len-ret, "command '%s' not found\r\n", argv[1]);
        }
    }
#endif

    return ret;
}

/* dump command function */
static int dump_command(int argc, char *argv[], char *buff, int len, void *user)
{
    int ret = 0;
    struct arg_lit *dump_arg_null = arg_lit0(NULL, NULL, "dump module information");
    struct arg_lit *dump_arg_m    = arg_lit0("m", "module", "which module for dumping");
    struct arg_end *dump_arg_end  = arg_end(5);
    void *dump_argtable[] = 
    {
        dump_arg_null,
        dump_arg_m,
        dump_arg_end,
    };


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

