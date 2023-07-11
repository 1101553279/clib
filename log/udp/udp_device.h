#ifndef __LOG_UDP_DEVICE_H__
#define __LOG_UDP_DEVICE_H__

#include "log/log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



struct log_udp_device {
    struct log_device dev;
    struct sockaddr_in caddr;
    socklen_t clen;
};

void log_udp_device_init(struct log_udp_device *dev);

struct log_udp_device *log_udp_device_new(void);

void log_udp_device_del(struct log_udp_device *dev);

arg_dstr_t logudp_cmd_dev(arg_dstr_t ds);

arg_dstr_t logudp_dev_help(arg_dstr_t ds, const char *name);

#endif
