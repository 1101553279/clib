#include "dstr.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>

#define DSTR_MIN_SIZE       10
#define DSTR_DFT_SIZE       512
#define DSTR_FLAG_ALLOC     (0x1<<0)

void dstr_init(struct dstr *ds, char *data, int ssize)
{
    if(ds && data && ssize>0)
    {
        ds->sdata = data;
        ds->slen = 0;
        ds->ssize = ssize;
        ds->flag = 0;
        ds->flag &= ~DSTR_FLAG_ALLOC;
    }

    return;
}

struct dstr *dstr_create(int size)
{
    struct dstr *ds;

    if(size < DSTR_MIN_SIZE)
        size = DSTR_DFT_SIZE;

    ds = malloc(sizeof(struct dstr));
    if(NULL != ds)
    {
        ds->sdata = malloc(size); 
        if(NULL == ds->sdata)
            goto err_sdata;

        ds->slen = 0;
        ds->ssize = size;
        ds->flag |= DSTR_FLAG_ALLOC;
    }

    return ds;

err_sdata:
    free(ds);
    return NULL;
}

void dstr_destroy(struct dstr *ds)
{
    if(ds)
    {
        if(ds->flag & DSTR_FLAG_ALLOC)
        {
            free(ds->sdata);
            ds->sdata = NULL;
            ds->slen = 0; 
            ds->ssize = 0;
            ds->flag = 0;
            free(ds);
        }
        else
        {
            ds->sdata = NULL;
            ds->slen = 0;
            ds->ssize = 0;
            ds->flag = 0;
        }
    }
}

void dstr_reset(struct dstr *ds)
{
    if(ds)
        ds->slen = 0;
}

struct dstr *dstr_cat(struct dstr *ds, const char *str)
{
    if(ds && str)
        ds->slen += snprintf(ds->sdata+ds->slen, ds->ssize-ds->slen, "%s", str);

    return ds;
}

struct dstr *dstr_catc(struct dstr *ds, char c)
{
    if(ds)
        ds->slen += snprintf(ds->sdata+ds->slen, ds->ssize-ds->slen, "%c", c);

    return ds;
}

struct dstr *dstr_catf(struct dstr *ds, const char *fmt, ...)
{
    va_list arglist;

    if(ds && fmt)
    {
        va_start(arglist, fmt);
        ds->slen += vsnprintf(ds->sdata+ds->slen, ds->ssize-ds->slen, fmt, arglist);
        va_end(arglist);
    }

    return ds;
}

char *dstr_cstr(struct dstr *ds)
{
    return (ds&&ds->slen>0)? ds->sdata: NULL;
}

int dstr_len(struct dstr *ds)
{
    return ds->slen<ds->ssize? ds->slen: ds->ssize;
}

void dstr_print(struct dstr *ds)
{
    if(ds)
    {
        printf("/****** ds summary ******/\n");
        if(ds->slen)
        {
            printf("sdata   : \n");
            printf("          %s\n", ds->sdata);
        }

        printf("slen    : %d\n", ds->slen);
        printf("ssize   : %d\n", ds->ssize);
        printf("flag    : %#x\n", ds->flag);
    }

}
    
#if 0
    struct dstr ds;
    char data[1024];
    dstr_init(&ds, data, sizeof(data));
    dstr_cat(&ds, "str");
    dstr_catf(&ds, " %s", "sirius company");
    dstr_catc(&ds, 'k');
    dstr_print(&ds);
    printf("%s\n", dstr_cstr(&ds));

    dstr_reset(&ds);
    dstr_print(&ds);
    if(NULL == dstr_cstr(&ds))
        printf("no data\n");
    else
        printf("%s", dstr_cstr(&ds));
     
    dstr_catf(&ds, " %s", "sirius company");

    dstr_print(&ds);

    dstr_destroy(&ds);
    dstr_print(&ds);
#endif
