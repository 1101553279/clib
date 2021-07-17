#include "command.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"
#include "blist.h"
#include "log.h"
#include <unistd.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "cbasic.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CMD_TDNAME      "shell routine"
#define CMD_MAX_ARGC    10
#define CMD_SRV_PORT    3000
#define CMD_BUFF_SIZE   2048

struct cmd_manager{
    struct command *root;   /* root pointer */
    int sfd;                /* server file description */
    pthread_t tid;          /* thread id */

    /* client information */
    struct sockaddr_in caddr;
    socklen_t clen;

    /* for reply message to PC host */
    char buff[CMD_BUFF_SIZE];
};

enum{
    CMD_PARSE_ERR,
    CMD_NOT_FOUND,
    MAX_FAIL,
};
const char *cmd_fail_list[MAX_FAIL] = {
    "command parse fail\n",
    "command not found\n",
};
static struct cmd_manager cm_obj;

static int cmd_do_hand(char *buff, struct sockaddr_in *cip, socklen_t clen);
static void *cmd_routine(void *arg);
static int cmd_srv_init(u16_t port);
static int cmd_reply(const char *buff, struct sockaddr_in *cip, socklen_t clen);
static int cmd_fail_reply(u16_t id, struct sockaddr_in *cip, socklen_t clen);
static struct command *cmd_node_find(struct command *pn, char *name);
static u16_t cmd_iterate(struct command *tree, char *buff, u16_t len, command_info_iterate_cb_t cb);
static void cmd_tree_node_print(struct command *pn, int level);
static void cmd_tree_node_value_print(char ch, int level, char *fmt, ...);
static u16_t cmd_dump_literate_cb(struct command *c, char *buff, u16_t len);
static int cmd_exe(struct command *cmd, int argc, char *argv[], struct sockaddr_in *cip, socklen_t clen);
/* init command module */
void cmd_init(void)
{
    struct cmd_manager *cm = &cm_obj;
    int ret;

    cm->root = NULL;

    cm->sfd = cmd_srv_init(CMD_SRV_PORT);
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
    
    /* basic command add */
    cbasic_init();
    
    return;

err_pthread:
    close(cm->sfd);
err_srv:
    return;
}

/* find command */
struct command *cmd_find(char *name)
{
    struct cmd_manager *cm = &cm_obj; 

    if(NULL == name)
        return NULL;

    return cmd_node_find(cm->root, name);
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
/* fill information into buff */
u16_t cmd_info_iterate(char *buff, u16_t len, command_info_iterate_cb_t cb, char *title)
{
    u16_t ret = 0;
    struct cmd_manager *cm = &cm_obj;
    
    if(NULL==buff || 0==len || NULL==cb)
        return 0;

    /* fill header information */
    if(NULL != title)
        ret += snprintf(buff+ret, len-ret, "%s", title);
    /* fill each command's information */
    ret += cmd_iterate(cm->root, buff+ret, len-ret, cb);
   
    return ret;
}

struct command **cmd_ppn_find(struct command **ppn, char *name)
{
    int ret = 0;

