#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "poll.h"
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

/*
usage:
    ./a.out -l port
*/
static void srv_init(unsigned short port);

int main(int argc, char *argv[])
{
    int op;
    unsigned int port = 0;

    while(-1 != ( op = getopt(argc, argv, "l:")) )
    {
        switch(op)
        {
            case 'l':
                port = atoi(optarg); 
                break;

            defualt:
                break;
        }
    }

    if(0 == port)
        return -1;

    srv_init(port);


    return 0;
}

static struct client *client_find(struct client *start, int nu, int key)
{
    int i =0;

    for(i = 0; i < nu; i++)
    {
        if(start[i].use && start[i].fd == key)
            break;
    }

    if(i == nu)
        return NULL;
    return start+i;
}

#define SOCK_RET(msg, args...)   do{printf(msg, ##args); return;}while(0)
#define CLIENT_NU 10
static void srv_init(unsigned short port)
{
    int sfd = 0;    //server fd
    int ret = 0;
    struct sockaddr_in addr;
    struct sockaddr_in addr_cli;
    socklen_t addr_cli_len;
    nfds_t nfds = 0;
    struct pollfd poll_list[100];
    int new_n;
    int new_fd;
    int i = 0;
    struct client cli_list[CLIENT_NU];
    struct client *pc = NULL;
    char buff[1000];

    printf("listen port = %u\n", port);
    
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if(-1 == sfd)
        SOCK_RET("socket fail!\n");
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if(-1 == ret )
        SOCK_RET("bind fail!\n");

    ret = listen(sfd, 3);
    if(-1 == ret)
        SOCK_RET("listen fail!\n");

// init client list
    bzero(cli_list, sizeof(cli_list));

    nfds = 0;
    poll_list[nfds].fd = sfd;           // server
    poll_list[nfds].events = POLLIN;
    nfds++;
    poll_list[nfds].fd = fileno(stdin); // stdin
    poll_list[nfds].events = POLLIN;
    nfds++;

    while(-1 != (new_n = poll(poll_list, nfds, -1)) )
    {
        // literate all events in poll_list
        for(i=0; i < nfds && new_n > 0; i++)
        {
            // for server fd
            if(POLLIN & poll_list[i].revents)
            {
                if(sfd == poll_list[i].fd)
                {
                    bzero(&addr_cli, sizeof(addr_cli));
                    addr_cli_len = sizeof(addr_cli);    // must do for indicating addr_cli size
                    new_fd = accept(sfd, (struct sockaddr *)&addr_cli, &addr_cli_len);
                    if(-1 != new_fd)
                    {
                        printf("client ip = %s:%u\n", inet_ntoa(addr_cli.sin_addr), ntohs(addr_cli.sin_port));
                        for(i=0; i < CLIENT_NU; i++)
                        {
                            if(!cli_list[i].use)
                            {
                                cli_list[i].use = 1;
                                cli_list[i].fd = new_fd;
                                snprintf(cli_list[i].ip, sizeof(cli_list[i].ip), "%s", inet_ntoa(addr_cli.sin_addr));
                                cli_list[i].port = ntohs(addr_cli.sin_port);
                                printf("%s:%u client connected!\n", cli_list[i].ip, cli_list[i].port); 
                                break;
                            }
                        }
                        if(i == CLIENT_NU)
                            close(new_fd);
                    }
                }
                else if(fileno(stdin) == poll_list[i].fd)
                {
                    bzero(buff, sizeof(buff));
                    fgets(buff, sizeof(buff), stdin);
                    if(0 == strncmp("list", buff, 4))
                    {
                        for(i=0; i < CLIENT_NU; i++)
                        {
                            printf("%u.", i);
                            if(cli_list[i].use)
                            {
                                printf("\t%s:%u\n", cli_list[i].ip, cli_list[i].port);
                            }
                            else
                                printf("\tonuse\n");
                        }
                    }
                    else if(0 == strncmp("close", buff, 5))
                    {
                        for(i=0; i < CLIENT_NU; i++)
                        {
                            if(cli_list[i].use)
                            {
                                printf("%s:%u client close!\n", cli_list[i].ip, cli_list[i].port); 
                                close(cli_list[i].fd);
                                cli_list[i].use = 0;
                            }
                        }
                    }
                    else if(0 == strncmp("quit", buff, 4))
                    {
                        for(i=0; i < CLIENT_NU; i++)
                        {
                            if(cli_list[i].use)
                            {
                                printf("%s:%u client close!\n", cli_list[i].ip, cli_list[i].port); 
                                close(cli_list[i].fd);
                                cli_list[i].use = 0;
                            }
                        }
                        goto go_quit;
                    }
                }
                else
                {
                    pc = client_find(cli_list, CLIENT_NU, poll_list[i].fd);
                    if(pc)
                    {
                        bzero(buff, sizeof(buff));
                        ret = read(poll_list[i].fd, buff, sizeof(buff));
                        if(ret <= 0)
                        {
                            printf("%s:%u client close!\n", pc->ip, pc->port); 
                            close(pc->fd);
                            pc->use = 0;
                        }
                        else if(ret > 0)
                            printf("%s:%u client msg: %s\n", pc->ip, pc->port, buff);
                    }
                }
                new_n--;
            }
        }
#if 0
        //for debug
        if(new_n == 0)
            printf("all events is handled!\r\n");
#endif
// setup new poll list
        nfds = 0;
        poll_list[nfds].fd = sfd;           // server
        poll_list[nfds].events = POLLIN;
        nfds++;
        poll_list[nfds].fd = fileno(stdin); // stdin
        poll_list[nfds].events = POLLIN;
        nfds++;
        for(i=0; i < CLIENT_NU; i++)              // clients
        {
            if(cli_list[i].use)
            {
                poll_list[nfds].fd = cli_list[i].fd;
                poll_list[nfds].events = POLLIN;
                nfds++;
            }
        }
    }
go_quit:
    printf("server port: %u closed!\n", port);
    close(sfd);

    return;
}
