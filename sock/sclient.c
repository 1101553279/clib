#include "sclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

void sclient_line_node_print(struct sclient *c);
static void sclient_size_cb(struct sclient *cur, void *data);
struct sclient *sclient_new(int fd, struct sockaddr_in *caddr)
{
    struct sclient *c;
    time_t curtime;

    c = malloc(sizeof(struct sclient));
    if(NULL != c)
    {
        c->l = c->r = NULL;
        c->fd = fd;
        c->port = ntohs(caddr->sin_port);
        snprintf(c->ip, sizeof(c->ip), "%s", inet_ntoa(caddr->sin_addr));
        time(&curtime);
        strftime(c->ltime, sizeof(c->ltime), "%H:%M:%S", localtime(&curtime));
    }
    
    return c;
}
struct sclient *sclient_prev_pn_find(struct sclient *cur)
{
    struct sclient *pn = cur->l;

    while(NULL!=pn->r && cur!=pn->r)
        pn = pn->r;
    
    return pn;
}
struct sclient **sclient_prev_ppn_find(struct sclient *cur)
{
    struct sclient **ppn = &(cur->l);

    while(NULL!=(*ppn)->r && cur!=(*ppn)->r)
        ppn = &((*ppn)->r);

    return ppn;
}
struct sclient **sclient_ppn_find(struct sclient **ppn, int fd)
{
    while(NULL != *ppn)
    {
        if(fd < (*ppn)->fd)
            ppn = &((*ppn)->l);
        else if((*ppn)->fd < fd)
            ppn = &((*ppn)->r);
        else
            break;
    }

    return ppn;
}
struct sclient **sclient_max_ppn_find(struct sclient *cur)
{
    struct sclient **ppn = &(cur->l);

    while(NULL != (*ppn)->r)
        ppn = &((*ppn)->r);

    return ppn;
}

int sclient_add(struct sclient **ppn, int fd, struct sockaddr_in *addr)
{
    ppn = sclient_ppn_find(ppn, fd);
    if(NULL == *ppn)
        *ppn = sclient_new(fd, addr);

    return 0;
}

int sclient_del(struct sclient **ppn, int fd)
{
    struct sclient *pn = NULL;

    ppn = sclient_ppn_find(ppn, fd);
    if(NULL == *ppn)
        return -1;
    
    pn = *ppn;      /* save current client */
    if(NULL!=pn->l &&  NULL!=pn->r)
    {
        ppn = sclient_max_ppn_find(pn);
        sclient_swap(pn, *ppn);
        pn = *ppn;  /* change left child's max client */
    }
    *ppn = (NULL==pn->l)? pn->r: pn->l;

    sclient_close(pn);

    return 0;
}
int sclient_close(struct sclient *pn)
{
    if(NULL != pn)
    {
        sclient_del_print(pn); /* only for debug */

        pn->l = pn->r = NULL;
        close(pn->fd);
        pn->port = 0;
        free(pn);
    }

    return 0;
}
struct sclient *sclient_find(struct sclient *pn, int fd)
{
    while(NULL != pn)
    {
        if(fd < pn->fd)
            pn = pn->l;
        else if(pn->fd < fd)
            pn = pn->r;
        else
            break;
    }

    return pn;
}

void sclient_swap(struct sclient *d, struct sclient *s)
{
    struct sclient tmp;

    if(NULL!=d && NULL!=s)
    {
        tmp.fd = d->fd;
        tmp.port = d->port;
        strcpy(tmp.ip, d->ip);
        strcpy(tmp.ltime, d->ltime);

        d->fd = s->fd;
        d->port = s->port;
        strcpy(d->ip, s->ip);
        strcpy(d->ltime, s->ltime);

        s->fd = tmp.fd;
        s->port = tmp.port;
        strcpy(s->ip, tmp.ip);
        strcpy(s->ltime, tmp.ltime);
    }
    return;
}
void sclient_iterate(struct sclient *cur, sclient_iterate_t cb, void *data)
{
    struct sclient *pn = NULL;

    while(NULL != cur) 
    {
        if(NULL == cur->l)
        {
            cb(cur, data);
            cur = cur->r;
        }
        else
        {
            pn = sclient_prev_pn_find(cur);
            if(NULL == pn->r)
            {
                pn->r = cur;
                cur = cur->l;
            }
            else
            {
                pn->r = NULL;
                cb(cur, data);
                cur = cur->r;
            }
        }
    }

    return;
}
void sclient_level_iterate(struct sclient *cur, sclient_iterate_t cb, void *data)
{
    struct sclient **que = NULL;
    int front = 0;
    int rear = 0;

    if(NULL==cur || NULL==(que = malloc(sizeof(struct client *) * sclient_size(cur))))
        return;
    
    while(rear!=front || NULL!=cur)
    {
        if(NULL != cur)
        {
            cb(cur, data);
            if(NULL != cur->l)
                que[front++] = cur->l;
            if(NULL != cur->r)
                que[front++] = cur->r;
            cur = NULL;
        }
        else
            cur = que[rear++];
    }
    
    return;
}

