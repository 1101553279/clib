#ifndef __POLL_H__
#define __POLL_H__

typedef unsigned int u32_t;
typedef unsigned short u16_t;

struct client{
    int use;
    int fd;
    char ip[32];
    u16_t port;
};

#endif
