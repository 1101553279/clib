#include "cfg.h"
#include "command.h"
#include "inih/ini.h"
#include "blist.h"
#include <string.h>
#include <stdlib.h>
#include "plog.h"
#include "log.h"

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
sec_head        | sec:"sock"|        | sec:"plog" |         |sec:"cmd" |
         ----->  -----------  ----->  -----------    ----->  ----------
                   ^ |                    ^ |                   ^ |
                   | V                    | V                   | V
              -----------          ---------------       ---------------
cfg_propety  |name:"port"|        |name:"msg_size"|     |name:"tcps_port"|
             |value:"cmd"|        |value:"20"     |     |value:"3000"    |
              -----------          ---------------       ---------------
                  ^ |                    ^ |               
                  | V                    | V               
             -----------------       -----------------     
            |name:"task_name" |     |name:"task_name" |    
            |value:"cmd"      |     |value:"plog"     |    
             -----------------       -----------------     

********************************************************************/
#define CFG_NAME "clib.ini"
#define CFG_SPLIT   "------------------------------------------"\
                    "------------------------\r\n"

struct cfg_property{
    struct list_head node;
    char *name;
    char *value; 
};

struct cfg_section{
    struct list_head node;      /* for section list */
    struct list_head pro_head;  /* property head, for property list  */
    char *sec;                  /* section name */
    int count;                  /* property count */
};

struct cfg_mgr{
    u8_t init;
    struct list_head sec_head;  /* section head */
    int count;                  /* section count */
};

static struct cfg_mgr cfg_obj;

static struct cfg_property *cfg_at(struct cfg_mgr *mgr, int sno, int pno);
static struct cfg_property *cfg_property_at(struct cfg_section *sec, int pno);
static int cfg_property_add(struct cfg_section *sec, const char *name, const char *value);
static struct cfg_section *cfg_section_at(struct cfg_mgr *mgr, int sno);
static int cfg_section_add(struct cfg_mgr *mgr, const char *section, 
        const char *name, const char *value);
static int cfg_ini_callback(void *user, const char *section, 
        const char *name, const char *value);
static int cfg_command(int argc, char *argv[], char *buff, int len, void *user);

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
    /* add plog command */
    cmd_add("cfg", "cfg information", CMD_INDENT"cfg <reload|dump>", cfg_command, NULL);

    return;
}

/****************************************************************************** 
 * brief    : get value of property of section
 * Param    : sno       section no
 * Param    : pno       property no
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
int cfg_read(const char *sec_name, const char *key, char** value)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_property *pro;
    struct list_head *pos1;
    struct list_head *pos2;
    int ret = -1;
    
    if(1 != c->init)
    {
        log_red("cfg has not been inited - can't read\r\n");
        return -1;
    }

    if(NULL==sec || NULL==key || NULL==value)
        return -1;

    list_for_each(pos1, &c->sec_head)
    {
        sec = list_entry(pos1, struct cfg_section, node);
        if(0 == strcmp(sec_name, sec->sec))
        {
            plog(CFG, "find section(%s)!\r\n", sec->sec);
            list_for_each(pos2, &sec->pro_head)
            {
                pro = list_entry(pos2, struct cfg_property, node);
                if(0 == strcmp(key, pro->name))
                {
                    *value = pro->value;
                    ret = 0;
                    break;
                }
            }
            break;
        }
    }

    if(0 == ret)
        plog(CFG, "find property(%s:%s) in section(%s)\r\n", 
                pro->name, pro->value, sec->sec);
    else
        plog(CFG, "not found property(%s) in section(%s)\r\n",
                key, sec_name);
    
    return ret;
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
u16_t cfg_dump(char *buff, u16_t len)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_property *pro;
    u16_t ret = 0;
    int max_count = 0;
    int i = 0;
    int j = 0;

    /* section title */
    ret += snprintf(buff+ret, len-ret, "/*** cfg summary ***/\r\n");
    ret += snprintf(buff+ret, len-ret, "%3s ", " ");
    for(i=0; i < c->count; i++)
    {
        sec = cfg_section_at(c, i);
        if(NULL != sec)
        {
            ret += snprintf(buff+ret, len-ret, "%-24s", sec->sec);
            if(sec->count > max_count)
                max_count = sec->count;
        }
    }

    /* property name */
    for(i=0; i < max_count; i++)
    {
        ret += snprintf(buff+ret, len-ret, "\r\n");
        ret += snprintf(buff+ret, len-ret, CFG_SPLIT);
        ret += snprintf(buff+ret, len-ret, "%2d ", i);
        for(j=0; j < c->count; j++)
        {
            pro = cfg_at(c, j, i);
            if(NULL != pro)
                ret += snprintf(buff+ret, len-ret, "| %10s:%-10s ", pro->name, pro->value);
            else
                ret += snprintf(buff+ret, len-ret, "| %10s:%10s ", " ", " ");
        }
    }
    ret += snprintf(buff+ret, len-ret, "\r\n");

    return ret;
}

