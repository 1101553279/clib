#include "cfg.h"
#include "command.h"
#include "inih/ini.h"
#include <string.h>
#include <stdlib.h>
#include "plog.h"
#include "log.h"
#include "util.h"

/*******************************************************************
example: cfg.ini 
[cmd]
    task_name = cmd
    port = 3000
[plog]
    task_name= plog
    msg_size = 20
[sock]
    tcps_port = 3000

list structure:
                cfg_section           cfg_section           cfg_section
         <-----  -----------  <-----  ------------   <-----  ----------
sec_head    | section:"sock"|        | section:"plog" |         |section:"cmd" |
         ----->  -----------  ----->  -----------    ----->  ----------
                   ^ |                    ^ |                   ^ |
                   | V                    | V                   | V
              -----------          ---------------       ---------------
cfg_propety  |key:"port" |        |key:"msg_size"|      |key:"tcps_port"|
             |value:"cmd"|        |value:"20"     |     |value:"3000"    |
              -----------          ---------------       ---------------
                  ^ |                    ^ |               
                  | V                    | V               
             -----------------       -----------------     
            |key:"task_name"  |     |key:"task_name" |    
            |value:"cmd"      |     |value:"plog"     |    
             -----------------       -----------------     

********************************************************************/
#define CFG_NAME "clib.ini"
#define CFG_SPLIT   "------------------------------------------"\
                    "------------------------\r\n"


struct cfg_mgr{
    u8_t init;
    struct list_head sec_head;  /* section head */
    int count;                  /* section count */
};

static struct cfg_mgr cfg_obj;

static struct cfg_proper *cfg_at(struct cfg_mgr *mgr, int sno, int pno);
static struct cfg_proper *cfg_proper_at(struct cfg_section *sec, int pno);
static int cfg_proper_add(struct cfg_section *sec, const char *key, const char *value);
static struct cfg_section *cfg_section_at(struct cfg_mgr *mgr, int sno);
static int cfg_section_add(struct cfg_mgr *mgr, const char *section, 
        const char *key, const char *value);
static int cfg_ini_callback(void *user, const char *section, 
        const char *key, const char *value);
/***************************************************
    cfg [-h]
    cfg -d
    cfg -r
***************************************************/
static int cfg_arg_cmdfn(int argc, char *argv[], arg_dstr_t ds)
{
    struct cfg_mgr *c = &cfg_obj;
    struct arg_lit *arg_h;
    struct arg_lit *arg_d;
    struct arg_lit *arg_r;
    struct arg_end *end_arg;
    void *argtable[] = 
    {
        arg_h           = arg_lit0("h", "help",   "help information about cfg command"),
        arg_d           = arg_lit0("d", "dump",   "dump information about cfg module"),
        arg_r           = arg_lit0("r", "reload", "reload(reparse) configure file"),
        end_arg         = arg_end(5),
    };
    int ret;

    ret = argtable_parse(argc, argv, argtable, end_arg, ds, argv[0]);
    if(0 != ret)
        goto to_parse;

    if(arg_h->count > 0)
    {
        arg_dstr_catf(ds, "usage: %s", argv[0]);
        arg_print_syntaxv_ds(ds, argtable, "\r\n");
        goto to_h;
    }

    if(arg_d->count > 0)
    {
        cfg_dump(ds);
        goto to_d;
    }

    if(arg_r->count > 0)
    {
        ini_parse(CFG_NAME, cfg_ini_callback, c);
        arg_dstr_catf(ds, "cfg reload %s - success!\r\n", CFG_NAME);
        goto to_r;
    }
    
    /* help usage for it's self */
    arg_dstr_catf(ds, "usage: %s", argv[0]);
    arg_print_syntaxv_ds(ds, argtable, "\r\n");
    arg_print_glossary_ds(ds, argtable, "%-20s %s\r\n");

to_r:
to_d:
to_h:
to_parse:
    arg_freetable(argtable, ARY_SIZE(argtable));
    return 0;

}

