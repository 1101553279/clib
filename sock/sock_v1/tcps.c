#include "tcps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "command.h"

static int tcps_command(int argc, char *argv[], char *buff, int len, void *user)
{

    return 0;
}
int tcps_init(struct sock_tcps *t, u16_t port)
{
    time_t c;

    t->root = NULL;
    t->fd = sock_fd_init(SOCK_TCP, port);
    if(-1 == t->fd)
        return -1;
    t->port = port;
    /* init time */
    time(&c);
    strftime(t->itime, sizeof(t->itime), "%H:%M:%S", localtime(&c));

    cmd_add("tcps", "list tcps information", CMD_INDENT"tcps [fd|port|clients]\r\n", tcps_command, t);

    return 0;
}

int tcps_client_add(struct sock_tcps *t, int fd, struct sockaddr_in *addr)
{
    struct sclient **ppn = &(t->root);
    
    while(NULL != *ppn)
    {
        if(fd < (*ppn)->fd)
            ppn = &((*ppn)->l);
        else if((*ppn)->fd < fd)
            ppn = &((*ppn)->r);
        else
            break;
    }
    *ppn = sclient_new(fd, addr);

    return 0;
}

int tcps_client_del(struct sock_tcps *t, int fd)
{
    struct sclient *pn = NULL;
    struct sclient **ppn = &t->root;

    while(NULL != *ppn)
    {
        if(fd < (*ppn)->fd)
            ppn = &((*ppn)->l);
        else if((*ppn)->fd < fd)
            ppn = &((*ppn)->r);
        else
            break;
    }
    if(NULL == *ppn)
        return -1;
    
    pn = *ppn;
    if(NULL!=pn->l &&  NULL!=pn->r)
    {
        ppn = &(pn->l);
        while(NULL != (*ppn)->r)
            ppn = &((*ppn)->r);

        sclient_copy(pn, *ppn);
        pn = *ppn;
    }
    *ppn = (NULL==pn->l)? pn->r: pn->l;

    /* for debug */
    printf("del client: ");
    sclient_line_print(pn);
    close(pn->fd);
    free(pn);

    return 0;
}

struct sclient *tcps_client_find(struct sock_tcps *t, int fd)
{
    struct sclient *pn = t->root;

    while(NULL != pn)
    {
        if(fd < pn->fd)
            pn = pn->l;
        else if(pn->fd < fd)
            pn = pn->r;
        else
            break;
    }

    return pn;
}

int tcps_fds_in(struct sock_tcps *t, int fd)
{
    return fd==t->fd || NULL!=tcps_client_find(t, fd);
}

int tcps_client_hand(struct sock_tcps *t, struct pollfd *fds)
{
    ssize_t len = 0;
    u8_t buff[100];
    struct sclient *pn;

    bzero(buff, sizeof(buff));
    len = read(fds->fd, buff, sizeof(buff)-1);
    if(len <= 0)
        tcps_client_del(t, fds->fd);
    else
    {
        pn = tcps_client_find(t, fds->fd);
        if(NULL != pn)
            sclient_msg_print(pn, buff, len);
    }

    return 0;
}

int tcps_fds_hand(struct sock_tcps *t, struct pollfd *fds)
{
    nfds_t nfds = 0;
    int cfd;
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
    bzero(&caddr, sizeof(caddr));
    
    if(POLLIN & fds->revents)
    {
        if(fds->fd == t->fd) /* server event */
        {
            cfd = accept(t->fd, (struct sockaddr *)&caddr, &clen);
            tcps_client_add(t, cfd, &caddr);
            /* for debug */
            tcps_tree_print(t);
            nfds++;
        }
        else
        {
            tcps_client_hand(t, fds);
            nfds++;
        }
    }

    return nfds;
}
int tcps_client_size(struct sclient *cur)
{
    int n = 0;
    struct sclient **ppn;

    while(NULL != cur)
    {
        if(NULL == cur->l)
        {
            n++; 
            cur = cur->r;
        }
        else
        {
            ppn = &(cur->l);
            while(NULL != (*ppn)->r && cur!=(*ppn)->r)
                ppn = &(*ppn)->r;
            if(NULL == (*ppn)->r)
            {
                (*ppn)->r = cur;
                cur = cur->l;
            }
            else
            {
                (*ppn)->r = NULL;
                n++;
                cur = cur->r;
            }
        }
    }

    return n;
}

