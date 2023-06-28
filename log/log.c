// SPDX-License-Identifier: GPL-2.0+
/*
 * Logging support
 *
 * Copyright (c) 2017 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include "errno.h"
#include <pthread.h>
#include "udp/udp.h"
#include "cmd/argtable3.h"

static const char *const log_cat_name[] = {
	"none",
	"arch",
	"board",
	"core",
	"driver-model",
	"device-tree",
	"efi",
	"alloc",
	"sandbox",
	"bloblist",
	"devres",
	"acpi",
	"boot",
	"event",
	"fs",
};
#if 0
_Static_assert(ARRAY_SIZE(log_cat_name) == LOGC_COUNT - LOGC_NONE,
	       "log_cat_name size");
#endif

static const char *const log_level_name[] = {
	"EMERG",
	"ALERT",
	"CRIT",
	"ERR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG",
	"CONTENT",
	"IO",
};
#if 0
_Static_assert(ARRAY_SIZE(log_level_name) == LOGL_COUNT, "log_level_name size");
#endif

static LOCK_DECLARE(log_device_lock);
static LIST_HEAD(log_device_head);

static LOCK_DECLARE(log_driver_lock);
static LIST_HEAD(log_driver_head);


#define CONFIG_LOG_DEFAULT_LEVEL    LOGL_DEBUG
enum log_level_t gd_default_log_level = CONFIG_LOG_DEFAULT_LEVEL;

#define CONFIG_SYS_CBSIZE   1024


const char *strchrnul(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			break;
	return s;
}

/* All error responses MUST begin with '<' */
const char *log_get_cat_name(enum log_category_t cat)
{
	const char *name = NULL;

	if (cat < 0 || cat >= LOGC_COUNT)
		return "<invalid>";
	if (cat >= LOGC_NONE)
		return log_cat_name[cat - LOGC_NONE];

#if 0
#if CONFIG_IS_ENABLED(DM)
	name = uclass_get_name((enum uclass_id)cat);
#else
	name = NULL;
#endif
#endif

	return name ? name : "<missing>";
}

#if 0
enum log_category_t log_get_cat_by_name(const char *name)
{
	enum uclass_id id;
	int i;

	for (i = LOGC_NONE; i < LOGC_COUNT; i++)
		if (!strcmp(name, log_cat_name[i - LOGC_NONE]))
			return i;
	id = uclass_get_by_name(name);
	if (id != UCLASS_INVALID)
		return (enum log_category_t)id;

	return LOGC_NONE;
}
#endif

const char *log_get_level_name(enum log_level_t level)
{
	if (level >= LOGL_COUNT)
		return "INVALID";
	return log_level_name[level];
}

enum log_level_t log_get_level_by_name(const char *name)
{
	int i;

	for (i = 0; i < LOGL_COUNT; i++) {
		if (!strcasecmp(log_level_name[i], name))
			return i;
	}

	return LOGL_NONE;
}

struct log_device *log_device_find_by_name(const char *drv_name)
{
	struct log_device *ldev;

	list_for_each_entry(ldev, &log_device_head, sibling_node) {
		if (!strcmp(drv_name, ldev->drv->name))
			return ldev;
	}

	return NULL;
}

bool log_has_cat(enum log_category_t cat_list[], enum log_category_t cat)
{
	int i;

	for (i = 0; i < LOGF_MAX_CATEGORIES && cat_list[i] != LOGC_END; i++) {
		if (cat_list[i] == cat)
			return 1;
	}

	return 0;
}

bool log_has_file(const char *file_list, const char *file)
{
	int file_len = strlen(file);
	const char *s, *p;
	int substr_len;

	for (s = file_list; *s; s = p + (*p != '\0')) {
		p = strchrnul(s, ',');
		substr_len = p - s;
		if (file_len >= substr_len &&
		    !strncmp(file + file_len - substr_len, s, substr_len))
			return 1;
	}

	return 0;
}

/**
 * log_passes_filters() - check if a log record passes the filters for a device
 *
 * @ldev: Log device to check
 * @rec: Log record to check
 * Return: true if @rec is not blocked by the filters in @ldev, false if it is
 */