/****************************************************************************** 
 * brief    : init cfg module
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
void cfg_init(void)
{
    struct cfg_mgr *c = &cfg_obj;

    INIT_LIST_HEAD(&c->sec_head);
    c->count = 0;

    ini_parse(CFG_NAME, cfg_ini_callback, c);

    c->init = 1;
    
    return;
}

/****************************************************************************** 
 * brief    : the second init period
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
void cfg_init_append(void)
{
    arg_cmd_register("cfg", cfg_arg_cmdfn, "manage cfg module");

    return;
}

struct cfg_section_find_cb_data{
    struct cfg_section *sec;
    const char *section;
};

int cfg_section_find_cb(struct cfg_section *sec, void *user)
{
    struct cfg_section_find_cb_data *data= (struct cfg_section_find_cb_data *)user;

    /* match section */
    if(NULL!=sec && NULL!=data 
    && 0==strcmp(data->section, sec->section))     /* section match */
    {
        plog(CFG, "find section(%s)\r\n", sec->section);
        data->sec = sec;
        return 1;
    }
    
    return 0;
}

/****************************************************************************** 
 * brief    : find section in section list
 * Param    : section
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
struct cfg_section *cfg_section_find(const char *section)
{
    struct cfg_section_find_cb_data user = {NULL, section};

    cfg_section_iterate(cfg_section_find_cb, &user);

    return user.sec;
}

struct cfg_proper_find_cb_data{
    struct cfg_proper *pro;
    const char *section;
    const char *key;
};

int cfg_proper_find_cb(const char *section, struct cfg_proper *pro, void *user)
{
    struct cfg_proper_find_cb_data *data= (struct cfg_proper_find_cb_data *)user;

    /* match property */
    if(NULL!=pro && NULL!=data && 0!=strlen(section)
    && 0==strcmp(data->section, section)    /* section match */
    && 0==strcmp(data->key, pro->key))      /* key match */
    {
        plog(CFG, "find property - section(%s):propery(%s:%s)\r\n", 
                section, pro->key, pro->value);
        data->pro = pro;
        return 1;
    }
    
    return 0;
}

