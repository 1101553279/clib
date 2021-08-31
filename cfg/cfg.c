#include "cfg.h"
#include "command.h"
#include "inih/ini.h"
#include "blist.h"
#include <string.h>
#include <stdlib.h>

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
                section_node          section_node           section node
         <-----  -----------   <-----  ------------   <-----  ----------
sec_head        | sec:"sock"|         | sec:"plog" |         |sec:"cmd" |
         ----->  -----------   ----->  -----------    ----->  ----------
                   ^ |                      ^ |                   ^ |
                   | V                      | V                   | V
               -----------            ---------------         ---------------
              |name:"port"|          |name:"msg_size"|       |name:"tcps_port"|
              |value:"cmd"|          |value:"20"     |       |value:"3000"    |
               -----------            ---------------         ---------------
                  ^ |                    ^ |               
                  | V                    | V               
             -----------------       -----------------     
            |name:"task_name" |     |name:"task_name" |    
            |value:"cmd"      |     |value:"plog"     |    
             -----------------       -----------------     

********************************************************************/
#define CFG_NAME "clib.ini"

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
    struct list_head sec_head;  /* section head */
    int count;                  /* section count */
};

static struct cfg_mgr cfg_obj;

static int cfg_property_add(struct cfg_section *sec, const char *name, const char *value);
static int cfg_section_add(struct cfg_mgr *mgr, const char *section, 
        const char *name, const char *value);
static int cfg_ini_callback(void *user, const char *section, 
        const char *name, const char *value);

void cfg_init(void)
{
    struct cfg_mgr *c = &cfg_obj;

    INIT_LIST_HEAD(&c->sec_head);
    c->count = 0;

    ini_parse(CFG_NAME, cfg_ini_callback, c);
    
    return;
}

int cfg_read(char *sec, char *key, void *value)
{
    
    return 0;
}

u16_t cfg_dump(char *buff, u16_t len)
{
    struct cfg_mgr *c = &cfg_obj;
    struct cfg_section *sec;
    struct cfg_property *pro;
    u16_t ret = 0;
    struct list_head *pos1 = NULL;
    struct list_head *pos2 = NULL;
    int count = 0;
    int max_count = 0;

    /* section title */
    ret += snprintf(buff+ret, len-ret, "/*** cfg summary ***/\r\n");
    list_for_each(pos1, &c->sec_head)
    {
        sec = list_entry(pos1, struct cfg_section, node);
        ret += snprintf(buff+ret, len-ret, "(%02d)%-20s", sec->count, sec->sec);
        if(sec->count > max_count)  /* get maximum count */
            max_count = sec->count;
    }
    ret += snprintf(buff+ret, len-ret, "\r\n");

#if 0
    /* property context */
    list_for_each(pos1, &c->sec_head)
    {
        sec = list_entry(pos1, struct cfg_section, node);
        list_for_each(pos2, &sec->pro_head)
        {
            pro = list_entry(pos2, struct cfg_property, node);
            ret += snprintf(buff+ret, len-ret, "%-10s:%-10s", pro->name, pro->value);
            printf("%-10s:%-10s", pro->name, pro->value);
        }
        ret += snprintf(buff+ret, len-ret, "\r\n");
    }
#endif

#if 0
    printf("section count = %d\r\n", c->count);
    list_for_each(pos1, &c->sec_head)
    {
        sec = list_entry(pos1, struct cfg_section, node);
        ret += snprintf(buff+ret, len-ret, "%-10s(%d):\t", sec->sec, sec->count);
        list_for_each(pos2, &sec->pro_head)
        {
            pro = list_entry(pos2, struct cfg_property, node);
            printf("\t%-10s:%-10s\r\n", pro->name, pro->value);
            pro = list_entry(pos2, struct cfg_property, node);
            ret += snprintf(buff+ret, len-ret, "%-10s:%-10s", pro->name, pro->value);
            printf("%-10s:%-10s", pro->name, pro->value);
        }
        ret += snprintf(buff+ret, len-ret, "\r\n");
    }
#endif
    
    return ret;
}

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
            flag = 1;
            break;
        }
    }
    
    if(1==flag && NULL!=pro)     /* has been added */
    {
        printf("name(%s):value(%s) repalace name(%s):value(%s)\r\n", pro->name, pro->value, name, value);
        if(NULL != pro->name)
            free(pro->name);
        if(NULL != pro->value)
            free(pro->value);
        pro->name = strdup(name);
        pro->value = strdup(value);
    }
    else                        /* has not been added */
    {
        pro = (struct cfg_property *)malloc(sizeof(struct cfg_property));
        if(NULL != pro)
        {
            pro->name = strdup(name);
            pro->value = strdup(value);
            list_add(&pro->node, &sec->pro_head); 
            sec->count += 1;
        }
        else
            return 0;
    }

    return 1;
}

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
        }
        else
            ret = 0;        /* error */
    }

    return ret;
}

static int cfg_ini_callback(void *user, const char *section, 
        const char *name, const char *value)
{
    struct cfg_mgr *c = (struct cfg_mgr *)user;
    
    return cfg_section_add(c, section, name, value);
}
