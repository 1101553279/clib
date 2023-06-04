#ifndef __UTIL_H__
#define __UTIL_H__
#include "btype.h"
#include <string.h>
#include <stdio.h>
#include "argtable3.h"
#include "blist.h"
#include "log.h"
#include "color.h"
#include "util.h"

#define GET_MAX(x, y)   (((x) > (y)) ? (x) : (y))
#define GET_MIN(x, y)   (((x) < (y)) ? (x) : (y))

#define ARY_SIZE( arr )      (sizeof(arr)/sizeof((arr)[0]))



int str_parse(char *str, char *argv[], const int nu);
// for debug
u16_t data_tostr(char *buff, u16_t len, u8_t *data, u8_t dlen);

int argtable_parse(int argc, char *argv[], void *argtable, struct arg_end *end_arg, arg_dstr_t ds, const char *cmd);

#endif
