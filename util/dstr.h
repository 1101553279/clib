#ifndef __DSTR_H__
#define __DSTR_H__

struct dstr {
    char *sdata;
    int slen;
    int ssize;
    unsigned int flag;
};

void dstr_init(struct dstr *ds, char *data, int ssize);
struct dstr *dstr_create(int size);
void dstr_destroy(struct dstr *ds);
void dstr_reset(struct dstr *ds);
struct dstr *dstr_cat(struct dstr *ds, const char *str);
struct dstr *dstr_catc(struct dstr *ds, char c);
struct dstr *dstr_catf(struct dstr *ds, const char *fmt, ...);
char *dstr_cstr(struct dstr *ds);
int dstr_len(struct dstr *ds);

void dstr_print(struct dstr *ds);

#endif
