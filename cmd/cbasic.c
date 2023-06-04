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


static int help_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds);
static int dump_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds);

/* init basic command module */
void cbasic_init(void)
{ 
    arg_cmd_register("help", help_arg_cmdfn, "get one or more commands help information");
    
    arg_cmd_register("dump", dump_arg_cmdfn, "information about the <dump> command");

    return;
}

static int help_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds)
{
    struct arg_str *arg_c;
    struct arg_lit *arg_a;
    struct arg_end *end_arg;
    int i = 0;
    void *argtable[] = 
    {
        arg_c           = arg_strn("c", "cmd", "[name]", 0, 10, "which command for helping"),
        arg_a           = arg_lit0("a", "all",                  "get all commands for helping"),
        end_arg         = arg_end(5),
    };
    int nerrors;
    const char *name = argv[0];
    arg_cmd_info_t *info;
    int ret = 0;

    ret = argtable_parse(argc, argv, argtable, end_arg, ds, name);
    if(0 != ret)
        goto to_parse;

    char *m_argv[1];
    /* for : help -c name0 -c name1 -c name2 ..... */
    if(arg_c->count > 0)
    {
        for(i=0; i < arg_c->count; i++)
        {
            name = arg_c->sval[i];
            info = arg_cmd_info(name);
            if(NULL != info)
            {
                m_argv[0] = info->name;
                if(info->proc)
                    info->proc(1, m_argv, ds);
            }
            else
                arg_dstr_catf(ds, "command '%s' not found\r\n", name);
        }
        goto to_c;
    }
    /* for : help -a */ 
    if(arg_a->count > 0)
    {
        info = arg_cmd_info("cmd");
        if(NULL != info)
        {
            m_argv[0] = info->name;
            if(info->proc)
                info->proc(1, m_argv, ds);
        }
        else
            arg_dstr_catf(ds, "not found <cmd>\r\n");

        goto to_a;
    }

    /* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", "help");
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-20s %s\r\n");

to_a:
to_c:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}

static int dump_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds)
{
    int ret = 0;
//    struct arg_str *arg_null = arg_lit0(NULL, NULL, "dump module information");
    struct arg_str *arg_m    = arg_strn("m", "module", "<plog|cmd|tick|cfg>", 0, 10, "which module for dumping");
    struct arg_str *arg_s    = arg_str0("s", "sock", "<tcps>", "which sock for dumping");
    struct arg_end *end_arg  = arg_end(5);
    int errors;
    int i = 0;
    const char *name;
    char buff[2048] = {'\0'};
    const size_t len=sizeof(buff);
    void *argtable[] = 
    {
//        arg_null,
        arg_m,
        arg_s,
        end_arg,
    };
    
    ret = argtable_parse(argc, argv, argtable, end_arg, ds, argv[0]);
    if(0 != ret)
        goto to_parse;
    
    if(arg_m->count > 0)
    {
        for(i=0; i < arg_m->count; i++)
        {
            name = arg_m->sval[i];
            if(0 ==strcmp(name, "plog"))
                plog_dump(ds);
            else if(0 == strcmp(name, "cmd"))
                cmd_dump(ds);
            else if(0 == strcmp(name, "tick"))
                tick_dump(ds);
            else if(0 == strcmp(name, "cfg"))
                cfg_dump(ds);
            else
            {
                arg_dstr_cat(ds, "usage : dump ");
                arg_print_option_ds(ds, "m", "module", "<name>", "\r\n");
            }
        }
        goto to_m;
    }
    
    if(arg_s->count > 0)
    {
        if(0 == strcmp(arg_s->sval[0], "tcps"))
            sock_tcps_dump(ds);
        else
        {
            arg_dstr_cat(ds, "usage : dump ");
            arg_print_option_ds(ds, "s", "sock", "<module>", "\r\n");
        }
        goto to_s;
    }

    arg_make_help_msg(ds, "dump", argtable);

to_s:
to_m:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}
