#include "cfg.h"
#include "command.h"
#include "inih/ini.h"
#include "blist.h"
#include <string.h>
#include <stdlib.h>

#define CFG_NAME "clib.ini"


struct cfg_property{
    struct list_head node;
    char *name;
    char *value; 
};

struct cfg_section{
    struct list_head pro_head;  /* property head */
    char *sec;                  /* section */
};

struct cfg_mgr{
    struct list_head sec_head;  /* section head */
};

static struct cfg_mgr cfg_obj;

static int cfg_command(int argc, char *argv[], char *buff, int len, void *user)
{

    return 0;
}

int cfg_property_add(struct list_head *head, const char *name, const char *value)
{
    struct list_head *pos = NULL;
    struct cfg_property *pro = NULL;
    int flag = 0;

    /* iterate the property list */
    list_for_each(pos, head)
    {
        pro = list_entry(pos, struct cfg_property, node);
        if(0 == strcmp(name, pro->name))
        {
            flag = 1;
            break;
        }
    }
    
    if(1==flag)     /* has been added */
    {
    }
    else            /* has not been added */
    {
    }


    return 1;
}

int cfg_section_add(struct list_head *head, const char *section, const char *name, const char *value)
{
    struct list_head *pos;
    struct cfg_section *sec;
    int flag = 0;
    int ret = 0;
    
    /* iterate the section list */
    list_for_each(pos, head)
    {
        sec = list_entry(pos, struct cfg_section, pro_head);
        if(0 == strcmp(section, sec->sec))
        {
            flag = 1;
            break;
        }
    }

    if(1==flag && NULL!=sec)        /* section has been existed */
        ret = cfg_property_add(&sec->pro_head, name, value);
    else                            /* section has not been added */
    {
        sec = (struct cfg_section *)malloc(sizeof(struct cfg_section));
        if(NULL != sec)
        {
            INIT_LIST_HEAD(&sec->pro_head);
            sec->sec = strdup(section);
            ret = cfg_property_add(&sec->pro_head, name, value);
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
    
    return cfg_section_add(&c->sec_head, section, name, value);
}

void cfg_init(void)
{
    struct cfg_mgr *c = &cfg_obj;

    INIT_LIST_HEAD(&c->sec_head);

    ini_parse(CFG_NAME, cfg_ini_callback, c);

    return;
}

int cfg_read(char *sec, char *key, void *value)
{
    
    return 0;
}


u16_t cfg_dump(char *buff, u16_t len)
{
}