static bool log_passes_filters(struct log_device *ldev, struct log_rec *rec)
{
	struct log_filter *filt;

	if (rec->flags & LOGRECF_FORCE_DEBUG)
		return 1;

	/* If there are no filters, filter on the default log level */
	if (list_empty(&ldev->filter_head)) {
		if (rec->level > gd_default_log_level)
			return 0;
		return 1;
	}

	list_for_each_entry(filt, &ldev->filter_head, sibling_node) {
		if (filt->flags & LOGFF_LEVEL_MIN) {
			if (rec->level < filt->level)
				continue;
		} else if (rec->level > filt->level) {
			continue;
		}

		if ((filt->flags & LOGFF_HAS_CAT) &&
		    !log_has_cat(filt->cat_list, rec->cat))
			continue;

		if (filt->file_list &&
		    !log_has_file(filt->file_list, rec->file))
			continue;

		if (filt->flags & LOGFF_DENY)
			return 0;
		else
			return 1;
	}

	return 0;
}
static int gd_log_cont;
/**
 * log_dispatch() - Send a log record to all log devices for processing
 *
 * The log record is sent to each log device in turn, skipping those which have
 * filters which block the record.
 *
 * All log messages created while processing log record @rec are ignored.
 *
 * @rec:	log record to dispatch
 * Return:	0 msg sent, 1 msg not sent while already dispatching another msg
 */
static int log_dispatch(struct log_rec *rec, const char *fmt, va_list args)
{
	struct log_device *ldev;
	char buf[CONFIG_SYS_CBSIZE];
    static int processing_msg = 0;

	/*
	 * When a log driver writes messages (e.g. via the network stack) this
	 * may result in further generated messages. We cannot process them here
	 * as this might result in infinite recursion.
	 */
	if (processing_msg)
		return 1;

	/* Emit message */
	processing_msg = 1;
	list_for_each_entry(ldev, &log_device_head, sibling_node) {
		if ((ldev->flags & LOGDF_ENABLE) &&
		    log_passes_filters(ldev, rec)) {
			if (!rec->msg) {
				int len;

				len = vsnprintf(buf, sizeof(buf), fmt, args);
				rec->msg = buf;
				gd_log_cont = len && buf[len - 1] != '\n';
			}
			ldev->drv->emit(ldev, rec);
		}
	}
	processing_msg = 0;
	return 0;
}

static int gd_log_fmt = LOGF_ALL;
static int gd_logc_prev = LOGC_NONE;
static int gd_logl_prev = LOGL_INFO;
static int gd_log_drop_count = 0;

int _log(enum log_category_t cat, enum log_level_t level, const char *file,
	 int line, const char *func, const char *fmt, ...)
{
	struct log_rec rec;
	va_list args;


	/* Check for message continuation */
	if (cat == LOGC_CONT)
		cat = gd_logc_prev;
	if (level == LOGL_CONT)
		level = gd_logl_prev;

	rec.cat = cat;
	rec.level = level & LOGL_LEVEL_MASK;
	rec.flags = 0;
	if (level & LOGL_FORCE_DEBUG)
		rec.flags |= LOGRECF_FORCE_DEBUG;
	if (gd_log_cont)
		rec.flags |= LOGRECF_CONT;
	rec.file = file;
	rec.line = line;
	rec.func = func;
	rec.msg = NULL;

#if 0
	if (!(gd->flags & GD_FLG_LOG_READY)) {
		gd_log_drop_count++;

		/* display dropped traces with console puts and DEBUG_UART */
		if (rec.level <= CONFIG_LOG_DEFAULT_LEVEL ||
		    rec.flags & LOGRECF_FORCE_DEBUG) {
			char buf[CONFIG_SYS_CBSIZE];

			va_start(args, fmt);
			vsnprintf(buf, sizeof(buf), fmt, args);
			puts(buf);
			va_end(args);
		}

		return -ENOSYS;
	}
#endif
	va_start(args, fmt);
	if (!log_dispatch(&rec, fmt, args)) {
		gd_logc_prev = cat;
		gd_logl_prev = level;
	}
	va_end(args);

	return 0;
}

#define MAX_LINE_LENGTH_BYTES		64
#define DEFAULT_LINE_LENGTH_BYTES	16

