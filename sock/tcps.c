#include "tcps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "command.h"

struct tcps_fds_cb_t
{
    struct pollfd *fds;
    nfds_t size;    /* size */
    nfds_t n;       /* number */
};

static int tcps_command(int argc, char *argv[], char *buff, int len, void *user);
static void tcps_fds_cb(struct sclient *cur, void *data);

int tcps_init(struct sock_tcps *t, u16_t port)
{
    time_t c;

    sclient_init(&t->cm);
    t->fd = sock_fd_init(SOCK_TCP, port);
    if(-1 == t->fd)
        return -1;
    t->port = port;
    /* init time */
    time(&c);
    strftime(t->itime, sizeof(t->itime), "%H:%M:%S", localtime(&c));

    return 0;
}

int tcps_init_append(struct sock_tcps *t)
{
    cmd_add("tcps", "list tcps information", CMD_INDENT"tcps [fd|port|client <list|close>]", 
            tcps_command, t);

    return 0;
}

nfds_t tcps_fds_size(struct sock_tcps *t)
{
    /* server + clients */
    return 1 + sclient_size(&t->cm);
}

nfds_t tcps_fds(struct sock_tcps *t, struct pollfd *fds, nfds_t size)
{
    struct tcps_fds_cb_t data = {fds, size, 0};

    /* tcp server */
    if(data.n < size--)
    {
        fds[data.n].fd = t->fd;
        fds[data.n].events = POLLIN;
        data.n++;
    }
    tcps_iterate(t, tcps_fds_cb, &data);

    return data.n;
}

int tcps_fds_in(struct sock_tcps *t, int fd)
{
    return fd==t->fd || NULL!=sclient_find(&t->cm, fd);
}

int tcps_fds_hand(struct sock_tcps *t, struct pollfd *fds)
{
    nfds_t nfds = 0;
    int cfd;
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
    bzero(&caddr, sizeof(caddr));
    
    if(POLLIN & fds->revents)
    {
        if(fds->fd == t->fd) /* server event */
        {
            cfd = accept(t->fd, (struct sockaddr *)&caddr, &clen);
            sclient_add(&t->cm, cfd, &caddr);
            nfds++;
            /* for debug */
            tcps_print(t);
        }
        else
        {
            sclient_hand(&t->cm, fds);
            nfds++;
            tcps_print(t);
        }
    }

    return nfds;
}

void tcps_iterate(struct sock_tcps *t, tcps_iterate_t cb, void *data)
{
    sclient_iterate(&t->cm, cb, data);

    return;
}

void tcps_uninit(struct sock_tcps *t)
{
    sclient_uninit(&t->cm);
    close(t->fd);

    return;
}

void tcps_print(struct sock_tcps *t)
{
    SOCK_TITLE("****** tcps summary ******\r\n");
    SOCK_PRINT("srv fd    : %d\r\n", t->fd); 
    SOCK_PRINT("srv port  : %u\r\n", t->port);
    SOCK_PRINT("srv online: %s\r\n", t->itime);
    SOCK_PRINT("\tlist print:\r\n");
    SOCK_PRINT( SOCK_SPLIT );
    SOCK_PRINT("| %-15s | %-15s | %-15s | %-5s | %-15s | %-5s | %-10s | \r\n", 
                "left", "current", "right", "fd", "ip", "port", "link time");
    sclient_list_print(&t->cm); 
    SOCK_PRINT( SOCK_SPLIT );

    SOCK_PRINT("\ttree print:\r\n");
    sclient_tree_print(&t->cm);

    return;
}

u16_t tcps_dump(struct sock_tcps *t, char *buff, u16_t len)
{
    u16_t ret = 0;

    ret += snprintf(buff+ret, len-ret, "****** tcps summary ******\r\n");
    ret += snprintf(buff+ret, len-ret, "srv fd    : %d\r\n", t->fd); 
    ret += snprintf(buff+ret, len-ret, "srv port  : %u\r\n", t->port);
    ret += snprintf(buff+ret, len-ret, "srv online: %s\r\n", t->itime);
    ret += snprintf(buff+ret, len-ret, SOCK_SPLIT);
    ret += snprintf(buff+ret, len-ret, "| %-15s | %-15s | %-15s | %-5s | %-15s | %-5s | %-10s |\r\n", 
                                       "left", "current", "right", "fd", "ip", "port", "link time");

    struct sclient_dump_cb_t data = {buff+ret, len-ret, 0};
    sclient_iterate(&t->cm, sclient_dump_cb, &data);
    ret += data.n;
    ret += snprintf(buff+ret, len-ret, SOCK_SPLIT);

    return ret;
}

static void tcps_fds_cb(struct sclient *cur, void *data)
{
    struct tcps_fds_cb_t *f = (struct tcps_fds_cb_t *)data;
    if(f->size-- > 0)
    {
        f->fds[f->n].fd = cur->fd;
        f->fds[f->n].events = POLLIN;
        f->n++;
    }
}
static int tcps_command(int argc, char *argv[], char *buff, int len, void *user)
{
    u16_t ret = 0;
    struct sock_tcps *tcp = (struct sock_tcps *)user;
    u8_t mark = 1;
    int i = 0;

    if(2 == argc) 
    {
        if(0 == strcmp(argv[1], "fd"))
            ret += snprintf(buff+ret, len-ret, "tcps srv fd: %d\r\n", tcp->fd);
        else if(0 == strcmp(argv[1], "port"))
            ret += snprintf(buff+ret, len-ret, "tcps srv port: %u\r\n", tcp->port);
        else
            mark = 0;
    }
    else if(3 == argc)
    {
        if(0 == strcmp(argv[1], "client"))
        {
            if(0 == strcmp(argv[2], "list"))
                ret += sclient_list(&tcp->cm, buff+ret, len-ret);
            else if(0 == strcmp(argv[2], "uninit"))
            {
                sclient_uninit(&tcp->cm);
                ret += snprintf(buff+ret, len-ret, "****** tcps client uninit ******\r\n");
                ret += snprintf(buff+ret, len-ret, "all clients are uninited\r\n");
            }
            else
                mark = 0;
        }
        else
            mark = 0;
    }
    else if(4 == argc)
    {
        if(0 == strcmp(argv[1], "client") && 0 == strcmp(argv[2], "level") && 0 == strcmp(argv[3], "list"))
            ret += sclient_level_list(&tcp->cm, buff+ret, len-ret);
        else
            mark = 0;
    }
    else
        mark = 0;

    if(0 == mark)
    {
        for(i=0; i < argc; i++)
            ret += snprintf(buff+ret, len-ret, "%s ", argv[i]);
        ret += snprintf(buff+ret, len-ret, " <- no this cmd\r\n");
    }

    return ret;
}