/****************************************************************************** 
 * brief    : find property in configure list
 * Param    : key       property's key
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
struct cfg_proper *cfg_proper_find(const char *section, const char *key)
{
    struct cfg_proper_find_cb_data user = {NULL, section, key};

    cfg_proper_iterate(cfg_proper_find_cb, &user);
    
    return user.pro;
}

/****************************************************************************** 
 * brief    : find property in section list
 * Param    : key       property's key
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
struct cfg_proper *cfg_proper_find_in_section(struct cfg_section *sec, const char *key)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_proper *pro = NULL;
    struct list_head *pos;
    
    if(1 != c->init)
    {
        log_red("cfg has not been inited - can't find\r\n");
        return NULL;
    }

    list_for_each(pos, &sec->pro_head)
    {
        pro = list_entry(pos, struct cfg_proper, node);
        if(0 == strcmp(key, pro->key))
        {
            plog(CFG, "find property - section(%s):propery(%s:%s)\r\n", 
                    sec->section, pro->key, pro->value);
            return pro;
        }
    }

    return NULL;
}

/****************************************************************************** 
 * brief    : iterate section list
 * Param    : cb        callback for every section node in section list
 * Param    : user      
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
void cfg_section_iterate(cfg_section_cb_t cb, void *user)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec = NULL;
    struct list_head *pos;
    
    if(1 != c->init)
    {
        log_red("cfg has not been inited - can't iterate\r\n");
        return;
    }

    list_for_each(pos, &c->sec_head)
    {
        sec = list_entry(pos, struct cfg_section, node);
        if(NULL==cb || 0!=cb(sec,user))
        {
            plog(CFG, "stop iterate section(%s)\r\n", sec->section);
            break;
        }
    }

    return;
}

/****************************************************************************** 
 * brief    : iterate propery in configure list
 * Param    : cb        callback for every property node in configure list
 * Param    : user      
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
void cfg_proper_iterate(cfg_proper_cb_t cb, void *user)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_proper *pro;
    struct list_head *pos1;
    struct list_head *pos2;
    int stop = 0;
    
    if(1 != c->init)
    {
        log_red("cfg has not been inited - can't iterate\r\n");
        return;
    }

    list_for_each(pos1, &c->sec_head)
    {
        sec = list_entry(pos1, struct cfg_section, node);
        list_for_each(pos2, &sec->pro_head)
        {
            pro = list_entry(pos2, struct cfg_proper, node);
            if(NULL== cb || 0!=cb(sec->section, pro, user))
            {
                plog(CFG, "stop iterate section(%s):proper(%s:%s)\r\n", 
                        sec->section, pro->key, pro->value);
                goto stop;
            }
        }
    }

stop:
    return;
}

/****************************************************************************** 
 * brief    : return default value of the section & key
 * Param    : section
 * Param    : key
 * Param    : ok
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
const char *cfg_default_read(const char *section, const char *key, int *ok)
{
    char *value = NULL;

    if(0 == strcmp("cmd", section))
    {
        if(0 == strcmp("name", key))
            value = "cmd_task";
        else if(0 == strcmp("port", key))
            value = "3000";
    }
    else if(0 == strcmp("plog", section))
    {
        if(0 == strcmp("name", key))
            value = "plog_task";
        else if(0 == strcmp("msize", key))
            value = "40";
    }
    else if(0 == strcmp("sock", section))
    {
        if(0 == strcmp("name", key))
            value = "sock_task";
        else if(0 == strcmp("tport", key))
            value = "3000";
    }

    if(NULL != value)   /* success */
        if(NULL != ok)
            *ok = 1;
    else                /* fail */
        if(NULL != ok)
            *ok = 0;

    return value;
}

/****************************************************************************** 
 * brief    : get value of property of section
 * Param    : section
 * Param    : key
 * Param    : ok
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
const char *cfg_read(const char *section, const char *key, int *ok)
{
    const char *value = NULL;
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_proper *pro = NULL;

    if(NULL != ok)
        *ok = 0;        /* default */
    
    if(1 != c->init)
    {
        log_red("cfg has not been inited - can't read\r\n");
        return NULL;
    }

    if(NULL==section || NULL==key)
        return NULL;

    pro = cfg_proper_find(section, key);
    if(NULL != pro)
    {
        plog(CFG, "find property(%s:%s) in section(%s)\r\n", 
                pro->key, pro->value, section);
        if(NULL != ok)      /* get ok */
            *ok = 1;
        return pro->value;
    }

    /* set default value */
    if(NULL == (value=cfg_default_read(section, key, ok)))
        plog(CFG, "not found property(%s) in section(%s)\r\n",
                key, section);
    
    return value;
}


/******************************************************************
output example:
*** cfg summary ***
	sock                    plog                    cmd                     
------------------------------------------------------------------
 0 |         ks:22         |   msg_size:20         |       port:3000       
------------------------------------------------------------------
 1 |         kk:213        |           :           |           :           
------------------------------------------------------------------
 2 |  tcps_port:3000       |           :           |       
*******************************************************************/
int cfg_dump(arg_dstr_t ds)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_proper *pro;
    int max_count = 0;
    int i = 0;
    int j = 0;

    /* section title */
    arg_dstr_catf(ds, "/*** cfg summary ***/\r\n");
    arg_dstr_catf(ds, "%3s ", " ");
    for(i=0; i < c->count; i++)
    {
        sec = cfg_section_at(c, i);
        if(NULL != sec)
        {
            arg_dstr_catf(ds, "%-24s", sec->section);
            if(sec->count > max_count)
                max_count = sec->count;
        }
    }

    /* property key */
    for(i=0; i < max_count; i++)
    {
        arg_dstr_catf(ds, "\r\n");
        arg_dstr_catf(ds, CFG_SPLIT);
        arg_dstr_catf(ds, "%2d ", i);
        for(j=0; j < c->count; j++)
        {
            pro = cfg_at(c, j, i);
            if(NULL != pro)
                arg_dstr_catf(ds, "| %10s:%-10s ", pro->key, pro->value);
            else
                arg_dstr_catf(ds, "| %10s:%10s ", " ", " ");
        }
    }
    arg_dstr_catf(ds, "\r\n");

    return 0;
}

