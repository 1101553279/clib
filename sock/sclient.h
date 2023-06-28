#ifndef __SCLIENT_H__
#define __SCLIENT_H__

#include "include/btype.h"
#include "sutil.h"
#include "cmd/argtable3.h"

/* client is managed by the AVL tree */
struct sclient{
    struct sclient *p;          /* father node */
    struct sclient *l;          /* left child */
    struct sclient *r;          /* right child */
    int factor;                 /* balance factor */
    int fd;                     /* client's fd */
    u16_t port;                 /* client's port */
    char ip[SOCK_IP_LEN];       /* client's ip  */
    char ltime[SOCK_TIME_LEN];  /* client's link time */
};
struct sclient_mgr{
    struct sclient *root;       /* AVL tree's root */
};

struct sclient_dump_cb_t{
    char *buff;
    u16_t len;
    u16_t n;
};

typedef void (*sclient_iterate_t)(struct sclient *c, void *data);

void sclient_init(struct sclient_mgr *m);
int sclient_add(struct sclient_mgr *m, int fd, struct sockaddr_in *addr);
int sclient_del(struct sclient_mgr *m, int fd);
struct sclient *sclient_find(struct sclient_mgr *m, int fd);
void sclient_iterate(struct sclient_mgr *m, sclient_iterate_t cb, void *data);
void sclient_level_iterate(struct sclient_mgr *m, sclient_iterate_t cb, void *data);
int sclient_size(struct sclient_mgr *m);
int sclient_hand(struct sclient_mgr *m, struct pollfd *fds);
void sclient_uninit(struct sclient_mgr *m);
/****** for debug ******/
void sclient_msg_print(struct sclient *c, u8_t *msg, size_t len);
void sclient_del_print(struct sclient *c);
void sclient_list_print(struct sclient_mgr *c);
int sclient_list(struct sclient_mgr *m, arg_dstr_t ds);
int sclient_level_list(struct sclient_mgr *m, arg_dstr_t ds);
int sclient_line_dump(struct sclient *c, arg_dstr_t ds);
void sclient_dump_cb(struct sclient *c, void *data);
void sclient_tree_print(struct sclient_mgr *m);
#endif
