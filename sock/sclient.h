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

struct sclient_dump_cb_t
{
    char *buff;
    u16_t len;
    u16_t n;
};

typedef void (*sclient_iterate_t)(struct sclient *c, void *data);

struct sclient *sclient_new(int fd, struct sockaddr_in *caddr);
struct sclient **sclient_prev_ppn_find(struct sclient *cur);
struct sclient **sclient_ppn_find(struct sclient **ppn, int fd);
struct sclient **sclient_max_ppn_find(struct sclient *cur);
int sclient_add(struct sclient **ppn, int fd, struct sockaddr_in *addr);
int sclient_del(struct sclient **ppn, int fd);
int sclient_close(struct sclient *pn);
struct sclient *sclient_find(struct sclient *pn, int fd);
void sclient_swap(struct sclient *d, struct sclient *s);
void sclient_iterate(struct sclient *cur, sclient_iterate_t cb, void *data);
int sclient_size(struct sclient *cur);
int sclient_hand(struct sclient **ppn, struct pollfd *fds);
struct sclient *sclient_uninit(struct sclient *cur);

void sclient_msg_print(struct sclient *c, u8_t *msg, size_t len);
void sclient_del_print(struct sclient *c);
void sclient_line_print(struct sclient *c);
void sclient_print(struct sclient *c);
u16_t sclient_list(struct sclient *c, char *buff, int len);
u16_t sclient_line_dump(struct sclient *c, char *buff, u16_t len);
void sclient_dump_cb(struct sclient *c, void *data);
#endif
