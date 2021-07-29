#include "tick.h"
#include <stdio.h>
#include <stdlib.h>
#include "blist.h"
#include "log.h"
#include "plog.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "command.h"

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
    struct list_head head;
    pthread_t tid;  /* thread id */
    u32_t ctimes;   /* current times */
};

static struct tick tick_obj;

static void tknode_do(struct tknode *node, u32_t ctimes);
static void *task_cb(void *data);
static u16_t tick_list(struct tick *tk, char *buff, u16_t len);
static int tick_command(int argc, char *argv[], char *buff, int len, void *user)
{
    struct tick *tk = (struct tick *)user;
    u16_t ret = 0;
    int i = 0;
    int mark = 1;

    if(2 == argc)
    {
        if(0 == strcmp(argv[1], "list"))
            ret += tick_list(tk, buff+ret, len-ret);
        else if(0 == strcmp(argv[1], "dump"))
            ret += tick_dump(buff+ret, len-ret);
        else
            mark = 0;
    }

    if(0 == mark)
    {
        for(i=0; i < argc; i++)
            ret += snprintf(buff+ret, len-ret, "%s ", argv[i]);
        ret += snprintf(buff+ret, len-ret, " <- no this cmd\r\n");
    }

    return ret;
}
void tick_init(void)
{
    struct tick *tk = &tick_obj; 
    int ret = 0;

    INIT_LIST_HEAD(&tk->head);
   
    tk->ctimes = 0;
    ret = pthread_create(&tk->tid, NULL, task_cb, NULL);
    if(ret < 0)
    {
        log_red("new task fail!\r\n"); 
        return;
    }
    
    log_grn("tick init success!\r\n");
    cmd_add("tick", "manage tick module", CMD_INDENT"tick [list|dump]", tick_command, tk);

    return;
}
int tick_add(char *name, tknode_cb_t cb, void *udata, u16_t div)
{
    struct tick *tk = &tick_obj;
    struct tknode *node = NULL;
    struct list_head *pos;

    if(NULL == cb)
    {
        log_red("tick add fail: 0==cb\r\n");
        return -1;
    }
    /* check repeated tick node add */
    list_for_each(pos, &tk->head)
    {
        node = list_entry(pos, struct tknode, node);
        if(NULL!=name && 0==strcmp(name, node->name))
        {
            log_red("repeated add: tick has been added!\r\n");
            return -1;
        }
    }
    
    /* add tick node to tick list */
    node = malloc(sizeof(struct tknode));
    if(NULL == node)
    {
        log_red("memory malloc fail!\r\n");
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

int tick_rmv(char *name)
{
    struct tick *tk = &tick_obj;
    struct tknode *node = NULL;
    struct list_head *pos = NULL;
    struct list_head *n = NULL;

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
static u16_t tknode_dump(struct tknode *node, char *buff, u16_t len)
{
    u16_t ret = 0;
    
    ret += snprintf(buff+ret, len-ret, "| %-15s | %-10u | %-15p | %-15p | %-15u |\r\n", 
            node->name, node->ptimes, node->cb, node->udata, node->div);

    return ret;
}

#define TICK_SPLIT  " --------------------------------------------------"\
                    "---------------------------------- \r\n"
static u16_t tick_list(struct tick *tk, char *buff, u16_t len)
{
    u16_t ret = 0;
    struct tknode *node = NULL;
    struct list_head *pos = NULL;

    ret += snprintf(buff+ret, len-ret, "****** tknode list summary ******\r\n");
    ret += snprintf(buff+ret, len-ret, TICK_SPLIT);
    ret += snprintf(buff+ret, len-ret, "| %-15s | %-10s | %-15s | %-15s | %-15s |\r\n", 
                                       "name", "ptimes", "cb", "udata", "div(ms)");
    list_for_each(pos, &tk->head)
    {
        ret += snprintf(buff+ret, len-ret, TICK_SPLIT);
        node = list_entry(pos, struct tknode, node);
        ret += tknode_dump(node, buff+ret, len-ret);
    }
    ret += snprintf(buff+ret, len-ret, TICK_SPLIT);

    return ret;
}

u16_t tick_dump(char *buff, u16_t len)
{
    u16_t ret = 0;
    struct tick *tk = &tick_obj;

    ret += snprintf(buff+ret, len-ret, "/*** tick summary ***/\r\n");
    ret += snprintf(buff+ret, len-ret, "ctimes  : %u\r\n", tk->ctimes);
    ret += tick_list(tk, buff+ret, len-ret);

    return ret;
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
