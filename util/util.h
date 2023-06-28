#ifndef __UTIL_H__
#define __UTIL_H__
#include "include/btype.h"
#include <string.h>
#include <stdio.h>
#include "cmd/argtable3.h"
#include "util/blist.h"
#include "util/color.h"
#include <semaphore.h>


#define LOCK_DECLARE(name)		sem_t name
#define LOCK_INIT(name)			sem_init(name, 0, 1)
#define LOCK_ENTER(name)		sem_wait(name)
#define LOCK_TRY_ENTER(name)	sem_trywait(name)
#define LOCK_EXIT(name)			sem_post(name)

#define GET_MAX(x, y)   (((x) > (y)) ? (x) : (y))
#define GET_MIN(x, y)   (((x) < (y)) ? (x) : (y))

#define ARY_SIZE( arr )      (sizeof(arr)/sizeof((arr)[0]))
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))

/*
 * Maximum length of an output line is when width == 1
 *	9 for address,
 *	a space, two hex digits and an ASCII character for each byte
 *	2 spaces between the hex and ASCII
 *	\0 terminator
 */
#define HEXDUMP_MAX_BUF_LENGTH(bytes)	(9 + (bytes) * 4 + 3)

/**
 * hexdump_line() - Print out a single line of a hex dump
 *
 * @addr:	Starting address to display at start of line
 * @data:	pointer to data buffer
 * @width:	data value width.  May be 1, 2, or 4.
 * @count:	number of values to display
 * @linelen:	Number of values to print per line; specify 0 for default length
 * @out:	Output buffer to hold the dump
 * @size:	Size of output buffer in bytes
 * Return: number of bytes processed, if OK, -ENOSPC if buffer too small
 *
 */
int hexdump_line(ulong addr, const void *data, uint width, uint count,
		 uint linelen, char *out, int size);


int str_parse(char *str, char *argv[], const int nu);
// for debug
u16_t data_tostr(char *buff, u16_t len, u8_t *data, u8_t dlen);

int argtable_parse(int argc, char *argv[], void *argtable, struct arg_end *end_arg, arg_dstr_t ds, const char *cmd);

#endif