/* print all configure information */
void cfg_print(void)
{
    u16_t ret = 0;
    char buff[1024];
    arg_dstr_t ds = arg_dstr_create();
    
    cfg_dump(ds);
    printf("%s", arg_dstr_cstr(ds));

    return;
}

/* print one section list information */
void cfg_section_print(struct cfg_section *sec)
{
    struct cfg_proper *pro = NULL;
    struct list_head *pos;

    if(NULL == sec)
        return;

    printf("/****** section(%s:%d) ******\r\n", sec->section, sec->count);
    list_for_each(pos, &sec->pro_head)
    {
        pro = list_entry(pos, struct cfg_proper, node);
        printf("\t%s:%s\r\n", pro->key, pro->value);
    }

    return;
}

/* print one property information */
void cfg_proper_print(struct cfg_proper *pro)
{
    if(NULL == pro)
        return;

    printf("/****** proper ******\r\n");
    printf("\t%s:%s\r\n", pro->key, pro->value);

    return;
}
/****************************************************************************** 
 * brief    : get property at pno:sno position 
 * Param    : mgr       cfg manager
 * Param    : sno       section no
 * Param    : pno       property no
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static struct cfg_proper *cfg_at(struct cfg_mgr *mgr, int sno, int pno)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_proper *pro;

    sec = cfg_section_at(c, sno);
    if(NULL == sec)
    {
        plog(CFG, "get section at sno(%d) fail - mgr->count=%d\r\n", sno, c->count);
        return NULL;
    }

    pro = cfg_proper_at(sec, pno);
    if(NULL == pro)
    {
        plog(CFG, "get property at pno(%d) in section(%s) at sno(%d) fail - sec->count=%d\r\n",
            pno, sec->section, sno, sec->count);
        return NULL;
    }

    plog(CFG, "get property at pno(%s:%d) sno(%d) is key(%s):value(%s)\r\n", 
            sec->section, pno, sno, pro->key, pro->value);

    return pro;
}

/****************************************************************************** 
 * brief    : get property in section at pno position 
 * Param    : sec 		section pointer
 * Param    : pno       property no
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static struct cfg_proper *cfg_proper_at(struct cfg_section *sec, int pno)   
{
    struct cfg_proper *pro;
    struct list_head *pos;

    if(pno<0 || pno>=sec->count)
    {
        plog(CFG, "pno(%d) is invalid sec->count=%d\r\n", pno, sec->count);
        return NULL;
    }

    /* iterate property list */
    list_for_each(pos, &sec->pro_head)
    {
        if(0 != pno--)
            continue;

        pro = list_entry(pos, struct cfg_proper, node);
		plog(CFG, "find property(%s:%s) in section(%s) - success!\r\n", 
						pro->key, pro->value, sec->section);
        return pro;
    }

    return NULL;
}