int _log_buffer(enum log_category_t cat, enum log_level_t level,
		const char *file, int line, const char *func, ulong addr,
		const void *data, uint width, uint count, uint linelen)
{
	if (linelen * width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	while (count) {
		uint thislinelen;
		char buf[HEXDUMP_MAX_BUF_LENGTH(width * linelen)];

		thislinelen = hexdump_line(addr, data, width, count, linelen,
					   buf, sizeof(buf));
		assert(thislinelen >= 0);
		_log(cat, level, file, line, func, "%s\n", buf);

		/* update references */
		data += thislinelen * width;
		addr += thislinelen * width;
		count -= thislinelen;
	}

	return 0;
}

int log_add_filter_flags(const char *drv_name, enum log_category_t cat_list[],
			 enum log_level_t level, const char *file_list,
			 int flags)
{
	struct log_filter *filt;
	struct log_device *ldev;
	int ret;
	int i;

	ldev = log_device_find_by_name(drv_name);
	if (!ldev)
		return -ENOENT;
	filt = calloc(1, sizeof(*filt));
	if (!filt)
		return -ENOMEM;

	filt->flags = flags;
	if (cat_list) {
		filt->flags |= LOGFF_HAS_CAT;
		for (i = 0; ; i++) {
			if (i == ARRAY_SIZE(filt->cat_list)) {
				ret = -ENOSPC;
				goto err;
			}
			filt->cat_list[i] = cat_list[i];
			if (cat_list[i] == LOGC_END)
				break;
		}
	}
	filt->level = level;
	if (file_list) {
		filt->file_list = strdup(file_list);
		if (!filt->file_list) {
			ret = -ENOMEM;
			goto err;
		}
	}
	filt->filter_num = ldev->next_filter_num++;
	/* Add deny filters to the beginning of the list */
	if (flags & LOGFF_DENY)
		list_add(&filt->sibling_node, &ldev->filter_head);
	else
		list_add_tail(&filt->sibling_node, &ldev->filter_head);

	return filt->filter_num;

err:
	free(filt);
	return ret;
}

int log_remove_filter(const char *drv_name, int filter_num)
{
	struct log_filter *filt;
	struct log_device *ldev;

	ldev = log_device_find_by_name(drv_name);
	if (!ldev)
		return -ENOENT;

	list_for_each_entry(filt, &ldev->filter_head, sibling_node) {
		if (filt->filter_num == filter_num) {
			list_del(&filt->sibling_node);
			free(filt);

			return 0;
		}
	}

	return -ENOENT;
}

/**
 * log_find_device_by_drv() - Find a device by its driver
 *
 * @drv: Log driver
 * Return: Device associated with that driver, or NULL if not found
 */
static struct log_device *log_find_device_by_drv(struct log_driver *drv)
{
	struct log_device *ldev;

	list_for_each_entry(ldev, &log_device_head, sibling_node) {
		if (ldev->drv == drv)
			return ldev;
	}
	/*
	 * It is quite hard to pass an invalid driver since passing an unknown
	 * LOG_GET_DRIVER(xxx) would normally produce a compilation error. But
	 * it is possible to pass NULL, for example, so this
	 */

	return NULL;
}

int log_device_set_enable(struct log_driver *drv, bool enable)
{
	struct log_device *ldev;

	ldev = log_find_device_by_drv(drv);
	if (!ldev)
		return -ENOENT;
	if (enable)
		ldev->flags |= LOGDF_ENABLE;
	else
		ldev->flags &= ~LOGDF_ENABLE;

	return 0;
}

int log_driver_bind_device(struct log_driver *drv, struct log_device *dev)
{
    int ret = 0;

    if(drv->probe && 0!=drv->probe(dev))
    {
        log_red_print("log driver(%s) probe device(%s:%d) fail!\n", drv->name, dev->name, dev->id);
        return ret;
    }

    log_grn_print("log driver(%s) bind a device(%s:%d) success!\n", drv->name, dev->name, dev->id);

	LOCK_ENTER(&drv->device_lock);
    /* add device to the device list in the driver */
    list_add_tail(&dev->driver_node, &drv->device_head);

	LOCK_EXIT(&drv->device_lock);

    /* bind a driver for a device */
    dev->drv = drv;

    /* no probe | probe success */
    return 0;
}

void log_driver_init(struct log_driver *drv)
{
    if(drv)
    {
    	INIT_LIST_HEAD(&drv->device_head);
		LOCK_INIT(&drv->device_lock);
    	INIT_LIST_HEAD(&drv->sibling_node);
    }

    return;
}

int log_driver_register(struct log_driver *drv)
{
    struct log_driver *drv_node;
    struct log_device *dev_node;

    if(NULL==drv || NULL==drv->name)
        return -ENOENT;
    
    log_driver_init(drv);

    LOCK_ENTER(&log_driver_lock);
    list_for_each_entry(drv_node, &log_driver_head, sibling_node)
    {
        if(0 == strcmp(drv->name, drv_node->name))
        {
            LOCK_EXIT(&log_driver_lock);

            log_red_print("log driver(%s) has been existed!\n", drv->name);
            return -EEXIST;
        }
    }

    /* add to the list of driver */
    list_add_tail(&drv->sibling_node, &log_driver_head);
    LOCK_EXIT(&log_driver_lock);

    /* match driver for each device */
	LOCK_ENTER(&log_device_lock);
    list_for_each_entry(dev_node, &log_device_head, sibling_node)
    {
        if(0 == strcmp(drv->name, dev_node->name))
        {
            log_grn_print("log driver(%s) bind a device\n", dev_node->name);
            log_driver_bind_device(drv_node, dev_node);
        }
    }
	LOCK_EXIT(&log_device_lock);

    return 0;
}
struct log_driver *log_driver_find_by_name(const char *name)
{
    struct log_driver *drv_node;

    /* bing a driver for the device */
    LOCK_ENTER(&log_driver_lock);
    list_for_each_entry(drv_node, &log_driver_head, sibling_node)
    {
        if(0 == strcmp(name, drv_node->name))
        {
            LOCK_EXIT(&log_driver_lock);
            return drv_node;
        }
    }
    LOCK_EXIT(&log_driver_lock);

    return NULL;
}

int log_driver_dev_iterate(struct log_driver *drv, int (*iterate_cb)(struct log_device *dev, arg_dstr_t ds ), arg_dstr_t ds)
{
    struct log_device *dev_node;

    LOCK_ENTER(&drv->device_lock);
    list_for_each_entry(dev_node, &drv->device_head, driver_node)
    {
        if(iterate_cb)
            iterate_cb(dev_node, ds);
    }
    
    LOCK_EXIT(&drv->device_lock);

    return 0;
}

void log_device_init(struct log_device *dev)
{
    if(dev)
    {
        INIT_LIST_HEAD(&dev->driver_node);
        INIT_LIST_HEAD(&dev->filter_head);
        INIT_LIST_HEAD(&dev->sibling_node);
    }

    return;
}

int log_device_register(struct log_device *dev)
{
    struct log_driver *drv_node;
    struct log_device *dev_node;
    int id = -1;
   
    if(NULL==dev || NULL==dev->name)
        return -ENOENT;

    log_device_init(dev);

    LOCK_ENTER(&log_device_lock);
    list_for_each_entry(dev_node, &log_device_head, sibling_node)
    {
        if(0 == strcmp(dev->name, dev_node->name))
        {
            if(id < dev_node->id)           /* maximum id value */
                id = dev_node->id;
        }
    }
    
    dev->id = ++id;

    /* add a device to the list */
    list_add_tail(&dev->sibling_node, &log_device_head);
    LOCK_EXIT(&log_device_lock);

    /* bing a driver for the device */
    LOCK_ENTER(&log_driver_lock);
    list_for_each_entry(drv_node, &log_driver_head, sibling_node)
    {
        if(0 == strcmp(dev->name, drv_node->name))
        {
            log_driver_bind_device(drv_node, dev);
            break;
        }
    }
    LOCK_EXIT(&log_driver_lock);

    return 0;
}

int log_device_unregister(struct log_device *dev)
{
    struct log_driver *drv;
	struct log_filter *pos;
	struct log_filter *n;
            
    log_grn_print("remove a device(%s:%d)\n", dev->name, dev->id);

    /* remove a node for device list */
    LOCK_ENTER(&log_device_lock);
    list_del(&dev->sibling_node);
    LOCK_EXIT(&log_device_lock);

    drv = dev->drv;
    /* driver callback */ 
    if(NULL!=drv)
    {
        LOCK_ENTER(&drv->device_lock);
        /* remove node from deivce list of the driver */
        list_del(&dev->driver_node);
        LOCK_EXIT(&drv->device_lock);

        if(drv->remove)
            drv->remove(dev);
    }
    
    dev->drv = NULL;

	/* remove all filter */
	list_for_each_entry_safe(pos, n, &dev->filter_head, sibling_node)
	{
		list_del(&pos->sibling_node);
		free(pos);
	}

    INIT_LIST_HEAD(&dev->driver_node);
    INIT_LIST_HEAD(&dev->filter_head);
    INIT_LIST_HEAD(&dev->sibling_node);

    return 0;
}

int log_device_freeall_by_name(const char *name, int(*free_cb)(struct log_device *dev, arg_dstr_t ds), arg_dstr_t ds)
{
    struct log_device *dpos;
    struct log_device *dn;

    LOCK_ENTER(&log_device_lock);
	list_for_each_entry_safe(dpos, dn, &log_device_head, sibling_node)
	{
        if(0 == strcmp(name, dpos->name))
        {
            LOCK_EXIT(&log_device_lock);
            log_device_unregister(dpos);
            if(free_cb)
                free_cb(dpos, ds);
            LOCK_ENTER(&log_device_lock);
        }
	}
    LOCK_EXIT(&log_device_lock);

    return 0;
}

static int log_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds);

