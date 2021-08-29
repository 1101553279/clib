#include "plog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "command.h"
#include "tick.h"
#include <time.h>
#include "pthread.h"
#include "log.h"
#include "cfg.h"

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
/* plog's command */
static int plog_command(int argc, char *argv[], char *buff, int len, void *user);

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
    /* add plog command */
    cmd_add("plog", "plog execution procedure", CMD_INDENT"plog [run/cmd] on/off", 
            plog_command, NULL);
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

u16_t plog_con_dump(struct plog_con *c, char *buff, u16_t len)
{
    u16_t ret = 0;

    ret += snprintf(buff+ret, len-ret, "connect ufd  : %d\r\n", c->ufd);
    ret += snprintf(buff+ret, len-ret, "connect addr : %s\r\n", 
                                        inet_ntoa(c->caddr.sin_addr));
    ret += snprintf(buff+ret, len-ret, "connect port : %u\r\n",
                                        ntohs(c->caddr.sin_port));

    return ret;
}

u16_t plog_que_dump(struct plog_que *q, char *buff, u16_t len)
{
    u16_t ret = 0;

    ret += snprintf(buff+ret, len-ret, "queue front     : %d\r\n", q->front);
    ret += snprintf(buff+ret, len-ret, "queue rear      : %d\r\n", q->rear);
    ret += snprintf(buff+ret, len-ret, "queue pmsg      : %p\r\n", q->pmsg);
    ret += snprintf(buff+ret, len-ret, "queue size      : %u\r\n", q->size);
    ret += snprintf(buff+ret, len-ret, "queue len       : %d\r\n", 
            ((q->front-q->rear)+q->size)%q->size);

    return ret;
}

/* dump plog module information to buffer */
u16_t plog_dump(char *buff, u16_t len)
{
    struct plog *p = &plog_obj;
    u16_t ret = 0;
    
    ret += snprintf(buff+ret, len-ret, "/*** plog summary ***/\n");
    ret += snprintf(buff+ret, len-ret, "thread id   : %#lx\r\n", p->tid);
    ret += snprintf(buff+ret, len-ret, "plog key    : %#x\n", p->key);
    ret += snprintf(buff+ret, len-ret, "-------------------------\r\n");
    ret += plog_con_dump(&p->con, buff+ret, len-ret);
    ret += snprintf(buff+ret, len-ret, "-------------------------\r\n");
    ret += plog_que_dump(&p->que, buff+ret, len-ret);

    return ret;
}

/****** static function list ******/
static int plog_command(int argc, char *argv[], char *buff, int len, void *user)
{
    int ret = 0;
    int i = 0;
    char mbuff[100];    /* mod buffer */
    int mret = 0;   /* mod return length */
    u32_t key = 0;
    char *arg;
    struct sockaddr_in caddr;
    socklen_t clen;
#if 0
    for(i=0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");
#endif
    if(argc < 2)
    {
        ret += snprintf(buff+ret, len-ret, "parameter < 2\n");
        goto err_argc;
    }

    if(0!=strcmp("on", argv[argc-1]) && 0!=strcmp("off", argv[argc-1]))
    {
        ret += snprintf(buff+ret, len-ret, "cmd error: the lastest parameter must be on or off\n");
        goto err_key;
    }

    memset(mbuff, 0, sizeof(mbuff));
    /* match mods */
    for(i=1; i < argc-1; i++)
    {
        arg = argv[i];
        if(0 == strcmp("run", arg))
            key |= PLOG_RUN;
        else if(0 == strcmp("cmd", arg))
            key |= PLOG_CMD;
        else if(0 == strcmp("cfg", arg))
            key |= PLOG_CFG;
        else
        {
            ret += snprintf(buff+ret, len-ret, "%s %s %s <- no this mod\n", argv[0], mbuff, arg);
            goto err_mod;  /* match fail, parameter error */
        }
        mret += snprintf(mbuff+mret, sizeof(mbuff)-mret, "%s ", arg);
    }

    if(0 == strcmp("on", argv[i]))
    {
        cmd_client(&caddr, &clen);  /* get client address information */
        plog_con_on(cmd_srv_fd(), &caddr, clen);
        if(0 != key)
            plog_key_on(key);
        ret += snprintf(buff+ret, len-ret, "/*** plog %s on ***/\n", 0==strlen(mbuff)? "key" :mbuff);
    }
    else
    {
        if(0 == key)
            plog_con_off();
        else
            plog_key_off(key);
        ret += snprintf(buff+ret, len-ret, "/*** plog %s off ***/\n", 0==strlen(mbuff) ?"all" :mbuff);
    }

    ret += snprintf(buff+ret, len-ret, "\n");
err_mod:
err_key:
err_argc: 
    return ret;
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

