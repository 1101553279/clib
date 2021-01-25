#include "plog.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE    1024

struct con{
    int ufd; /* udp file description */
    struct sockaddr_in caddr;/* client address */
    socklen_t clen; /* client address length*/
};

struct plog{
    u32_t key;              /* debug key, each bit indicate a mod */
    struct con con;         /* for send msg */
    char buff[BUF_SIZE];    /* for format log */
};

static struct plog plog_obj;

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
    /* has client */
    if(c->ufd > 0)
        sendto(c->ufd, buff, len, 0, (const struct sockaddr*)&c->caddr, c->clen);
    return;
}

/* init plog module */
void plog_init(void)
{
    struct plog *p = &plog_obj;
    
    p->key = 0;
    con_init(&p->con);
    memset(p->buff, 0, sizeof(p->buff));

    return;
}

/* decide the mods's key whther is opened*/
u8_t plog_key_is(u32_t mods)
{
    return plog_obj.key & mods? 1: 0;
}

/* open the mods's key */
void plog_key_on(u32_t mods)
{
    plog_obj.key |= mods;
    return;
}
/* close the mods's key */
void plog_key_off(u32_t mods)
{
    plog_obj.key &= ~mods;
    return;
}
/* format message, and send to client  */
void plog_out(char *mod, const char *fmt, const char *func, int line, ...)
{
    struct plog *p = &plog_obj;
    u16_t ret = 0;
    va_list ap;
    char *buff = p->buff;

    memset(buff, 0, BUF_SIZE);

    /* format log */
    ret += snprintf(buff+ret, BUF_SIZE-ret, "%s: %d ## %s ## ", func, line, mod);
    va_start(ap, line);
    ret += vsnprintf(buff+ret, BUF_SIZE-ret, fmt, ap);
    va_end(ap);

    /* send log to client */
    con_send(&p->con, buff, ret);
    return;
}

/* set client's connection information */
void plog_con_on(int ufd, struct sockaddr_in *addr, socklen_t len)
{
    struct con *c = &plog_obj.con;

    /* set client connection information */
    c->ufd = ufd;
    memcpy(&c->caddr, addr, sizeof(struct sockaddr_in));
    c->clen = len;

    /* set PLOG_KEY on*/
    plog_obj.key |= PLOG_KEY;

    return;
}

/* clear client's connect information */
void plog_con_off(void)
{
    struct con *c = &plog_obj.con;

    /* clear all information */
    con_init(c);

    /* set PLOG_KEY off*/
    plog_obj.key &= ~PLOG_KEY;

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