    while(NULL != *ppn)
    {
        ret = strcmp(name, (*ppn)->name);
        if(ret < 0)
            ppn = &((*ppn)->l);
        else if(ret > 0)
            ppn = &((*ppn)->r);
        else                /* find it */
            break;
    }

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

/* add a command */
int cmd_add(char *name, char *spec, char *usage, command_cb_t func, void *user)
{
    struct cmd_manager *cm = &cm_obj;
    struct command **ppn;

    if(NULL==name || NULL==func)
    {
        log_red("command add fail: %s == NULL\n", (NULL==name)? "name": "func");
        return -1;
    }

    ppn = cmd_ppn_find(&(cm->root), name);
    if(NULL != *ppn)
        log_red("command '%s' has been added -> substitute\r\n", name);

    *ppn = cmd_new(name, spec, usage, func, user);

    return NULL!=*ppn? 0: -1;
}

int cmd_insert(char *name, char *spec, char *usage, command_cb_t func, void *user)
{
    struct cmd_manager *cm = &cm_obj;
    struct command **ppn;

    if(NULL==name || NULL==func)
    {
        log_red("command insert fail: %s == NULL\n", (NULL==name)? "name": "func");
        return -1;
    }

    ppn = cmd_ppn_find(&(cm->root), name);
    if(NULL != *ppn)
    {
        log_red("command '%s' has been added -> substitute\r\n", name);
        copy_replace(*ppn, name, spec, usage, func, user);
    }
    else
        *ppn = cmd_new(name, spec, usage, func, user);

    return NULL!=*ppn? 0: -1;

}

int cmd_copy(struct command *dst, struct command *src)
{
    dst->name = src->name;
    dst->spec = src->spec;
    dst->usage = src->usage;
    dst->func = src->func;
    dst->user = src->user;

    return 0;
}

int copy_replace(struct command *pn, char *name, char *spec, char *usage, command_cb_t func, void *user)
{
    if(NULL == pn)
        return -1;

    pn->name = name;
    pn->spec = spec;
    pn->usage = usage;
    pn->func = func;
    pn->user = user;

    return 0;
}

/* delete a command from command list */
int cmd_rmv(char *name)
{
    struct cmd_manager *cm = &cm_obj; 
    struct command *c = NULL;
    struct command **ppn;

    if(NULL == name)
    {
        log_red("name == NULL\n");
        return -1;
    }
    
    ppn = cmd_ppn_find(&(cm->root), name);
    if(NULL == *ppn)
    {
        log_red("not found this command\n");
        return -1; 
    }
    
    c = *ppn;
    if(NULL!=c->l && NULL!=c->r)
    {
        ppn = &((*ppn)->l);
        while(NULL != (*ppn)->r)
            ppn = &((*ppn)->r);

        cmd_copy(c, *ppn);      /* copy */

        c = *ppn;     /* save it for freeing */
    }
    *ppn = (NULL==(*ppn)->l)? (*ppn)->r: (*ppn)->l;

    free(c);

    return 0;
}


u16_t cmd_list_dump(struct command *n, char *buff, u16_t len)
{
    u16_t ret = 0; 
    if(NULL != n)
    {
        ret += cmd_list_dump(n->l, buff+ret, len-ret);
        ret += snprintf(buff+ret, len-ret, "%s ", n->name);
        ret += cmd_list_dump(n->r, buff+ret, len-ret);
    }

    return ret;
}

/* dump command module information */
u16_t cmd_dump(char *buff, u16_t len)
{
    struct cmd_manager *cm = &cm_obj;
    struct sockaddr_in *paddr = &cm->caddr;
    u16_t ret = 0;
    
    if(NULL==buff || 0==len)
        return 0;

    ret += snprintf(buff+ret, len-ret, "/*** command summary ***/\n");
    ret += cmd_info_iterate(buff+ret, len-ret, cmd_dump_literate_cb, "command list: ");
    ret += snprintf(buff+ret, len-ret, "\n");
    ret += snprintf(buff+ret, len-ret, "srv fd : %d\n", cm->sfd);
    ret += snprintf(buff+ret, len-ret, "tid    : %lu\n", cm->tid);
    ret += snprintf(buff+ret, len-ret, "caddr  : %s\n", inet_ntoa(paddr->sin_addr) );
    ret += snprintf(buff+ret, len-ret, "cport  : %d\n", ntohs(paddr->sin_port) );

    return ret;
}

void cmd_tree_print(void)
{
   struct cmd_manager *cm = &cm_obj; 
   printf("*** cmd tree summary ***\r\n");
   cmd_tree_node_print(cm->root, 0);
}

/****** static function list ******/
static u16_t cmd_dump_literate_cb(struct command *c, char *buff, u16_t len)
{
    u16_t ret = 0;

    if(NULL==c || NULL==buff || 0==len)
        return 0;

    ret += snprintf(buff+ret, len-ret, "%s ", c->name);
    
    return ret;
}

static void cmd_tree_node_value_print(char ch, int level, char *fmt, ...)
{
    int i = 0; 
    for(i=0; i < level; i++)
        putchar(ch);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    return;
}

static void cmd_tree_node_print(struct command *pn, int level)
{
    if(NULL == pn)
        cmd_tree_node_value_print('\t', level, "~\r\n");
    else
    {
        cmd_tree_node_print(pn->l, level+1);
        cmd_tree_node_value_print('\t', level, "%s\r\n", pn->name);
        cmd_tree_node_print(pn->r, level+1);
    }
}

static struct command *cmd_node_find(struct command *pn, char *name)
{
    int ret = 0;

