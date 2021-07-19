#ifndef __TCPS_H__
#define __TCPS_H__
#include "sclient.h"
#include "btype.h"
#include "sutil.h"
#include <poll.h>

struct sock_tcps{
    struct sclient *root;       /* client's tree root */
    int fd;                     /* server fd */
    u16_t port;
    char itime[SOCK_IP_LEN];    /* init time */
};

int tcps_init(struct sock_tcps *t, u16_t port);
int tcps_client_add(struct sock_tcps *t, int fd, struct sockaddr_in *addr);
int tcps_client_del(struct sock_tcps *t, int fd);
struct sclient *tcps_client_find(struct sock_tcps *t, int fd);
int tcps_fds_in(struct sock_tcps *t, int fd);
int tcps_client_hand(struct sock_tcps *t, struct pollfd *fds);
int tcps_fds_hand(struct sock_tcps *t, struct pollfd *fds);
nfds_t tcps_fds_size(struct sock_tcps *t);
nfds_t tcps_fds(struct sock_tcps *t, struct pollfd *fds, nfds_t size);
void tcps_uninit(struct sock_tcps *t);

void tcps_tree_print(struct sock_tcps *t);
u16_t tcps_dump(struct sock_tcps *t, char *buff, u16_t len);
#endif