int log_init(void)
{
	LOCK_INIT(&log_device_lock);
	LOCK_INIT(&log_driver_lock);

//	gd->flags |= GD_FLG_LOG_READY;
	if (!gd_default_log_level)
		gd_default_log_level = CONFIG_LOG_DEFAULT_LEVEL;
	gd_log_fmt = log_get_default_format();
	gd_logc_prev = LOGC_NONE;
	gd_logl_prev = LOGL_INFO;


    log_udp_init();

	return 0;
}

void log_init_append(void)
{
    arg_cmd_register("log", log_arg_cmdfn, "manage log module");
}

/* dump log module information to buffer */
int log_dump(arg_dstr_t ds)
{
    struct log_driver *drv_node;
    struct log_device *dev_node;
    char nameid[100];

    arg_dstr_catf(ds, "/****** log summary ******/\n");

    arg_dstr_catf(ds, "\tlog device list:\n");
    arg_dstr_catf(ds, "%-10s %-5s %-15s %-s\n", \
            "name:id", "flag", "filter_num", "driver");
    LOCK_ENTER(&log_device_lock);
    list_for_each_entry(dev_node, &log_device_head, sibling_node)
    {

        snprintf(nameid, sizeof(nameid), "%s:%d", dev_node->name, dev_node->id);
        arg_dstr_catf(ds, "%-10s %#-5x %#-15x %p", \
                nameid, dev_node->flags,\
                dev_node->next_filter_num, dev_node->drv);
        arg_dstr_catf(ds, "\n");
    }
    LOCK_EXIT(&log_device_lock);
    
    arg_dstr_catf(ds, "\n");
    arg_dstr_catf(ds, "\tlog driver list:\n");
    arg_dstr_catf(ds, "%-10s %-20s %-s\n", "name", "addr", "devices");
    LOCK_ENTER(&log_driver_lock);
    list_for_each_entry(drv_node, &log_driver_head, sibling_node)
    {
        arg_dstr_catf(ds, "%-10s %-21p", drv_node->name, drv_node);
        
        LOCK_ENTER(&drv_node->device_lock);  
        list_for_each_entry(dev_node, &drv_node->device_head, driver_node)
            arg_dstr_catf(ds, "%s:%d ", dev_node->name, dev_node->id);
        LOCK_EXIT(&drv_node->device_lock);

        arg_dstr_catf(ds, "\n");
    }
    LOCK_EXIT(&log_driver_lock);

    return 0;
}


/************************************************************
 log [-h]
 log -d

*************************************************************/
static int log_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds)
{
    struct arg_lit *arg_d;
    struct arg_end *end_arg;
    void *argtable[] = 
    {
        arg_d           = arg_lit0("d", "dump",     "dump information about log"),
        end_arg         = arg_end(5),
    };
    int ret;

    ret = argtable_parse(argc, argv, argtable, end_arg, ds, argv[0]);
    if(0 != ret)
        goto to_parse;

    if(arg_d->count > 0)
    {
        log_dump(ds);
        goto to_d;
    }

    /* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", argv[0]);
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-25s %s\r\n");

to_d:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;
}