    while(NULL != pn)
    {
        ret = strcmp(name, pn->name);
        if(ret < 0)
            pn = pn->l;
        else if(ret > 0)
            pn = pn->r; 
        else
            break;
    }

    return pn;
}

static u16_t cmd_iterate(struct command *tree, char *buff, u16_t len, command_info_iterate_cb_t cb)
{
    u16_t ret = 0;

    if(NULL != tree)
    {
        ret += cmd_iterate(tree->l, buff+ret, len-ret, cb);
        ret += cb(tree, buff+ret, len-ret);
        ret += cmd_iterate(tree->r, buff+ret, len-ret, cb);
    }

    return ret;
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

/* reply message to PC host */
static int cmd_reply(const char *buff, struct sockaddr_in *cip, socklen_t clen)
{
    struct cmd_manager *cm = &cm_obj;

    if(NULL==buff || NULL==cip || 0==clen)
    {
        log_red("parameter error: msg=%s, cip=%p, clen=%d\n", buff, cip, clen);
        return -1;
    }
   
    return sendto(cm->sfd, buff, strlen(buff), 0, (struct sockaddr *)cip, clen );
}

/* reply fail message to PC host */
static int cmd_fail_reply(u16_t id, struct sockaddr_in *cip, socklen_t clen)
{
    if(id >= MAX_FAIL)
        return -1;

    cmd_reply(cmd_fail_list[id], cip, clen);

    return 0;
}

/* execute a command */
static int cmd_exe(struct command *cmd, int argc, char *argv[], struct sockaddr_in *cip, socklen_t clen)
{
    struct cmd_manager *cm = &cm_obj;
    int len = 0;

    /* parameter check */
    if(NULL==cmd || 0==argc || NULL==argv || NULL==cip || 0==clen)
    {
        log_red("parameter error: cmd=%p, argc=%d, argv=%p, cip=%p, clen=%d\n", 
                cmd, argc, argv, cip, clen);
        return -1;
    }

    /* command execution */
    if(cmd && cmd->func) 
    {
        len = cmd->func(argc, argv, cm->buff, sizeof(cm->buff), cmd->user);
        if(len > 0)
            cmd_reply(cm->buff, cip, clen);   /* send command execute log to PC host */
    }

    return 0;
}

/* hand command from PC host */
static int cmd_do_hand(char *buff, struct sockaddr_in *cip, socklen_t clen)
{
    int argc = 0;
    char *argv[CMD_MAX_ARGC];
    int i = 0;
    struct command *c = NULL;
    struct cmd_manager *cm = &cm_obj;

    if(NULL==buff || NULL==cip || 0==clen)
        return -1;

    /* command parse */
    argc = str_parse(buff, argv, ARY_SIZE(argv));
    if(argc <= 0)
    {
        cmd_fail_reply(CMD_PARSE_ERR, cip, clen);
        return -1;
    }

    plog(CMD, "command parse success!\n");
#if 0
    printf("argc list: %d\n", argc);
    for(i=0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");
#endif
    c = cmd_find(argv[0]);
    if(NULL == c)
    {
        cmd_fail_reply(CMD_NOT_FOUND, cip, clen);
        return -1;
    }

    plog(CMD, "command search success!\n");
     
    if(cmd_exe(c, argc, argv, cip, clen))
        return -1;

    return 0;
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
        buff[n] = '\0'; /* append tail character */
        cmd_do_hand(buff, paddr, *plen);
    }
    return NULL;
}

