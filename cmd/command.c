#include "command.h"
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "blist.h"
#include "log.h"
#include <unistd.h>
#include <pthread.h>

#define CMD_MAX_ARGC 10
#define SRV_PORT 3000

struct cmd_manager{
    struct list_head hd;    /* command list head */
    int sfd;        /* server file description */
    pthread_t tid;  /* thread id */
};

static struct cmd_manager cm_obj;

static void *__cmd_routine(void *arg);
static int __srv_init(u16_t port);
/* init command module */
void cmd_init(void)
{
    struct cmd_manager *cm = &cm_obj;
    int ret;
    
    INIT_LIST_HEAD(&cm->hd);

    cm->sfd = __srv_init(SRV_PORT);
    if(cm->sfd < 0)
    {
        log_red("srv init fail!\n");
        goto err_srv;
    }
    /* this task is for recviving command from PC host */
    ret = pthread_create(&cm->tid, NULL, __cmd_routine, NULL);
    if(ret < 0)
    {
        log_red("thread new fail!\n");
        goto err_pthread;
    }
    
    return;

err_pthread:
    close(cm->sfd);
err_srv:
    return;
}
int cmd_add(char *name, char *spec, char *usage, command_cb_t func, void *user);
int cmd_rmv(char *name);

/****** static function list ******/
static int __srv_init(u16_t port)
{
	struct sockaddr_in servaddr;
    int sfd;
	
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd < 0)
	{
		log_red("socket new failed\r\n");
		goto err_socket;
	}
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(port); 
	// Bind the socket with the server address 
	if ( bind(sfd, (struct sockaddr *)&servaddr, 
			sizeof(servaddr)) < 0 ) 
	{ 
		log_red("socket bind fail!\r\n");
		goto err_bind;
	} 
    return sfd;

err_bind:
	close(sfd);
err_socket:
	return -1;
}

static int do_hand(char *buff, struct sockaddr_in *cip, socklen_t clen)
{
    int argc = 0;
    char *argv[CMD_MAX_ARGC];
    int i = 0;

    /* command parse */
    argc = str_parse(buff, argv, ARY_SIZE(argv));
    if(argc <= 0)
        return -1;
#if 0
    printf("argc list: %d\n", argc);
    for(i=0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");
#endif

    return 0;
}

static void *__cmd_routine(void *arg)
{
    struct cmd_manager *cm = &cm_obj;
	struct sockaddr_in cliaddr;
	socklen_t clilen;
    char buff[100];
    ssize_t n;
   
    /* must do */
    clilen = sizeof(cliaddr);
    while(1)
    {
        memset(buff, 0, sizeof(buff));
        n = recvfrom(cm->sfd, buff, sizeof(buff)-1,
                MSG_WAITALL, (struct sockaddr*)&cliaddr, &clilen);
        buff[n] = '\0'; /* append tail character */
        do_hand(buff, &cliaddr, clilen);
    }
    return NULL;
}
