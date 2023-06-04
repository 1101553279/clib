#include "command.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"
#include "blist.h"
#include "log.h"
#include <unistd.h>
#include "cfg.h"
#define _GNU_SOURCE
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "cbasic.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "argtable3.h"

#define CMD_TDNAME      "cmd_routine"
#define CMD_MAX_ARGC    10
#define CMD_SRV_PORT    3000
#define CMD_BUFF_SIZE   4096

struct cmd_manager{
    u8_t init;
    int sfd;                /* server file description */
    pthread_t tid;          /* thread id */

    /* client information */
    struct sockaddr_in caddr;
    socklen_t clen;

    /* for reply message to PC host */
    char buff[CMD_BUFF_SIZE];
};

static struct cmd_manager cm_obj;

static int cmd_do_hand(char *buff, struct sockaddr_in *cip, socklen_t clen);
static void *cmd_routine(void *arg);
static int cmd_srv_init(u16_t port);

static int cmd_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds);
/* init command module */
void cmd_init(void)
{
    struct cmd_manager *cm = &cm_obj;
    int ret;


    /* read port frome ini configure file */
    cm->sfd = cmd_srv_init((u16_t )atoi(cfg_read("cmd", "port", NULL)));
    if(cm->sfd < 0)
    {
        log_red("srv init fail!\n");
        goto err_srv;
    }
    /* this task is for recviving command from PC host */
    ret = pthread_create(&cm->tid, NULL, cmd_routine, NULL);
    if(ret < 0)
    {
        log_red("thread new fail!\n");
        goto err_pthread;
    }
//    pthread_setname_np(cm->tid, CMD_TDNAME);
    
    cm->init = 1;
    
    arg_set_module_name("cmd");
    arg_set_module_version(1, 0, 0, "argtable3");
    arg_cmd_init();

    /* basic command add */
    cbasic_init();
    
    arg_cmd_register("cmd", cmd_arg_cmdfn, "information about all commands");

    return;

err_pthread:
    close(cm->sfd);
err_srv:
    return;
}

/* command manager information */
int cmd_srv_fd(void)
{
    return cm_obj.sfd;
}
/* sender(PC host) information */
int cmd_client(struct sockaddr_in *addr, socklen_t *len)
{
    if(NULL==addr || NULL==len)
        return -1;
    
    *addr = cm_obj.caddr;
    *len = cm_obj.clen;

    return 0;
}

void cmd_uninit(void)
{
    
}


static int cmd_arg_cmdfn(int argc, char* argv[], arg_dstr_t ds)
{
    arg_cmd_itr_t itr = arg_cmd_itr_create();
    unsigned int count = 0;
    arg_cmd_info_t *info;

    arg_dstr_catf(ds, "/****** cmd list summary ******/\n");
    for(count = arg_cmd_count(); count-- > 0; arg_cmd_itr_advance(itr))
    {
        info = arg_cmd_itr_value(itr);
//        arg_dstr_catf(ds, "%-10s   - %s\n", arg_cmd_itr_key(itr), info->description);
        arg_dstr_catf(ds, "%-08s   - %s\n", info->name, info->description);
    }

    arg_cmd_itr_destroy(itr);

    return 0;
}


/* dump command module information */
int cmd_dump(arg_dstr_t ds)
{
    struct cmd_manager *cm = &cm_obj;
    struct sockaddr_in *paddr = &cm->caddr;
    u16_t ret = 0;
    u16_t count = 0;
    arg_cmd_itr_t itr = arg_cmd_itr_create();
    
    arg_dstr_catf(ds, "/*** command summary ***/\n");

    arg_dstr_catf(ds, "command list: ");
    for(count=arg_cmd_count(); count-- > 0; arg_cmd_itr_advance(itr))
        arg_cmd_itr_key(itr);
    arg_cmd_itr_destroy(itr);

    arg_dstr_catf(ds, "\n----------------------------\n");
    arg_dstr_catf(ds, "srv fd : %d\n", cm->sfd);
    arg_dstr_catf(ds, "tid    : %lu\n", cm->tid);
    arg_dstr_catf(ds, "caddr  : %s\n", inet_ntoa(paddr->sin_addr) );
    arg_dstr_catf(ds, "cport  : %d\n", ntohs(paddr->sin_port) );

    return 0;
}


/****** static function list ******/
static struct command **cmd_max_ppn_find(struct command **ppn)
{
    while(NULL != (*ppn)->r)
        ppn = &((*ppn)->r);

    return ppn;
}

struct command *cmd_new(char *name, char *spec, char *usage, command_cb_t func, void *user)
{
    struct command *c;

    c = malloc(sizeof(struct command));
    if(NULL != c)
    {
//        printf("add: name=%s, spec=%s, usage=%s, func=%p\r\n", name, spec, usage, func);
        c->l = NULL;        /* must do */
        c->r = NULL;
        c->name = name;
        c->spec = spec;
        c->usage = usage;
        c->func = func;
        c->user = user;
    }
    
    return c;
}


/****** static function list ******/
static int cmd_srv_init(u16_t port)
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



/* hand command from PC host */
static int cmd_do_hand(char *buff, struct sockaddr_in *cip, socklen_t clen)
{
    int argc = 0;
    char *argv[CMD_MAX_ARGC];
    struct cmd_manager *cm = &cm_obj;
    arg_dstr_t ds = arg_dstr_create();
    char *msg;

    if(NULL==buff || NULL==cip || 0==clen)
        return -1;

    /* command parse */
    plog(CMD, "recv command: %s(%d)\r\n", buff, strlen(buff));
    argc = str_parse(buff, argv, ARY_SIZE(argv));
    if(argc <= 0)
    {
        plog(CMD, "parse command: fail!\r\n");
        goto err_parse; 
    }

    /****** for argtable ******/
    if(NULL == arg_cmd_info(argv[0]))
    { 
        arg_dstr_catf(ds, "%s is not supported!\r\n", argv[0]);
        plog(CMD, "find command: is not supported!\r\n", argv[0]);
    }
    else
    {
        arg_cmd_dispatch(argv[0], argc, argv, ds);
        plog(CMD, "find command: is supported!\r\n", argv[0]);
    }

    msg = arg_dstr_cstr(ds);
     
    sendto(cm->sfd, msg, strlen(msg), 0, (struct sockaddr *)cip, clen );

    arg_dstr_destroy(ds);

    return 0;
err_parse:
    return -1;
}

static void *cmd_routine(void *arg)
{
    struct cmd_manager *cm = &cm_obj;
	struct sockaddr_in *paddr = &cm->caddr;
	socklen_t *plen = &cm->clen;
    char buff[100];
    ssize_t n;
   
    /* must do */
    *plen = sizeof(struct sockaddr_in);
    while(1)
    {
        memset(buff, 0, sizeof(buff));
        n = recvfrom(cm->sfd, buff, sizeof(buff)-1,
                MSG_WAITALL, (struct sockaddr*)paddr, plen);
        if(n < 0)
            continue;
        buff[n] = '\0'; /* append tail character */
        /* remove '\r' '\n' of the tail character */
        while(--n>0 && ('\r'==buff[n] || '\n'==buff[n]))
            buff[n] = '\0';
        if(n > 0)
            cmd_do_hand(buff, paddr, *plen);
    }
    return NULL;
}

