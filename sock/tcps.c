#include "tcps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "command.h"
#include "util.h"

struct tcps_fds_cb_t
{
    struct pollfd *fds;
    nfds_t size;    /* size */
    nfds_t n;       /* number */
};

static int tcps_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds);
static void tcps_fds_cb(struct sclient *cur, void *data);

struct sock_tcps *tcp = NULL;

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

    tcp = t;

    return 0;
}

int tcps_init_append(struct sock_tcps *t)
{
    arg_cmd_register("tcps", tcps_arg_cmdfn, "manage tcps module");

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

int tcps_dump(struct sock_tcps *t, arg_dstr_t ds)
{
    u16_t ret = 0;

    arg_dstr_catf(ds, "****** tcps summary ******\r\n");
    arg_dstr_catf(ds, "srv fd    : %d\r\n", t->fd); 
    arg_dstr_catf(ds, "srv port  : %u\r\n", t->port);
    arg_dstr_catf(ds, "srv online: %s\r\n", t->itime);
    arg_dstr_catf(ds, SOCK_SPLIT);
    arg_dstr_catf(ds, "| %-15s | %-15s | %-15s | %-5s | %-15s | %-5s | %-10s |\r\n", 
                       "left", "current", "right", "fd", "ip", "port", "link time");

    sclient_iterate(&t->cm, sclient_dump_cb, ds);
    arg_dstr_catf(ds, SOCK_SPLIT);

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
/*
    cmd_add("tcps", "list tcps information\r\n","tcps [fd|port|client <list|close>]\r\n", 
            tcps_command, t);
    tcps                    - tcps usage
    tcps -d|dump            - dump module information
    tcps -c uninit          - uninit 
    tcps -c list
    tcps -c level
*/
static int tcps_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds)
{
    struct arg_lit *arg_d;
    struct arg_str *arg_c;
    struct arg_end *end_arg;
    int i = 0;
    void *argtable[] = 
    {
        arg_d       = arg_lit0("d", "dump",             "dump information about tcps module"),
        arg_c       = arg_str0("c", "client", "[action]", "what action you will do for client"),
                      arg_rem(NULL, "list    - print client in list view"),
                      arg_rem(NULL, "level   - print client in level view"),
                      arg_rem(NULL, "uninit  - uninit all clients"),
        end_arg     = arg_end(5),
    };
    int nerrors;
    const char *name = argv[0];
    arg_cmd_info_t *info;
    int ret = 0;
    const char *action;

    ret = argtable_parse(argc, argv, argtable, end_arg, ds, name);
    if(0 != ret)
        goto to_parse;

    if(arg_d->count > 0)
    {
        arg_dstr_catf(ds, "tcps srv fd  : %d\r\n", tcp->fd);
        arg_dstr_catf(ds, "tcps srv port: %u\r\n", tcp->port);
        goto to_d;
    }

    if(arg_c->count > 0)
    {
        action = arg_c->sval[0];
        if(0 == strcmp("uninit", action))
        {
                sclient_uninit(&tcp->cm);
                arg_dstr_catf(ds, "****** tcps client uninit ******\r\n");
                arg_dstr_catf(ds, "all clients are uninited\r\n");
        }
        else if(0 == strcmp("list", action))
            sclient_list(&tcp->cm, ds);
        else if(0 == strcmp("level", action))
            sclient_level_list(&tcp->cm, ds);
        else
            arg_dstr_catf(ds, "<%s> is not supported!\r\n", action);
        goto to_c;
    }

    /* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", name);
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-25s %s\r\n");

to_c:
to_d:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}
