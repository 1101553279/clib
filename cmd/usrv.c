#include "usrv.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>

#define MAXLINE 1024 

static void client_init(struct client *ct, int sfd, struct sockaddr_in addr, socklen_t len)
{
    if(ct)
    {
        bzero(ct, sizeof(struct client));
        ct->sfd = sfd;
        memcpy(&ct->addr, &addr, len);		
        ct->len = len;
    }
    return;
}

void *start_routine(void *arg)
{
	struct usrv *us = (struct usrv*)arg;
    /* client address */
	struct sockaddr_in cliaddr;
	socklen_t clilen;
    /* client message */
	unsigned char buffer[ MAXLINE ];
    ssize_t n;
    int i = 0;

    while(1)
    {
        bzero(buffer, sizeof(buffer));
        clilen = sizeof(cliaddr);
        n = recvfrom(us->sfd, buffer, MAXLINE-1, 
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                    &clilen); 
		buffer[n] = '\0';	// append tail character */
#if 0
        printf("%s:%u:%lu: %s\n", 
                inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n, buffer);
#endif

        client_init(&us->client, us->sfd, cliaddr, clilen);
        /* callback */
        us->cb(us, buffer, n);
    }

	return NULL;
}

int usrv_init(struct usrv *us, unsigned short port, usrv_callback_t cb)
{
	struct sockaddr_in servaddr;
	int ret = 0;
	pthread_t thread;
	
	us->sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(us->sfd < 0)
	{
		printf("socket new failed\r\n");
		goto err_socket;
	}
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(port); 
	// Bind the socket with the server address 
	if ( bind(us->sfd, (struct sockaddr *)&servaddr, 
			sizeof(servaddr)) < 0 ) 
	{ 
		printf("socket bind fail!\r\n");
		goto err_bind;
	} 
	us->sport = port;
	us->cb = cb;
	ret = pthread_create(&thread, NULL, start_routine, us);
	if(ret < 0)
	{
		printf("pthread create fail!\r\n");
		goto err_pcreate;
	}
	
	return 0;

err_pcreate:
err_bind:
	close(us->sfd);
err_socket:
	memset(us, 0, sizeof(struct usrv));
	return -1;
}
/* reply msg for client */
int usrv_client_reply(struct client *cli, unsigned char *msg, int len)
{
//    printf("sfd = %d, msg=%s, len=%d\n", cli->sfd, msg, len);
	return sendto(cli->sfd, msg, len, \
		MSG_CONFIRM, (struct sockaddr *) &cli->addr, cli->len);    
}
