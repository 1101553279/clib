#include "plog.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "command.h"
#include "plog.h"
#include "tick.h"
#include <time.h>

#define PBUF_SIZE    1024

struct con{
    int ufd;        /* udp file description */
    struct sockaddr_in caddr;/* client address */
    socklen_t clen; /* client address length*/
};

struct plog{
    u32_t key;              /* debug key, each bit indicate a mod */
    struct con con;         /* for send msg */
    char buff[PBUF_SIZE];   /* for format log */
};

static struct plog plog_obj;
static void con_init(struct con *c);
static void con_send(struct con *c, char *buff, u16_t len);
static int plog_command(int argc, char *argv[], char *buff, int len, void *user);
static void plog_key_on(u32_t mods);
static void plog_key_off(u32_t mods);
static void plog_con_on(int ufd, struct sockaddr_in *addr, socklen_t len);
static void plog_con_off(void);


void plog_tick(void *data)
{
    time_t t;

    time(&t);

    printf("plog tick: %s", ctime(&t));
}

/* init plog module */
void plog_init(void)
{
    struct plog *p = &plog_obj;
    
    p->key = 0;
    con_init(&p->con);
    memset(p->buff, 0, sizeof(p->buff));
    /* add plog command */
    cmd_add("plog", "plog execution procedure", CMD_INDENT"plog [run/cmd] on/off", plog_command, NULL);
    
    tick_add("plog", plog_tick, p, 10000);
    

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
    char *buff = p->buff;

    memset(buff, 0, PBUF_SIZE);

    /* format log */
    ret += snprintf(buff+ret, PBUF_SIZE-ret, "%s: %d ## %s ## ", func, line, mod);
    va_start(ap, line);
    ret += vsnprintf(buff+ret, PBUF_SIZE-ret, fmt, ap);
    va_end(ap);

    /* send log to client */
    con_send(&p->con, buff, ret);

    return;
}

void plog_uninit(void)
{
    return;
}

/* dump plog module information to buffer */
u16_t plog_dump(char *buff, u16_t len)
{
    struct plog *p = &plog_obj;
    struct con *c = &p->con;
    u16_t ret = 0;
    
    ret += snprintf(buff+ret, len-ret, "/*** plog summary ***/\n");
    ret += snprintf(buff+ret, len-ret, "key : %#x\n", p->key);
    ret += snprintf(buff+ret, len-ret, "ufd : %d\n", c->ufd);
    ret += snprintf(buff+ret, len-ret, "add : %s\n", inet_ntoa(c->caddr.sin_addr));

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

/* init connection module */
static void con_init(struct con *c)
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
static void con_send(struct con *c, char *buff, u16_t len)
{
    /* has a client */
    if(c && c->ufd > 0 && NULL!=buff && 0!=len)
        sendto(c->ufd, buff, len, 0, (const struct sockaddr*)&c->caddr, c->clen);

    return;
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
/* set client's connection information */
static void plog_con_on(int ufd, struct sockaddr_in *addr, socklen_t len)
{
    struct con *c = &plog_obj.con;

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
    struct con *c = &plog_obj.con;

    /* clear all information */
    con_init(c);

    /* set PLOG_KEY off*/
    plog_obj.key &= ~PLOG_KEY;

    return;
}
