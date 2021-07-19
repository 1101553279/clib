#ifndef __CLIENT_H__
#define __CLIENT_H__
#include "list.h"

struct client{
    struct list_head head;
    int fd;             /* client file description */
    char ip[20];        /* client ip */
    unsigned short port;/* client port */
    char ctime[100];    /* connect time */
};


#endif
