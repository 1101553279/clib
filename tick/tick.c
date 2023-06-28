#include "tick.h"
#include <stdio.h>
#include <stdlib.h>
#include "util/blist.h"
#include "log/log.h"
#include "log/plog.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "cmd/command.h"
#include "util/util.h"

#define TICK_NAME       "ticktask"
#define USEC_PER_SEC    1000000
#define TICK_001MS      (USEC_PER_SEC/1000)
#define TICK_010MS      (USEC_PER_SEC/100)
#define TICK_100MS      (USEC_PER_SEC/10)
#define TICK_MSSEC      (TICK_001MS) 
#define TICK_DIV        TICK_MSSEC

struct tknode{
    struct list_head node;
    u32_t ptimes;    /* previous times */
    char *name;
    tknode_cb_t cb;
    void *udata;
    u16_t div;      /* call cb how times */
};

struct tick{
    u8_t init;
    struct list_head head;
    pthread_t tid;  /* thread id */
    u32_t ctimes;   /* current times */
};

static struct tick tick_obj;

static void tknode_do(struct tknode *node, u32_t ctimes);
static void *task_cb(void *data);
static int tick_list(struct tick *tk, arg_dstr_t ds);
/***************************************************
    tick [-d|dump]
    tick -l
***************************************************/
static int tick_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds)
{
    struct tick *tk = &tick_obj; 
    struct arg_lit *arg_d;
    struct arg_lit *arg_l;
    struct arg_end *end_arg;
    void *argtable[] = 
    {
        arg_d           = arg_lit0("d", "dump",  "dump information about tick module"),
        arg_l           = arg_lit0("l", "list",  "list all tick nodes"),
        end_arg         = arg_end(5),
    };
    int ret;

    ret = argtable_parse(argc, argv, argtable, end_arg, ds, argv[0]);
    if(0 != ret)
        goto to_parse;

    if(arg_d->count > 0)
    {
        tick_dump(ds);
        goto to_d;
    }

    if(arg_l->count > 0)
    {
        tick_list(tk, ds);
        goto to_l;
    }
    
    /* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", argv[0]);
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-20s %s\r\n");


to_l:
to_d:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}

/****************************************************************************** 
 * brief    : init tick module
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
void tick_init(void)
{
    struct tick *tk = &tick_obj; 
    s8_t ret = 0;

    INIT_LIST_HEAD(&tk->head);
   
    tk->ctimes = 0;
    ret = pthread_create(&tk->tid, NULL, task_cb, NULL);
    if(ret < 0)
    {
        log_red_print("new task fail!\r\n"); 
        return;
    }
    
    log_grn_print("tick init success!\r\n");
    tk->init = 1;

    return;
}

void tick_init_append(void)
{
    
    arg_cmd_register("tick", tick_arg_cmdfn, "manage tick module");

    return;
}

/****************************************************************************** 
 * brief    : add a tick_node into tick module 
 * Param    : name  tick_node's name 
 * Param    : cb    tick_node's callback
 * Param    : udata tick_node's userdata
 * Param    : div   tick_node's interval
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
int tick_add(char *name, tknode_cb_t cb, void *udata, u16_t div)
{
    struct tick *tk = &tick_obj;
    struct tknode *node = NULL;
    struct list_head *pos;

    if(1 != tk->init)
    {
        log_red_print("tick has not been inited - name(%s) can't not add\r\n", name);
        return -1;
    }

    if(NULL == cb)
    {
        log_red_print("tick add fail: 0==cb\r\n");
        return -1;
    }
    /* check repeated tick node add */
    list_for_each(pos, &tk->head)
    {
        node = list_entry(pos, struct tknode, node);
        if(NULL!=name && 0==strcmp(name, node->name))
        {
            log_red_print("repeated add: tick has been added!\r\n");
            return -1;
        }
    }
    
    /* add tick node to tick list */
    node = malloc(sizeof(struct tknode));
    if(NULL == node)
    {
        log_red_print("memory malloc fail!\r\n");
        return -1;
    }
    node->ptimes = tk->ctimes;
    node->name = name;
    node->cb = cb;
    node->udata = udata;
    node->div = div;
    
    /* add to tick list */
    list_add(&node->node, &tk->head);

    return 0;
}

/****************************************************************************** 
 * brief    : remove a tick_node into tick module 
 * Param    : name  tick_node's name 
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
int tick_rmv(char *name)
{
    struct tick *tk = &tick_obj;
    struct tknode *node = NULL;
    struct list_head *pos = NULL;
    struct list_head *n = NULL;
    
    if(1 != tk->init)
    {
        log_red_print("tick has not been inited - name(%s) can't not rmv\r\n", name);
        return -1;
    }

    list_for_each_safe(pos, n, &tk->head)
    {
        node = list_entry(pos, struct tknode, node);
        if(0 == strcmp(name, node->name))
        {
            list_del(&node->node);
            free(node); /* do not froget it */
            return 0;
        }
    }
   
    return -1;
}

/* dump one tick node information */
static int tknode_dump(struct tknode *node, arg_dstr_t ds)
{
    arg_dstr_catf(ds, "| %-15s | %-10u | %-15p | %-15p | %-15u |\r\n", 
            node->name, node->ptimes, node->cb, node->udata, node->div);

    return 0;
}

#define TICK_SPLIT  " --------------------------------------------------"\
                    "---------------------------------- \r\n"
static int tick_list(struct tick *tk, arg_dstr_t ds)
{
    struct tknode *node = NULL;
    struct list_head *pos = NULL;

    arg_dstr_catf(ds, "****** tknode list summary ******\r\n");
    arg_dstr_catf(ds, TICK_SPLIT);
    arg_dstr_catf(ds, "| %-15s | %-10s | %-15s | %-15s | %-15s |\r\n", \
            "name", "ptimes", "cb", "udata", "div(ms)");
    list_for_each(pos, &tk->head)
    {
        arg_dstr_catf(ds, TICK_SPLIT);
        node = list_entry(pos, struct tknode, node);
        tknode_dump(node, ds);
    }
    arg_dstr_catf(ds, TICK_SPLIT);

    return 0;
}

int tick_dump(arg_dstr_t ds)
{
    struct tick *tk = &tick_obj;

    arg_dstr_catf(ds, "/*** tick summary ***/\r\n");
    arg_dstr_catf(ds, "ctimes  : %u\r\n", tk->ctimes);
    tick_list(tk, ds);

    return 0;
}

static void tknode_do(struct tknode *node, u32_t ctimes)
{
    if(NULL==node)
        return;

    /* check whether has reached target times */
    if(ctimes >= node->ptimes+node->div)
    {
        node->ptimes = ctimes;
        if(node->cb)
            node->cb(node->udata);
    }
    return;
}

static void *task_cb(void *data)
{
    struct tick *tk = &tick_obj;
    struct tknode *node;
    struct list_head *pos;

//        logc(COLOR_FONT_GREN, "1ms task!\r\n");
    while(1)
    {
        tk->ctimes++;
    
        /* check whether needs to do */
        list_for_each(pos, &tk->head)
        {
            node = list_entry(pos, struct tknode, node);
            plog(TICK, "do task: %s\r\n", node->name);
            tknode_do(node, tk->ctimes);
        }
        
        /* delay 1ms */
        usleep(TICK_DIV);
    }
    return NULL;
}
