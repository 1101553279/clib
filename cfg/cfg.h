#ifndef __CFG_H__
#define __CFG_H__

#include "util/blist.h"
#include "include/btype.h"
#include "cmd/argtable3.h"

struct cfg_proper{
    struct list_head node;
    char *key;
    char *value; 
};

struct cfg_section{
    struct list_head node;      /* for section list */
    struct list_head pro_head;  /* property head, for property list  */
    char *section;              /* section name */
    int count;                  /* property count */
};

/* init cfg module */
void cfg_init(void);
void cfg_init_append(void);

/* find section accordding to section name */
struct cfg_section *cfg_section_find(const char *section);

/* find section accordding to section:key name */
struct cfg_proper *cfg_proper_find(const char *section, const char *key);
struct cfg_proper *cfg_proper_find_in_section(struct cfg_section *sec, const char *key);

/* iterate a cfg's all of sections */
typedef int (* cfg_section_cb_t)(struct cfg_section *sec, void *user);
void cfg_section_iterate(cfg_section_cb_t cb, void *user);

/* iterate a section's all of properties */
typedef int (* cfg_proper_cb_t)(const char *section, struct cfg_proper *pro, void *user);
void cfg_proper_iterate(cfg_proper_cb_t cb, void *user);

const char *cfg_read(const char *section, const char *key, int *ok);

/* for debug */
int cfg_dump(arg_dstr_t ds);
void cfg_print(void);
void cfg_section_print(struct cfg_section *sec);
void cfg_proper_print(struct cfg_proper *pro);
#endif
