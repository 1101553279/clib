#include "udp_device.h"
#include "log/log.h"
#include "argudp.h"
#include "cmd/command.h"
#include <stdlib.h>

struct log_udp_device *log_udp_device_new(void)
{
    struct log_udp_device *dev = NULL;

    dev = (struct log_udp_device *)malloc(sizeof(struct log_udp_device));
    if(NULL == dev)
        return NULL;

    dev->dev.name = "udp";

    return dev;
}

void log_udp_device_init(struct log_udp_device *dev)
{
    dev->dev.name = "udp";

    cmd_client(&dev->caddr, &dev->clen);

    return;
}

struct log_udp_device *log_udp_device_new_and_init(void)
{
    struct log_udp_device *dev = NULL;

    dev = log_udp_device_new();
    if(NULL != dev)
        log_udp_device_init(dev);

    return dev;
}

void log_udp_device_del(struct log_udp_device *dev)
{
    memset(dev, 0, sizeof(struct log_udp_device));
    free(dev);
}

arg_dstr_t logudp_cmd_dev_create(arg_dstr_t ds)
{
    struct log_udp_device *dev = NULL;
    int ret = 0;

    dev = log_udp_device_new_and_init();
    if(NULL == dev)
    {
        arg_dstr_catf(ds, "log udp device new and init fail!\n");
        goto err_new;
    }

    ret = log_device_register(&dev->dev);
    if(0 != ret)
    {
        arg_dstr_catf(ds, "log udp device register fail!\n");
        goto err_reg;
    }

    arg_dstr_catf(ds, "log udp device create && register success!\n");

    return ds;
err_reg:
err_new:
    return ds;
}

arg_dstr_t logudp_cmd_dev_delete(arg_dstr_t ds, int id)
{
    struct log_device *dev = NULL;
    struct log_udp_device *udev = NULL;

    dev = log_device_find_by_nameid("udp", id);
    if(NULL == dev)
    {
        arg_dstr_catf(ds, "device(udp:%d) is not found!\r\n", id);
        return ds;
    }

    log_device_unregister(dev);

    udev = list_entry(dev, struct log_udp_device, dev);
    arg_dstr_catf(ds, "device(udp:%d), ipport=%s:%d delete successful!\r\n", id, \
            inet_ntoa(udev->caddr.sin_addr), ntohs(udev->caddr.sin_port));

    free(udev);

    return ds;
}

int logudp_dev_del_cb(struct log_device *dev, arg_dstr_t ds)
{
    struct log_udp_device *udev = list_entry(dev, struct log_udp_device, dev);

    log_device_unregister(dev);

    arg_dstr_catf(ds, "delete device(%s:%d), ip:port=%s:%d\r\n", \
            dev->name, dev->id, inet_ntoa(udev->caddr.sin_addr),ntohs(udev->caddr.sin_port));

    free(udev);

    return 0;
}

/************************************************************
 logudp --dev [-i id]                               # output device information
 logudp --dev -c|--create [--srv ip --port port]    # create
 logudp --dev -d|--del -i id                        # delete
 logudp --dev -s|--set -i id                        # set
 logudp --dev -r|--reset -i id                      # reset
 logudp --dev -a                # all devices
*************************************************************/
arg_dstr_t logudp_cmd_dev(arg_dstr_t ds)
{
    int i = 0;

    /* logudp --dev -c|--create */
    if(argudp_arg_c->count > 0)
    {
        ds = logudp_cmd_dev_create(ds);
        goto to_create;
    }

    if(argudp_arg_d->count > 0)
    {
        /* logudp --dev -d|--del -a */
        if(argudp_arg_a->count > 0)
        {
            arg_dstr_catf(ds, "logudp: delete all udp devices\r\n");
            log_device_dstr_handle_by_name("udp", logudp_dev_del_cb, ds);
        }
        else/* logudp --dev -d|--del -i id1 -i -id2..... */
        {
            if(0 == argudp_arg_i->count)
                arg_dstr_catf(ds, "logudp : must supply a id for deleting device\r\n");
            else
            {
                for(i=0; i < argudp_arg_i->count; i++)
                    logudp_cmd_dev_delete(ds, argudp_arg_i->ival[i]);
            }
        }
        goto to_delete;
    }
    if(argudp_arg_s->count > 0)
    {
    }

to_delete:
to_create:
    return ds;
}

arg_dstr_t logudp_dev_help(arg_dstr_t ds, const char *name)
{
    arg_dstr_catf(ds, "%s --dev [-i id]\r\n", name);
    arg_dstr_catf(ds, "%s --dev -c|--create [--srv ip] [--port port]\r\n", name);
    arg_dstr_catf(ds, "%s --dev -d|--del [-i id]\r\n", name);
    arg_dstr_catf(ds, "%s --dev -s|--set [-i id]\r\n", name);
    arg_dstr_catf(ds, "%s --dev -r|--reset [-i id]\r\n", name);
}
