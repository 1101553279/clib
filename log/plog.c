#include "plog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cmd/command.h"
#include "tick/tick.h"
#include <time.h>
#include "pthread.h"
#include "log/log.h"
#include "cfg/cfg.h"
#include "cmd/argtable3.h"
#include "util/util.h"

#define PBUF_SIZE       1024
#define PMSG_MIN_SIZE   2       /* minimum size */
#define PMSG_DEF_SIZE   20      /* default size */
#define PMSG_MAX_SIZE   1000    /* maximum size */

/* for debug */
#define PLOG_LOCAL  1

struct plog_con{
    int ufd;                    /* udp file description */
    struct sockaddr_in caddr;   /* client address */
    socklen_t clen;             /* client address length*/
};

struct plog_que{
    char **pmsg;                /* message size can configure */
    u16_t size;                 /* message size */
    int front;                  /* queue's front */
    int rear;                   /* queue's rear */
    pthread_mutex_t lock;
    pthread_cond_t in;          /* put a message */
    pthread_cond_t out;         /* get a message */
};

struct plog{
    u8_t init;
    pthread_t tid;
    u32_t key;                  /* debug key, each bit indicate a mod */
    struct plog_con con;        /* connection for send msg */
    struct plog_que que;        /* queue for caching message */
};

static struct plog plog_obj;


static int plog_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds);

static void plog_key_on(u32_t mods);
static void plog_key_off(u32_t mods);
/* plog's connection */
static void plog_con_init(struct plog_con *c);
static void plog_con_send(struct plog_con *c, char *buff);
static void plog_con_on(int ufd, struct sockaddr_in *addr, socklen_t len);
static void plog_con_off(void);
/* plog's message queue */
static void plog_que_init(struct plog_que *q, u16_t size);
static char *plog_que_get(struct plog_que *q);
static int plog_que_put(struct plog_que *q, char *msg);

void plog_tick(void *data)
{
    time_t t;

    time(&t);

    printf("plog tick: %s", ctime(&t));
}

static void *plog_routine(void *arg)
{
    struct plog *p = &plog_obj;
    char *msg = NULL;

    while(1)
    {
        msg = plog_que_get(&p->que);
        if(NULL != msg)
        {
            plog_con_send(&p->con, msg);
            free(msg);      /* don't forget free msg */
        }
    }

    return NULL;
}

/* init plog module */
void plog_init(void)
{
    struct plog *p = &plog_obj;
    
    p->key = PLOG_KEY|PLOG_CFG;

    plog_con_init(&p->con);

    /* read msg_size from configure file */
    plog_que_init(&p->que, (u16_t )atoi(cfg_read("plog", "msize", NULL)));
    pthread_create(&p->tid, NULL, plog_routine, NULL);
    p->init = 1;
    
    return;
}

/* append plog init */
void plog_init_append(void)
{
    arg_cmd_register("plog", plog_arg_cmdfn, "manage plog module");
//    tick_add("plog", plog_tick, p, 10000);

    return;
}

/* decide the mods's key whther is opened*/
u8_t plog_key_is(u32_t mods)
{
    return plog_obj.key & mods? 1: 0;
}

/* format message, and send to client  */
void plog_out(char *mod, const char *fmt, const char *func, int line, ...)
{
    struct plog *p = &plog_obj;
    u16_t ret = 0;
    va_list ap;

#if PLOG_LOCAL
    char msg[PBUF_SIZE];
#else
    char *msg = NULL;

    if(1 != p->init)
    {
        log_red("plog has not been inited - mod(%s) can't plog\r\n", mod);
        return;
    }

    msg = (char *)malloc(PBUF_SIZE); 
    if(NULL == msg)
    {
        printf("%s:%d calloc fail!\r\n", __func__, __LINE__);
        return;
    }
#endif
    /* format log */
    ret += snprintf(msg+ret, PBUF_SIZE-ret, "%s: %d ## %s ## ", func, line, mod);
    va_start(ap, line);
    ret += vsnprintf(msg+ret, PBUF_SIZE-ret, fmt, ap);
    va_end(ap);
    
#if PLOG_LOCAL
    printf("%s", msg);
#else
    /* put init message queue */
    plog_que_put(&p->que, msg);
#endif

    return;
}

void plog_uninit(void)
{
    return;
}

int plog_con_dump(struct plog_con *c, arg_dstr_t ds)
{
    arg_dstr_catf(ds, "connect ufd  : %d\r\n", c->ufd);
    arg_dstr_catf(ds, "connect addr : %s\r\n", inet_ntoa(c->caddr.sin_addr));
    arg_dstr_catf(ds, "connect port : %u\r\n", ntohs(c->caddr.sin_port));

    return 0;
}

int plog_que_dump(struct plog_que *q, arg_dstr_t ds)
{
    arg_dstr_catf(ds, "queue front     : %d\r\n", q->front);
    arg_dstr_catf(ds, "queue rear      : %d\r\n", q->rear);
    arg_dstr_catf(ds, "queue pmsg      : %p\r\n", q->pmsg);
    arg_dstr_catf(ds, "queue size      : %u\r\n", q->size);
    arg_dstr_catf(ds, "queue len       : %d\r\n", ((q->front-q->rear)+q->size)%q->size);

    return 0;
}

/* dump plog module information to buffer */
int plog_dump(arg_dstr_t ds)
{
    struct plog *p = &plog_obj;
    
    arg_dstr_catf(ds, "/*** plog summary ***/\n");
    arg_dstr_catf(ds, "thread id   : %#lx\r\n", p->tid);
    arg_dstr_catf(ds, "plog key    : %#x\n", p->key);
    arg_dstr_catf(ds, "-------------------------\r\n");
    plog_con_dump(&p->con, ds);
    arg_dstr_catf(ds, "-------------------------\r\n");
    plog_que_dump(&p->que, ds);

    return 0;
}

