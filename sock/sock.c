#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sock.h"
#include "sclient.h"
#include "tcps.h"
#include "udps.h"
#include "btype.h"
#include <pthread.h>
#include "command.h"
#include "cfg.h"

struct sock{
    pthread_t tid;          /* pthread id */
    struct sock_tcps tcp;
};

struct sock sock_obj;

static void *sock_routine(void *arg);
int sock_init(void)
{
    struct sock *sock = &sock_obj;

    if(-1 == tcps_init(&sock->tcp, (u16_t )atoi(cfg_read("sock", "tport", NULL))))
        return -1;

    if(0 != pthread_create(&sock->tid, NULL, sock_routine, NULL))
        return -1;

    return 0;
}

void sock_init_append()
{
    tcps_init_append(&sock_obj.tcp);

    return;
}

static nfds_t sock_poll_init(struct sock *sock, struct pollfd **fds)
{
    nfds_t offset = 0;
    nfds_t nfds = 0;
    
    nfds = tcps_fds_size(&sock->tcp)/*+udp_fds_size(&sock->udp)*/;
    *fds = malloc(sizeof(struct pollfd) * nfds);
    if(NULL == *fds)
        return 0;

    offset += tcps_fds(&sock->tcp, *fds+offset, nfds-offset);
    
    return nfds;
}

static void sock_poll_hand(struct sock *sock, struct pollfd *fds, nfds_t nfds, nfds_t efds)
{
    nfds_t i = 0;

    for(i=0; i < nfds && efds > 0; i++)
    {
        if(tcps_fds_in(&sock->tcp, fds[i].fd))
            efds -= tcps_fds_hand(&sock->tcp, fds+i);
    }


    return;
}
static void sock_poll_uninit(struct pollfd *fds)
{
    free(fds);
    return;
}

static void *sock_routine(void *arg)
{
    struct sock *sock = &sock_obj;
    struct pollfd *fds;
    nfds_t nfds = 0;      /* number fds */
    int efds = 0;       /* event fds */

    while(1)
    {
//        printf("sock start ------------------\r\n");
        nfds = sock_poll_init(sock, &fds);
        if(nfds < 0)
            continue;
        efds = poll(fds, nfds, -1);
        if(efds > 0)
            sock_poll_hand(sock, fds, nfds, efds);

        sock_poll_uninit(fds);
    }
    return NULL;
}

u16_t sock_tcps_dump(char *buff, u16_t len)
{
    return tcps_dump(&(sock_obj.tcp), buff, len);
}
