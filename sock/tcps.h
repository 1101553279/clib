#ifndef __TCPS_H__
#define __TCPS_H__
#include "sclient.h"
#include "btype.h"
#include "sutil.h"
#include <poll.h>

struct sock_tcps{
    struct sclient_mgr cm;      /* client manager */
    int fd;                     /* server fd */
    u16_t port;                 /* server port */
    char itime[SOCK_IP_LEN];    /* init time */
};

typedef sclient_iterate_t tcps_iterate_t;

int tcps_init(struct sock_tcps *t, u16_t port);
int tcps_init_append(struct sock_tcps *t);
nfds_t tcps_fds_size(struct sock_tcps *t);
nfds_t tcps_fds(struct sock_tcps *t, struct pollfd *fds, nfds_t size);
int tcps_fds_in(struct sock_tcps *t, int fd);
int tcps_fds_hand(struct sock_tcps *t, struct pollfd *fds);
void tcps_iterate(struct sock_tcps *t, tcps_iterate_t cb, void *data);
void tcps_uninit(struct sock_tcps *t);
void tcps_print(struct sock_tcps *t);
int tcps_dump(struct sock_tcps *t, arg_dstr_t ds);
#endif