int sclient_size(struct sclient *cur)
{
    int n = 0;

    sclient_iterate(cur, sclient_size_cb, &n);

    return n;
}

int sclient_hand(struct sclient **ppn, struct pollfd *fds)
{
    ssize_t len = 0;
    u8_t buff[100];
    struct sclient *pn = *ppn;

    bzero(buff, sizeof(buff));
    len = read(fds->fd, buff, sizeof(buff)-1);
    if(len <= 0)        /* close    */
        sclient_del(ppn, fds->fd);
    else                /* read or write data */
    {
        pn = sclient_find(*ppn, fds->fd);
        if(NULL != pn)
            sclient_msg_print(pn, buff, len);
    }

    return 0;
}

struct sclient *sclient_uninit(struct sclient *cur)
{
    struct sclient *prev;
    struct sclient *next;
    struct sclient **ppn;

    while(NULL != cur)
    {
        if(NULL == cur->l)
        {
            next = cur->r;
            sclient_close(cur);
            cur = next;
        }
        else
        {
            ppn = sclient_max_ppn_find(cur);
            (*ppn)->r = cur;
            prev = cur->l;
            cur->l = NULL;
            cur = prev;
        }
    }
    
    return cur;
}


void sclient_msg_print(struct sclient *c, u8_t *msg, size_t len)
{
    SOCK_PRINT("(%s:%u(%d)) # %lu # %s\r\n", c->ip, c->port, c->fd, len, msg);
}

void sclient_del_print(struct sclient *c)
{
    SOCK_INFO("del client: ");
    sclient_line_node_print(c);
}

void sclient_line_node_print(struct sclient *c)
{
    SOCK_PRINT("| %-15p | %-15p | %-15p | %-5d | %-15s | %-5u | %-10s |\r\n", 
            c->l, c, c->r, c->fd, c->ip, c->port, c->ltime);
}

void sclient_list_print(struct sclient *c)
{
    if(NULL != c)
    {
        sclient_list_print(c->l);
        SOCK_PRINT( SOCK_SPLIT );
        sclient_line_node_print(c);
        sclient_list_print(c->r);
    }
    return;
}

u16_t sclient_list(struct sclient *c, char *buff, int len)
{
    u16_t ret = 0;

    ret += snprintf(buff+ret, len-ret, "****** tcps client list summary ******\r\n");

    struct sclient_dump_cb_t data = {buff+ret, len-ret, 0};

    sclient_iterate(c, sclient_dump_cb, &data);

    ret += data.n;

    return ret;
}

u16_t sclient_level_list(struct sclient *c, char *buff, int len)
{
    u16_t ret = 0;

    ret += snprintf(buff+ret, len-ret, "****** tcps client level list summary ******\r\n");

    struct sclient_dump_cb_t data = {buff+ret, len-ret, 0};

    sclient_level_iterate(c, sclient_dump_cb, &data);

    ret += data.n;

    return ret;

}
u16_t sclient_line_dump(struct sclient *c, char *buff, u16_t len)
{
    u16_t ret = 0;
    
    ret += snprintf(buff+ret, len-ret, "| %-15p | %-15p | %-15p | %-5d | %-15s | %-5u | %-10s |\r\n", 
            c->l, c, c->r, c->fd, c->ip, c->port, c->ltime);


    return ret;
}
void sclient_dump_cb(struct sclient *c, void *data)
{
    struct sclient_dump_cb_t *f = (struct sclient_dump_cb_t *)data;

    f->n += snprintf(f->buff+f->n, f->len-f->n, SOCK_SPLIT);
    f->n += sclient_line_dump(c, f->buff+f->n, f->len-f->n);

    return;
}

static void sclient_size_cb(struct sclient *cur, void *data)
{
    (*(int *)data)++;
    return;
}

void sclient_tree_node_print(struct sclient *n, int level, const char *fmt, ...)
{
    va_list ap;
    int i = 0;

    for(i=0; i < level; i++)
        putchar('\t');

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    return;
}

void sclient_tree_node_recursive(struct sclient *n, int level)
{
    if(NULL == n)
        sclient_tree_node_print(n, level, "%c\r\n", '~');
    else
    {
        sclient_tree_node_recursive(n->r, level+1);
        sclient_tree_node_print(n, level, "%d\r\n", n->fd);
        sclient_tree_node_recursive(n->l, level+1);
    }

    return;
}

void sclient_tree_print(struct sclient *c)
{
    sclient_tree_node_recursive(c, 1);
    return;
}