nfds_t tcps_fds_size(struct sock_tcps *t)
{
    /* server + clients */
    return 1 + tcps_client_size(t->root);
}
nfds_t tcps_fds(struct sock_tcps *t, struct pollfd *fds, nfds_t size)
{
    nfds_t n = 0;
    struct sclient *cur = t->root;;
    struct sclient **ppn;

    /* tcp server */
    if(n < size--)
    {
        fds[n].fd = t->fd;
        fds[n].events = POLLIN;
        n++;
    }
    while(NULL != cur)
    {
        if(NULL == cur->l)
        {
            if(n < size--)
            {
                fds[n].fd = cur->fd;
                fds[n].events = POLLIN;
                n++;
                cur = cur->r;
            }
        }
        else
        {
            ppn = &(cur->l);
            while(NULL!=(*ppn)->r && cur!=(*ppn)->r)
                ppn = &(*ppn)->r;
            if(NULL == (*ppn)->r)
            {
                (*ppn)->r = cur;
                cur = cur->l;
            }
            else
            {
                (*ppn)->r = NULL;
                if(n < size--)
                {
                    fds[n].fd = cur->fd;
                    fds[n].events = POLLIN;
                    n++;
                }
                cur = cur->r;
            }
        }
    }

    return n;
}
void tcps_uninit(struct sock_tcps *t)
{


    return;
}
void tcps_node_print(struct sclient *c)
{
    if(NULL != c)
    {
        tcps_node_print(c->l);
        SOCK_PRINT("------------------------------------------------"
                   "-----------------------------------------\r\n");
        sclient_line_print(c);
        tcps_node_print(c->r);
    }
    return;
}
void tcps_tree_print(struct sock_tcps *t)
{
    SOCK_TITLE("****** tcps summary ******\r\n");
    SOCK_PRINT("srv fd    : %d\r\n", t->fd); 
    SOCK_PRINT("srv port  : %u\r\n", t->port);
    SOCK_PRINT("srv online: %s\r\n", t->itime);
    SOCK_PRINT("------------------------------------------------"
               "-----------------------------------------\r\n");
    SOCK_PRINT("| %-15s | %-5s | %-15s | %-5s | %-10s | %-15s |\r\n", 
                "left", "fd", "ip", "port", "link time", "right");
    tcps_node_print(t->root); 
    SOCK_PRINT("------------------------------------------------"
               "-----------------------------------------\r\n");

    return;
}

u16_t tcps_dump(struct sock_tcps *t, char *buff, u16_t len)
{
    u16_t ret = 0;

    ret += snprintf(buff+ret, len-ret, "****** tcps summary ******\r\n");
    ret += snprintf(buff+ret, len-ret, "srv fd    : %d\r\n", t->fd); 
    ret += snprintf(buff+ret, len-ret, "srv port  : %u\r\n", t->port);
    ret += snprintf(buff+ret, len-ret, "srv online: %s\r\n", t->itime);
    ret += snprintf(buff+ret, len-ret, "------------------------------------------------"
                                       "-----------------------------------------\r\n");
    ret += snprintf(buff+ret, len-ret, "| %-15s | %-5s | %-15s | %-5s | %-10s | %-15s |\r\n", 
                                       "left", "fd", "ip", "port", "link time", "right");
//    tcps_node_print(t->root); 
    ret += snprintf(buff+ret, len-ret, "------------------------------------------------"
                                       "-----------------------------------------\r\n");
    
    return ret;
}