/****************************************************************************** 
 * brief    : add seciton & property into list 
 * Param    : sec 		section name 
 * Param    : key		property key
 * Param    : avlue		property value
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static int cfg_proper_add(struct cfg_section *sec, const char *key, const char *value)
{
    struct list_head *pos = NULL;
    struct cfg_proper *pro = NULL;
    int flag = 0;

    /* iterate the property list */
    list_for_each(pos, &sec->pro_head)
    {
        pro = list_entry(pos, struct cfg_proper, node);
        if(0 == strcmp(key, pro->key))
        {
            plog(CFG, "find key(%s) - value(%s)\r\n", key, value);
            flag = 1;
            break;
        }
    }
    
    if(1==flag && NULL!=pro)     /* has been added - replace */
    {
        plog(CFG, "replace property key(%s):value(%s) <- key(%s):value(%s) in section(%s)\r\n", 
					pro->key, pro->value, key, value, sec->section);
        if(NULL != pro->key)
            free(pro->key);
        if(NULL != pro->value)
            free(pro->value);
        pro->key = strdup(key);
        pro->value = strdup(value);
    }
    else                        /* has not been added - add */
    {
        pro = (struct cfg_proper *)malloc(sizeof(struct cfg_proper));
        if(NULL != pro)
        {
            pro->key = strdup(key);
            pro->value = strdup(value);
            list_add(&pro->node, &sec->pro_head); 
            sec->count += 1;
			plog(CFG, "add properpty key(%s):value(%s) success!\r\n", pro->key, pro->value);
        }
        else
        {
            plog(CFG, "add property key(%s):value(%s) malloc fail!\r\n", key, value);
            return 0;
        }
    }

    return 1;
}

/****************************************************************************** 
 * brief    : get section structure in sno postion
 * Param    : mgr	manager
 * Param    : sno	section no
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static struct cfg_section *cfg_section_at(struct cfg_mgr *mgr, int sno)   
{
    struct cfg_section *sec;
    struct list_head *pos;

    if(sno<0 || sno>mgr->count)
    {
        plog(CFG, "sno(%d) is invalid mgr->count=%d\r\n", sno, mgr->count);
        return NULL;
    }

	/* iterate the section list */
    list_for_each(pos, &mgr->sec_head)
    {
        if(0 != sno--)
            continue;
        sec = list_entry(pos, struct cfg_section, node);
		plog(CFG, "find section(%s) - success!\r\n", sec->section);
        return sec;
    }

    return NULL;
}

/****************************************************************************** 
 * brief    : add section && property
 * Param    : mgr		cfg manager
 * Param    : section 	section name
 * Param    : key		property key
 * Param    : avlue		property value
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static int cfg_section_add(struct cfg_mgr *mgr, const char *section, 
        const char *key, const char *value)
{
    struct list_head *pos;
    struct cfg_section *sec;
    int flag = 0;
    int ret = 0;

    /* whether the parameters are valid? */
    if(NULL==mgr || NULL==section || NULL==key || NULL==value
    || 0==strlen(section) || 0==strlen(key) || 0==strlen(value))
        return 0;
    
    /* iterate the section list */
    list_for_each(pos, &mgr->sec_head)
    {
        sec = list_entry(pos, struct cfg_section, node);
        if(0 == strcmp(section, sec->section))
        {
			plog(CFG, "find section(%s)!\r\n", sec->section);
            flag = 1;
            break;
        }
    }

    if(1==flag && NULL!=sec)        /* section has been existed */
        ret = cfg_proper_add(sec, key, value);
    else                            /* section has not been added */
    {
        sec = (struct cfg_section *)malloc(sizeof(struct cfg_section));
        if(NULL != sec)
        {
            INIT_LIST_HEAD(&sec->pro_head);
            sec->section = strdup(section);
            ret = cfg_proper_add(sec, key, value);
            list_add(&sec->node, &mgr->sec_head);
            mgr->count += 1;        /* new section */
			plog(CFG, "add section(%s) success!\r\n", sec->section);
        }
        else
            ret = 0;        /* error */
    }

    return ret;
}

/* ini_parse's callback */
static int cfg_ini_callback(void *user, const char *section, 
        const char *key, const char *value)
{
    struct cfg_mgr *c = (struct cfg_mgr *)user;
   	
	plog(CFG, "ini add : section(%s), key(%s), value(%s)!\r\n", 
			section, key, value);
    return cfg_section_add(c, section, key, value);
}

