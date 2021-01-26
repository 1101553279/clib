#include "command.h"
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "blist.h"
#include "log.h"
#include "plog.h"
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "cbasic.h"

#define CMD_MAX_ARGC    10
#define CMD_SRV_PORT    3000
#define CMD_BUFF_SIZE   2048

struct cmd_manager{
    struct list_head hd;    /* command list head */
    int sfd;                /* server file description */
    pthread_t tid;          /* thread id */

    /* client information */
    struct sockaddr_in caddr;
    socklen_t clen;

    /* for reply message to PC host */
    char buff[CMD_BUFF_SIZE];
};

enum{
    NOT_FOUND,
    MAX_FAIL,
};
const char *fail_list[MAX_FAIL] = {
    "command not found\n",
};
static struct cmd_manager cm_obj;

static void *__cmd_routine(void *arg);
static int __srv_init(u16_t port);
static int __cmd_reply(const char *buff, struct sockaddr_in *cip, socklen_t clen);
/* init command module */
void cmd_init(void)
{
    struct cmd_manager *cm = &cm_obj;
    int ret;
    
    INIT_LIST_HEAD(&cm->hd);

    cm->sfd = __srv_init(CMD_SRV_PORT);
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
    
    /* basic command add */
    cbasic_init();
    
    return;

err_pthread:
    close(cm->sfd);
err_srv:
    return;
}
/* for help command */
struct list_head *cmd_head(void)
{
    return &cm_obj.hd;
}

/* find command */
struct command *cmd_find(char *name)
{
    struct command *c = NULL;
    struct cmd_manager *cm = &cm_obj; 
    struct list_head *pos;

    if(NULL == name)
        return NULL;

    /* iterate command list */
    list_for_each(pos, &cm->hd)
    {
        c = list_entry(pos, struct command, head);
        if(0 == strcmp(c->name, name))
            return c;
    }

    return NULL;
}
/* fill information into buff */
int cmd_info_iterate(char *buff, int len, command_info_iterate_cb_t cb, char *title)
{
    int ret = 0;
    struct cmd_manager *cm = &cm_obj;
    struct list_head *pos = NULL;
    struct command *c = NULL;

    /* fill header information */
    ret += snprintf(buff+ret, len-ret, "%s", title);
    /* fill each command's information */
    list_for_each(pos, &cm->hd)
    {
        c = list_entry(pos, struct command, head);
        ret += cb(c, buff+ret, len-ret); 
    }
   
    return ret;
}
/* add a command */
int cmd_add(char *name, char *spec, char *usage, command_cb_t func, void *user)
{
    struct cmd_manager *cm = &cm_obj;
    struct command *c = NULL;
    if(NULL==name || NULL==func)
    {
        log_red("command add fail: %s == NULL\n", (NULL==name)? "name": "func");
        return -1;
    }
    /* decide whether this command has added */ 
    c = cmd_find(name);
    if(NULL != c)
    {
        log_red("command '%s' has added!\n", name);
        return -1;
    }

    c = (struct command *)malloc(sizeof(struct command));
    if(NULL == c)
    {
        log_red("command add fail: malloc fail!\n");
        return -1;
    }

    /* init a command information */
    INIT_LIST_HEAD(&c->head);
    c->name = name;
    c->spec = spec;
    c->usage = usage;
    c->func = func;
    c->user = user;

    /* add to command list */
    list_add(&c->head, &cm->hd);

    return 0;
}

/* delete a command from command list */
int cmd_rmv(char *name)
{
    struct command *c = NULL;
    if(NULL == name)
    {
        log_red("name == NULL\n");
        return -1;
    }

    /* search this command */
    c = cmd_find(name);
    if(NULL == c)
    {
        log_red("not found this command\n");
        return -1; 
    }
    /* delete from command list */
    list_del(&c->head);

    /* free command space */
    free(c);

    return 0;
}

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

/* reply message to PC host */
static int __cmd_reply(const char *buff, struct sockaddr_in *cip, socklen_t clen)
{
    struct cmd_manager *cm = &cm_obj;

    if(NULL==buff || NULL==cip || 0==clen)
    {
        plog(CMD, "parameter error: msg=%s, cip=%p, clen=%d\n", buff, cip, clen);
        return -1;
    }
   
    return sendto(cm->sfd, buff, strlen(buff), 0, (struct sockaddr *)cip, clen );
}

/* reply fail message to PC host */
static int __fail_reply(u16_t id, struct sockaddr_in *cip, socklen_t clen)
{
    if(id >= MAX_FAIL)
        return -1;

    __cmd_reply(fail_list[id], cip, clen);

    return 0;
}

/* execute a command */
static int __cmd_exe(struct command *cmd, int argc, char *argv[], struct sockaddr_in *cip, socklen_t clen)
{
    struct cmd_manager *cm = &cm_obj;
    int len;

    if(NULL==cmd || 0==argc || NULL==argv || NULL==cip || 0==clen)
    {
        plog(CMD, "parameter error: cmd=%p, argc=%d, argv=%p, cip=%p, clen=%d\n", 
                cmd, argc, argv, cip, clen);
        return -1;
    }

    if(cmd && cmd->func) 
    {
        len = cmd->func(argc, argv, cm->buff, sizeof(cm->buff), cmd->user);
        if(len > 0)
            __cmd_reply(cm->buff, cip, clen);   /* send command execute log to PC host */
    }

    return 0;
}

/* hand command from PC host */
static int __do_hand(char *buff, struct sockaddr_in *cip, socklen_t clen)
{
    int argc = 0;
    char *argv[CMD_MAX_ARGC];
    int i = 0;
    struct command *c = NULL;
    struct cmd_manager *cm = &cm_obj;

    /* command parse */
    argc = str_parse(buff, argv, ARY_SIZE(argv));
    if(argc <= 0)
        return -1;
#if 1
    printf("argc list: %d\n", argc);
    for(i=0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");
#endif
    c = cmd_find(argv[0]);
    if(NULL == c)
    {
        __fail_reply(NOT_FOUND, cip, clen);
        return -1;
    }

    plog(CMD, "command search success!\n");
     
    if(__cmd_exe(c, argc, argv, cip, clen))
        return -1;

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
        __do_hand(buff, &cliaddr, clilen);
    }
    return NULL;
}

