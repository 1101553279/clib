#include "sclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

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

void sclient_copy(struct sclient *d, struct sclient *s)
{
    d->fd = s->fd;
    d->port = s->port;
    strcpy(d->ip, s->ip);
    strcpy(d->ltime, s->ltime);
}

void sclient_del(struct sclient *c)
{
    if(NULL != c)
    {
        c->l = c->r = NULL;
        free(c);
    }
}
void sclient_msg_print(struct sclient *c, u8_t *msg, size_t len)
{
    SOCK_PRINT("(%s:%u(%d)) # %lu # %s\r\n", c->ip, c->port, c->fd, len, msg);
}

void sclient_line_print(struct sclient *c)
{
    SOCK_PRINT("| %-15p | %-5d | %-15s | %-5u | %-10s | %-15p |\r\n", 
            c->l, c->fd, c->ip, c->port, c->ltime, c->r);
}
