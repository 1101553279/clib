#ifndef __SRV_H__
#define __SRV_H__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "list.h"
#include "poll.h"

struct srv{
    int sfd;    /* server fd */
    struct list_head clist; /* client list */
    char stime[100];    /* setup time */
};
int srv_init(struct srv *srv, unsigned short port);
int srv_get_sfd(struct srv *srv);
int srv_rmv_client(struct srv *srv, int cfd); 
int srv_nfds_cnt(struct srv *srv);
int srv_client_cnt(struct srv *srv);
int srv_nfds_add(struct srv *srv, struct pollfd *fds, nfds_t nfds);
void server_do(struct srv *srv);
void server_client_do(struct srv *srv, int fd);
void srv_uninit(struct srv *srv);
#define SRV_SIMPLE     "simple"
#define SRV_VERBOSE    "verbose"
void srv_dump(struct srv *srv, const char *mode);

#endif