/****************************************************************************** 
 * brief    : get property at pno:sno position 
 * Param    : mgr       cfg manager
 * Param    : sno       section no
 * Param    : pno       property no
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static struct cfg_property *cfg_at(struct cfg_mgr *mgr, int sno, int pno)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_property *pro;

    sec = cfg_section_at(c, sno);
    if(NULL == sec)
    {
        plog(CFG, "get section at sno(%d) fail - mgr->count=%d\r\n", sno, c->count);
        return NULL;
    }

    pro = cfg_property_at(sec, pno);
    if(NULL == pro)
    {
        plog(CFG, "get property at pno(%d) in section(%s) at sno(%d) fail - sec->count=%d\r\n",
            pno, sec->sec, sno, sec->count);
        return NULL;
    }

    plog(CFG, "get property at pno(%s:%d) sno(%d) is name(%s):value(%s)\r\n", 
            sec->sec, pno, sno, pro->name, pro->value);

    return pro;
}

/****************************************************************************** 
 * brief    : get property in section at pno position 
 * Param    : sec 		section pointer
 * Param    : pno       property no
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static struct cfg_property *cfg_property_at(struct cfg_section *sec, int pno)   
{
    struct cfg_property *pro;
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

        pro = list_entry(pos, struct cfg_property, node);
		plog(CFG, "find property(%s:%s) in section(%s) - success!\r\n", 
						pro->name, pro->value, sec->sec);
        return pro;
    }

    return NULL;
}

/****************************************************************************** 
 * brief    : add seciton & property into list 
 * Param    : sec 		section name 
 * Param    : name		property name
 * Param    : avlue		property value
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static int cfg_property_add(struct cfg_section *sec, const char *name, const char *value)
{
    struct list_head *pos = NULL;
    struct cfg_property *pro = NULL;
    int flag = 0;

    /* iterate the property list */
    list_for_each(pos, &sec->pro_head)
    {
        pro = list_entry(pos, struct cfg_property, node);
        if(0 == strcmp(name, pro->name))
        {
            plog(CFG, "find name(%s) - name(%s)\r\n", name, value);
            flag = 1;
            break;
        }
    }
    
    if(1==flag && NULL!=pro)     /* has been added - replace */
    {
        plog(CFG, "replace property name(%s):value(%s) <- name(%s):value(%s) in section(%s)\r\n", 
					pro->name, pro->value, name, value, sec->sec);
        if(NULL != pro->name)
            free(pro->name);
        if(NULL != pro->value)
            free(pro->value);
        pro->name = strdup(name);
        pro->value = strdup(value);
    }
    else                        /* has not been added - add */
    {
        pro = (struct cfg_property *)malloc(sizeof(struct cfg_property));
        if(NULL != pro)
        {
            pro->name = strdup(name);
            pro->value = strdup(value);
            list_add(&pro->node, &sec->pro_head); 
            sec->count += 1;
			plog(CFG, "add properpty name(%s):value(%s) success!\r\n", pro->name, pro->value);
        }
        else
        {
            plog(CFG, "add property name(%s):value(%s) malloc fail!\r\n", name, value);
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
		plog(CFG, "find section(%s) - success!\r\n", sec->sec);
        return sec;
    }

    return NULL;
}

/****************************************************************************** 
 * brief    : add section && property
 * Param    : mgr		cfg manager
 * Param    : section 	section name
 * Param    : name		property name
 * Param    : avlue		property value
 * Return   : 
 * Note     :                                                                   
 ******************************************************************************/
static int cfg_section_add(struct cfg_mgr *mgr, const char *section, 
        const char *name, const char *value)
{
    struct list_head *pos;
    struct cfg_section *sec;
    int flag = 0;
    int ret = 0;
    
    /* iterate the section list */
    list_for_each(pos, &mgr->sec_head)
    {
        sec = list_entry(pos, struct cfg_section, node);
        if(0 == strcmp(section, sec->sec))
        {
			plog(CFG, "find section(%s)!\r\n", sec->sec);
            flag = 1;
            break;
        }
    }

    if(1==flag && NULL!=sec)        /* section has been existed */
        ret = cfg_property_add(sec, name, value);
    else                            /* section has not been added */
    {
        sec = (struct cfg_section *)malloc(sizeof(struct cfg_section));
        if(NULL != sec)
        {
            INIT_LIST_HEAD(&sec->pro_head);
            sec->sec = strdup(section);
            ret = cfg_property_add(sec, name, value);
            list_add(&sec->node, &mgr->sec_head);
            mgr->count += 1;        /* new section */
			plog(CFG, "add section(%s) success!\r\n", sec->sec);
        }
        else
            ret = 0;        /* error */
    }

    return ret;
}

/* ini_parse's callback */
static int cfg_ini_callback(void *user, const char *section, 
        const char *name, const char *value)
{
    struct cfg_mgr *c = (struct cfg_mgr *)user;
   	
	plog(CFG, "ini add : section(%s), name(%s), value(%s)!\r\n", 
			section, name, value);
    return cfg_section_add(c, section, name, value);
}

static int cfg_command(int argc, char *argv[], char *buff, int len, void *user)
{
    struct cfg_mgr *c = &cfg_obj;
    int ret = 0;
    char *cmd;
    int mark = 1;
    int i = 0;
    
    if(2 == argc)
    {
        cmd = argv[1];
        if(0 == strcmp("reload", cmd))
        {
            ini_parse(CFG_NAME, cfg_ini_callback, c);
            ret += snprintf(buff+ret, len-ret, "cfg reload %s - success!\r\n", 
                    CFG_NAME);
        }
        else
            mark = 0;
    }
    else
        mark = 0;

    if(0 == mark)
    {
        for(i=0; i < argc; i++)
            ret += snprintf(buff+ret, len-ret, "%s ", argv[i]);
        ret += snprintf(buff+ret, len-ret, " <- no this cmd\r\n");
    }

    return ret;
}