/************************************************************
 plog [-h]
 plog -d
 plog -m module --on
 plog -m module --off

*************************************************************/
static int plog_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds)
{
    struct plog *p = &plog_obj;
    struct arg_lit *arg_d;
    struct arg_str *arg_m;
    struct arg_lit *arg_on;
    struct arg_lit *arg_off;
    struct arg_end *end_arg;
    int i = 0;
    u32_t key = 0;
    const char *module;
    char mbuff[100];    /* mod buffer */
    int mret = 0;
    void *argtable[] = 
    {
        arg_d           = arg_lit0("d", "dump",     "dump information about plog"),
        arg_m           = arg_str0("m",  "module",  "<module>", "which module you will select"),
        arg_on          = arg_lit0(NULL, "on",      "start plog"),
        arg_off         = arg_lit0(NULL, "off",     "stop plog"),
        end_arg         = arg_end(5),
    };
    int ret;

    ret = argtable_parse(argc, argv, argtable, end_arg, ds, argv[0]);
    if(0 != ret)
        goto to_parse;

    if(arg_d->count > 0)
    {
        plog_dump(ds);
        goto to_d;
    }

    if(arg_m->count > 0)
    {
        for(i=0; i < arg_m->count; i++)
        {
            module = arg_m->sval[i];
            if(0 == strcmp("run", module))
                key |= PLOG_RUN;
            else if(0 == strcmp("cmd", module))
                key |= PLOG_CMD;
            else if(0 == strcmp("cfg", module))
                key |= PLOG_CFG;
            else
                continue;
            mret += snprintf(mbuff+mret, sizeof(mbuff)-mret, "%s ", module);
        }
        if(arg_on->count > 0)
        {
            plog_key_on(key);
            arg_dstr_catf(ds, "/*** plog %s on ***/\n", mbuff);
        }
    }
    

    /* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", argv[0]);
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-25s %s\r\n");


to_l:
to_d:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}

/* open the mods's key */
static void plog_key_on(u32_t mods)
{
    plog_obj.key |= mods;

    return;
}
/* close the mods's key */
static void plog_key_off(u32_t mods)
{
    plog_obj.key &= ~mods;

    return;
}

/* init connection module */
static void plog_con_init(struct plog_con *c)
{
    if(c)
    {
        c->ufd = -1;
        memset(&c->caddr, 0, sizeof(c->caddr));
        memset(&c->clen, 0, sizeof(c->clen));
    }
    return;
}
/* send log to client */
static void plog_con_send(struct plog_con *c, char *msg)
{
    /* has a client */
    if(c && c->ufd > 0 && NULL!=msg)
        sendto(c->ufd, msg, strlen(msg), 0, (const struct sockaddr*)&c->caddr, c->clen);

    return;
}
/* set client's connection information */
static void plog_con_on(int ufd, struct sockaddr_in *addr, socklen_t len)
{
    struct plog_con *c = &plog_obj.con;

    if(NULL==addr)
        return;

    /* set client connection information */
    c->ufd = ufd;
    memcpy(&c->caddr, addr, sizeof(struct sockaddr_in));
    c->clen = len;

    /* set PLOG_KEY on*/
    plog_obj.key |= PLOG_KEY;

    return;
}

/* clear client's connect information */
static void plog_con_off(void)
{
    struct plog_con *c = &plog_obj.con;

    /* clear all information */
    plog_con_init(c);

    /* set PLOG_KEY off*/
    plog_obj.key &= ~PLOG_KEY;

    return;
}

static void plog_que_init(struct plog_que *q, u16_t size)
{
    q->pmsg = malloc(size * sizeof(char *));
    if(NULL == q->pmsg)
        return;

    q->size = size;
    pthread_cond_init(&q->in, NULL);
    pthread_cond_init(&q->out, NULL);
    pthread_mutex_init(&q->lock, NULL);
    q->front = q->rear = 0;

    return;
}

static int plog_que_put(struct plog_que *q, char *msg)
{
    if(NULL==q || NULL==q->pmsg)
        return -1;

    pthread_mutex_lock(&q->lock);
    while(q->front == (q->rear+1)%q->size)    /* wait space for storing message if que is full */
    {
 //       printf("%s:%d queue is full!\r\n", __func__, __LINE__);
        pthread_cond_wait(&q->in, &q->lock);
    }

    q->pmsg[q->rear] = msg;                     /* put message into queue */
    q->rear = (q->rear+1)%q->size;
    
    pthread_cond_signal(&q->out);               /* notify message has been putted, message is available */
    pthread_mutex_unlock(&q->lock);

    return 0;
}

static char *plog_que_get(struct plog_que *q)
{
    char *msg = NULL;

    if(NULL==q || NULL==q->pmsg)
        return NULL;

    pthread_mutex_lock(&q->lock);
    while(q->front == q->rear)                  /* wait for getting message if que is empty */
    {
//        printf("%s:%d queue is empty!\r\n", __func__, __LINE__);
        pthread_cond_wait(&q->out, &q->lock);
    }

    msg = q->pmsg[q->front];                    /* get message from queue */
    q->front = (q->front+1)%q->size;

    pthread_cond_signal(&q->in);                /* notify message has been getted, space is available*/
    pthread_mutex_unlock(&q->lock);
    
    return msg;
}

