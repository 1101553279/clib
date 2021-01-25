#ifndef __USRV_H__
#define __USRV_H__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

struct client{
    int sfd;
	struct sockaddr_in addr;  /* client information for reply */
    socklen_t len;
};

#define USRV_CLIENT(srv)    (&((srv)->client))

struct usrv;
typedef int (*usrv_callback_t)(struct usrv *usrv, unsigned char *buff, unsigned int len);

struct usrv{
	int sfd;    /* server fd */
	unsigned short sport; /* server port */
	usrv_callback_t cb;
	struct client client; /* the lastest client information */
};

int usrv_init(struct usrv *us, unsigned short port, usrv_callback_t cb);
int usrv_client_reply(struct client *cli, unsigned char *msg, int len);

#endif

