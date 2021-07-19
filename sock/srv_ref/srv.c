#include "srv.h"
#include "log.h"
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
static int srv_add_client(struct srv *srv, int cfd, struct sockaddr_in *addr, socklen_t len);


extern char *opt_ip;
/* init a server: port */
int srv_init(struct srv *srv, unsigned short port)
{
    int ret = 0;
    struct sockaddr_in addr;
    time_t now;

    /* init client list head, must do */
    INIT_LIST_HEAD(&srv->clist);

    /* setup a tcp server */
    srv->sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if(-1 == srv->sfd)
    {
        log_red("socket new fail!\n");
        return -1;
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
#if 0   /* for debug */
    if(NULL != opt_ip)
        log_grn("ip:port = %s:%u\r\n", opt_ip, port);
#endif
    if(NULL == opt_ip || (0 != inet_aton(opt_ip, &(addr.sin_addr))))
        addr.sin_addr.s_addr = INADDR_ANY;

    if(NULL != opt_ip)
        free(opt_ip);

    ret = bind(srv->sfd, (struct sockaddr *)&addr, sizeof(addr));
    if(-1 == ret)
    {
        log_red("bind fail!\n");
        return -1;
    }

    ret = listen(srv->sfd, 3);
    if(-1 == ret)
    {
        log_red("listen fail!\n");
        return -1;
    }
    log_grn("server %u init success!\r\n", port);
    
    time(&now);
    strcpy(srv->stime, asctime(localtime(&now)));

    return 0;
}

int srv_get_sfd(struct srv *srv)
{
    return srv->sfd;
}
/* add client to list */
static int srv_add_client(struct srv *srv, int cfd, struct sockaddr_in *addr, socklen_t len)
{
    time_t now;
    struct client *pc = calloc(1, sizeof(struct client));   /* alloc client space */
    int flags;

    if(NULL == pc)
    {
        log_red("%s: malloc error!\r\n", __func__);
        return -1;
    }
    /* record fd */
    pc->fd = cfd;
    /* record ip */
    strcpy(pc->ip, inet_ntoa(addr->sin_addr));
    /* record port */
    pc->port = ntohs(addr->sin_port);
    /* record connect time */
    time(&now);
    strcpy(pc->ctime, asctime(localtime(&now)));

    log_whi("add client: %d, %s, %u, %s\n", pc->fd, pc->ip, pc->port, pc->ctime);
#if 0 
    /* get socket flag */
    if(-1 == (flags=fcntl(cfd, F_GETFL, 0)))
        log_red("socket fcntl get fail!\n");
    /* set socket flag */
    if(-1 == (fcntl(cfd, F_SETFL, flags|O_NONBLOCK)))
        log_red("socket fcntl set 'NONBLOCK' fail!\n");
#endif
    /* add client to list's tail */ 
    list_add_tail(&pc->head, &srv->clist);

    return 0;
}


/* remove client from list */
int srv_rmv_client(struct srv *srv, int cfd)
{
    struct list_head *pos;
    struct list_head *n;
    struct client *pc;  /* pointer client */

    list_for_each_safe(pos, n, &srv->clist)
    {
        pc = list_entry(pos, struct client, head);
        if(cfd == pc->fd)
        {
            list_del(&pc->head);
            log_whi("del client: %d, %s, %u, %s\n", pc->fd, pc->ip, pc->port, pc->ctime);
            close(pc->fd);  /* close socket */
            free(pc);   /* free client space */
            return 0;
        }
    }

    log_red("%d client is not found!\r\n", cfd);

    return -1;
}
int srv_nfds_cnt(struct srv *srv)
{       /*server + clients */
    return 1 + srv_client_cnt(srv);
}

/* get client number */
int srv_client_cnt(struct srv *srv)
{
    int cnt = 0;
    struct list_head *pos;

    list_for_each(pos, &srv->clist)
        cnt++;

    return cnt;
}

/* add all fds that is need to watch */
int srv_nfds_add(struct srv *srv, struct pollfd *fds, nfds_t nfds)
{
    int cnt = 0;
    struct list_head *pos;
    struct client *pc;

    /* add server for watch */
    if(cnt <= nfds)
    {
        fds[cnt].fd = srv->sfd;
        fds[cnt].events = POLLIN;
        cnt++;
    }

    /* add clients for watch */
    list_for_each(pos, &srv->clist)
    {
        pc = list_entry(pos, struct client, head);
        if(cnt <= nfds)
        {
            fds[cnt].fd = pc->fd;   
            fds[cnt].events = POLLIN;
            cnt++;
        }
        else
            break;
    }
     

    return cnt;
}

void server_do(struct srv *srv)
{
    int cli_fd;
    struct sockaddr_in cli_add;
    socklen_t cli_len;

//    log("server do !\r\n");
    bzero(&cli_add, sizeof(cli_add));
    cli_len = sizeof(cli_add);  /* must init */
    cli_fd = accept(srv_get_sfd(srv), (struct sockaddr *)&cli_add, &cli_len);
    if(srv_add_client(srv, cli_fd, &cli_add, cli_len) < 0)
        log_red("client add fail!\r\n");

    return;
}
static void hex_dump(unsigned char *buff, unsigned short len)
{
    unsigned short i = 0;
    for(i=0; i < len; i++)
        printf("%x ", buff[i]);
    printf("\n");
    return;
}
void server_client_do(struct srv *srv, int fd)
{
    struct list_head *pos;
    struct client *pc = NULL;
    unsigned char buff[100];
    ssize_t ret;

    /* add clients for watch */
    list_for_each(pos, &srv->clist)
    {
        pc = list_entry(pos, struct client, head);
        if(fd == pc->fd)
            break;
    }
    if(pc)
    {
        bzero(buff, sizeof(buff));
        ret = read(fd, buff, sizeof(buff));
        if(ret > 0)
        {
            log_yel("client(%s:%u): rx: %lu: ", pc->ip, pc->port, ret);
            hex_dump(buff, ret);

            bzero(buff, sizeof(buff));
        }
        else
            srv_rmv_client(srv, fd);
    }
     

    return;
}


/* uninit server includes all clients */
void srv_uninit(struct srv *srv)
{
    struct list_head *pos;
    struct list_head *n;
    struct client *pc;  /* pointer client */

    list_for_each_safe(pos, n, &srv->clist)
    {
        pc = list_entry(pos, struct client, head);
        list_del(&pc->head);
        log_yel("close client: %d, %s, %u, %s\r\n", pc->fd, pc->ip, pc->port, pc->ctime);
        close(pc->fd);
        free(pc);   /* free client space */
    }
    log_yel("close server!\r\n");
    close(srv->sfd);    /* cose socket of server */
    log("uninit server succes!\r\n");
    
    return;
}

/* dump all server information include clients */
void srv_dump(struct srv *srv, const char *mode)
{
    struct list_head *pos;
    struct client *pc;
    int no = 0;
    int color = 0;

    log("/****** srv summary ******/\r\n");
    log_yel("server fd: %d, setup time: %s", srv->sfd, srv->stime);
    if(0 == strcasecmp("verbose", mode))
        log("%-3s %-5s %-20s %-10s %-s\r\n", "no", "fd", "ip", "port", "connect time");
    else
        log("%-3s %-20s %-10s\r\n", "no", "ip", "port");
    list_for_each(pos, &srv->clist)
    {
        pc = list_entry(pos, struct client, head);
        if(0 == strcasecmp("verbose", mode))
            if(0 == (no%2))
                logc(LOG_YEL, "%-3d %-5d %-20s %-10u %-s\r\n", ++no, pc->fd, pc->ip, pc->port, pc->ctime);
            else
                logc(LOG_GRN, "%-3d %-5d %-20s %-10u %-s\r\n", ++no, pc->fd, pc->ip, pc->port, pc->ctime);
        else
            if(0 == (no%2))
                logc(LOG_YEL, "%-3d %-20s %-10u\r\n", ++no, pc->ip, pc->port);
            else
                logc(LOG_GRN, "%-3d %-20s %-10u\r\n", ++no, pc->ip, pc->port);
    }

    return;
}

