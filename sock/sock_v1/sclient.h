#ifndef __SCLIENT_H__
#define __SCLIENT_H__

#include "btype.h"
#include "sutil.h"

struct sclient{
    struct sclient *l;          /* left child */
    struct sclient *r;          /* right child */
    int fd;                     /* client's fd */
    u16_t port;                 /* client's port */
    char ip[SOCK_IP_LEN];       /* client's ip  */
    char ltime[SOCK_TIME_LEN];  /* client's link time */
};

struct sclient *sclient_new(int fd, struct sockaddr_in *caddr);
void sclient_copy(struct sclient *d, struct sclient *s);
void sclient_del(struct sclient *c);
void sclient_msg_print(struct sclient *c, u8_t *msg, size_t len);
void sclient_line_print(struct sclient *c);
#endif
