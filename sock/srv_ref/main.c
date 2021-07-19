#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "srv.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sig.h"
#include <string.h>
#include <strings.h>


#define COMMAND_PROMPT  "please input a command: "
static void do_handle(struct srv *srv);

/*typedef void(*sig_cb_t)(int num, siginfo_t *info, void *context);*/
void catch_sig_cb(int num, siginfo_t *info, void *ctxt)
{
    log("catch # %s # signal!\r\n", strsignal(num));
    exit(0);

    return;
}

void usage_print(const char *name)
{
    printf( "usage: %s\r\n"
            "%s -p [ipaddress] -l <port> -d [verbose|simple] -s <yes|no>\r\n", 
            name, name);
    return;
}

/* p::  -p  ip address      option argument */
/* l:   -l  listen port     must has a argument */
/* d::  -d  display mode    option argument */
/* s:   -s  signal install  must has a argument */
/* the first colon ':'  means that will return ':' when a option missing argument */
/* notice: must is this format -p192.168.68.3 if option element is option arguement */
/*         this format -p 192.168.68.3 is not ok! */
const char *optstring = "+:p::l:d::s:";
char *opt_ip = NULL;
char *opt_mode = SRV_SIMPLE;          /* log verbose or simple */
int main(int argc, char *argv[])
{
    struct srv srv;  /* server hand */
    int ret = 0;

    unsigned int opt_port = 3000;   /* listen port */
    unsigned char opt_sig = 0;      /* whether signal handler register */
    int opt_err = 0;                /* command parser result */
    
    if(argc < 2)
    {
        usage_print(argv[0]);
        return -1;
    }

    opterr = 1;     /* prevent the error message when function(getopt) parsing */

    while(-1 != (ret=getopt(argc, argv, optstring)))
    {
        switch(ret)
        {
            case 'p':
                if(NULL != optarg)
                {
                    opt_ip = strdup(optarg);
                    printf("optarg = %s, opt_ip = %s\r\n", optarg, opt_ip);
                }
                break;
            case 'l':
                opt_port = atoi(optarg);
                break;
            case 'd':
                if(NULL != optarg && 0 == strcasecmp("verbose", optarg))
                    opt_mode = SRV_VERBOSE;
                else if(NULL != optarg && 0 != strcasecmp("simple", optarg))
                {
                    printf("-d \'argument\' must be \"verbose\" or \"simple\"\r\n");
                    opt_err = 1;
                }
                break; 
            case 's':
                opt_sig = (0==strcasecmp("yes", optarg))? 1: 0;
                break;
            case ':':   /* missing option argument */
                printf("\'%c\' must has a argument\r\n", optopt);
                opt_err = 1;
                break;
            case '?':   /* unrecognized option element */
                printf("\'%c\' no this option\r\n", optopt);
                opt_err = 1;
                break;
            default:
                break;
        }
    }
    if(1 == opt_err)
        return -1;


    /* catch CTRL+C signal */
    if(1 == opt_sig)
        sig_reg(SIGINT, catch_sig_cb);

    /* init a server */
    ret = srv_init(&srv, opt_port);
    if(ret < 0)
    {
        log_red("server init error!\r\n");
        return -1;
    }

    logc(LOG_RED, COMMAND_PROMPT);

    while(1)
        do_handle(&srv);

    return 0;
}

static int watch_fill(struct srv *srv, struct pollfd *pf, int nfds)
{
    int cnt = 0;

    /* add watch for stdin */
    if(cnt <= nfds)
    {
        pf[cnt].fd = fileno(stdin);
        pf[cnt].events = POLLIN;
    }
    cnt++;

    /* add watch for server + clients */
    cnt += srv_nfds_add(srv, pf+cnt, nfds-cnt);

    return cnt;
}

void stdin_do(struct srv *srv)
{
    char buff[100];

    fgets(buff, sizeof(buff), stdin);
//    log("stdin : %s, len = %lu\r\n", buff, strlen(buff));
    if(0 == strncmp("dump", buff, 4))
        srv_dump(srv, opt_mode);


    return;
}

static void watch_do(struct srv *srv, struct pollfd *pf, int nfds, int cnt)
{
    int i = 0;
    
//    log("nfds = %d, cnt = %d\n", nfds, cnt);
    for(i=0; (i<nfds) && cnt>0; i++)
    {
        if(pf[i].revents && POLLIN)
        {
            cnt--;
            if(fileno(stdin) == pf[i].fd)
            {
                stdin_do(srv);
                logc(LOG_RED, COMMAND_PROMPT);
            }
            else if(pf[i].fd == srv_get_sfd(srv))
                server_do(srv);
            else
                server_client_do(srv, pf[i].fd);
        }
    }

    return;
}

static void do_handle(struct srv *srv)
{
    struct pollfd *pf = NULL;
    int nfds = 0;
    int cnt = 0;
    int ret;

    nfds += 1;  /* stdin */
    nfds += srv_nfds_cnt(srv);    /* server + all clients */
    pf = (struct pollfd *)calloc(nfds, sizeof(struct pollfd));
    if(NULL == pf)
    {
        log_red("%s calloc fail!\r\n", __func__);
        return;
    }

    /* add for watching */
    nfds = watch_fill(srv, pf, nfds);
    
    cnt = poll(pf, nfds, -1);
    
    watch_do(srv, pf, nfds, cnt);
    
    if(pf)
        free(pf);

    return;
}
