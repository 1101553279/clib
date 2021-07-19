#ifndef __SUTIL_H__
#define __SUTIL_H__
#include "btype.h"
#include <unistd.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "log.h"

#define SOCK_IP_LEN     20
#define SOCK_TIME_LEN   40

#define SOCK_TCP        0x1
#define SOCK_UDP        0x2

#define SOCK_LOG(fmt, args...)      do{ log("%s:%d # "fmt, __func__, __LINE__, ##args); }while(0)
#define SOCK_ERR(fmt, args...)      do{ logc(COLOR_FONT_RED, fmt, ##args); } while(0) 
#define SOCK_OK(fmt, args...)       do{ logc(COLOR_FONT_GREN, fmt, ##args); }while(0)
#define SOCK_INFO(fmt, args...)     do{ logc(COLOR_FONT_YELL, fmt, ##args); }while(0)
#define SOCK_TITLE(fmt, args...)    do{ logc(COLOR_FONT_YELL, fmt, ##args);}while(0)
#define SOCK_PRINT(fmt, args...)    do{ log(fmt, ##args); }while(0)

#define SOCK_SPLIT                  "--------------------------------------------------"\
                                    "----------------------------------------------------\r\n"

int sock_fd_init(int type, u16_t port);
#endif
