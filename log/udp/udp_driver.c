#include "udp_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cmd/command.h"
#include <time.h>
#include "log/log.h"
#include "cmd/argtable3.h"
#include "util/util.h"
#include "util/blist.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "udp_device.h"
#include "argudp.h"


#define LOG_UDP_DRV_MSG_SIZE    2048
struct log_udp_driver {
    struct log_driver drv; 
    int send_fd;        /* socket fd */
    char msg[LOG_UDP_DRV_MSG_SIZE];
};


int log_udp_driver_emit(struct log_device *dev, struct log_rec *rec)
{
    struct log_udp_device *udev = list_entry(dev, struct log_udp_device, dev);
    struct log_udp_driver *udrv = list_entry(dev->drv, struct log_udp_driver, drv);
    char *msg = udrv->msg;
    int ret = 0;
    
//    log_grn_print("udp driver emit -> device(%s:%d)\n", inet_ntoa(udev->caddr.sin_addr),ntohs(udev->caddr.sin_port));
   
    if(LOG_DEVICE_FLAGS(dev, LOGDF_DEVID))
        ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, "<%s:%d>", dev->name, dev->id);
    if(LOG_DEVICE_FLAGS(dev, LOGDF_LEVEL))
        ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, "<%u>", rec->level);
    if(LOG_DEVICE_FLAGS(dev, LOGDF_CAT))
        ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, "%s,", log_get_cat_name(rec->cat));
    if(LOG_DEVICE_FLAGS(dev, LOGDF_FILE))
        ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, "%s:", rec->file);
    if(LOG_DEVICE_FLAGS(dev, LOGDF_LINE))
        ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, "%d-", rec->line);
    if(LOG_DEVICE_FLAGS(dev, LOGDF_FUNC))
        ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, "%s()", rec->func);
    ret += snprintf(msg+ret, LOG_UDP_DRV_MSG_SIZE-ret, " %s", rec->msg);
    
    printf("%s", msg);

    return sendto(udrv->send_fd, msg, strlen(msg), 0, (struct sockaddr *)&udev->caddr, udev->clen);
}

int log_udp_driver_probe(struct log_device *dev)
{
    struct log_udp_device *udev = list_entry(dev, struct log_udp_device, dev);
    
    cmd_client(&udev->caddr, &udev->clen);

    log_grn_print("udp driver probe device(%s:%d)\n", inet_ntoa(udev->caddr.sin_addr), ntohs(udev->caddr.sin_port));

    return 0;
}

void log_udp_driver_remove(struct log_device *dev)
{
    struct log_udp_device *udev = list_entry(dev, struct log_udp_device, dev);

    log_grn_print("udp driver remove device(%s:%d)\n", inet_ntoa(udev->caddr.sin_addr),ntohs(udev->caddr.sin_port));

    return;
}

static struct log_udp_driver driver_obj = 
{
    .drv = {
        .name = "udp",
        .emit = log_udp_driver_emit,
        .probe = log_udp_driver_probe,
        .remove = log_udp_driver_remove,
    },
};

void log_udp_drv_init(void)
{
    struct log_udp_driver *pobj = &driver_obj;

    log_driver_init(&pobj->drv);

    log_driver_register(&pobj->drv); 

    pobj->send_fd = cmd_srv_fd();
    
    return;
}

int logudp_cmd_drv_iterate_cb(struct log_device *dev, arg_dstr_t ds )
{
    struct log_udp_device *udev = list_entry(dev, struct log_udp_device, dev);
    char nameid[100];
    char ipport[100];

    snprintf(nameid, sizeof(nameid), "%s:%d", dev->name, dev->id);
    snprintf(ipport, sizeof(ipport), "%s:%d", inet_ntoa(udev->caddr.sin_addr),ntohs(udev->caddr.sin_port));

    arg_dstr_catf(ds, "\t%-10s %-20s %#-10x %p\r\n", nameid, ipport, dev->flags, dev->drv);

    return 0;
}

arg_dstr_t logudp_cmd_drv(arg_dstr_t ds)
{
    struct log_udp_driver *pobj = &driver_obj;
    struct log_driver *drv = &pobj->drv;

    arg_dstr_catf(ds, "/****** log udp driver summary ******/\r\n");
    arg_dstr_catf(ds, "name : %s\r\n", drv->name);
    arg_dstr_catf(ds, "device list:\r\n");
    arg_dstr_catf(ds, "\t%-10s %-20s %#-10s %-s\r\n", "nameid", "ipport", "flags", "drv");
    log_driver_dev_iterate(drv, logudp_cmd_drv_iterate_cb, ds);

    return ds;
}

arg_dstr_t logudp_drv_help(arg_dstr_t ds, const char *name)
{
    arg_dstr_catf(ds, "%s --drv\r\n", name);

    return ds;
}
