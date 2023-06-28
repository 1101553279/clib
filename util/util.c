#include "util/util.h"
#include "util/errno.h"
#include "include/btype.h"
#include <ctype.h>
#include "log/log.h"

/*
|  set    key       val 	.....|
   ^	  ^			^
   |	  |			|
argv[0]	argv[1]	argv[2].......
    str -> must can be modified!
*/
int str_parse(char *str, char *argv[], const int nu)
{
    size_t len = 0;
	int argc = 0;
	int i = 0;
	int bflag = 1;	/* whether the previous character is space character */

    if(NULL==str || NULL==argv || 0==nu)
        return -1;

    len = strlen(str);
	for(i=0; i<len && argc<nu; i++)
	{
		/* event0: previous character is space character, and current character is not space character */
		if(bflag && ' '!=str[i])	
		{
			argv[argc++] = str+i;
			bflag = 0;
		}/* event1: previous character is not space character, and current character is space character */
		else if(!bflag && ' '==str[i])
		{
			str[i] = '\0';
			bflag = 1;
		}
	}

	return argc;
}
    
int argtable_parse(int argc, char *argv[], void *argtable, struct arg_end *end_arg, arg_dstr_t ds, const char *cmd)
{
    int nerrors = 0;

    if( 0 != arg_nullcheck(argtable))
    {
        arg_dstr_catf(ds, "%s: insufficient memory\r\n", cmd);
        return -1;
    }

    nerrors = arg_parse(argc, argv, argtable);
    if(nerrors > 0)
        arg_print_errors_ds(ds, end_arg, cmd);

    return nerrors;
}

#define MAX_LINE_LENGTH_BYTES		64
#define DEFAULT_LINE_LENGTH_BYTES	16

int hexdump_line(ulong addr, const void *data, uint width, uint count,
		 uint linelen, char *out, int size)
{
	/* linebuf as a union causes proper alignment */
	union linebuf {
		u64_t uq[MAX_LINE_LENGTH_BYTES/sizeof(u64_t) + 1];
		u32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(u32_t) + 1];
		u16_t us[MAX_LINE_LENGTH_BYTES/sizeof(u16_t) + 1];
		u8_t  uc[MAX_LINE_LENGTH_BYTES/sizeof(u8_t) + 1];
	} lb;
	uint thislinelen;
	int i;
	ulong x;

	if (linelen * width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	/*
	 * Check the size here so that we don't need to use snprintf(). This
	 * helps to reduce code size
	 */
	if (size < HEXDUMP_MAX_BUF_LENGTH(linelen * width))
		return -ENOSPC;

	thislinelen = linelen;
	out += sprintf(out, "%08lx:", addr);

	/* check for overflow condition */
	if (count < thislinelen)
		thislinelen = count;

	/* Copy from memory into linebuf and print hex values */
	for (i = 0; i < thislinelen; i++) {
		if (width == 4)
			x = lb.ui[i] = *(volatile u32_t *)data;
#if 0
		else if (MEM_SUPPORT_64BIT_DATA && width == 8)
			x = lb.uq[i] = *(volatile ulong *)data;
#endif
		else if (width == 2)
			x = lb.us[i] = *(volatile u16_t *)data;
		else
			x = lb.uc[i] = *(volatile u8_t *)data;
#if 0
		if (CONFIG_IS_ENABLED(USE_TINY_PRINTF))
			out += sprintf(out, " %x", (uint)x);
		else
#endif
			out += sprintf(out, " %0*lx", width * 2, x);
		data += width;
	}

	/* fill line with whitespace for nice ASCII print */
	for (i = 0; i < (linelen - thislinelen) * (width * 2 + 1); i++)
		*out++ = ' ';

	/* Print data in ASCII characters */
	for (i = 0; i < thislinelen * width; i++) {
		if (!isprint(lb.uc[i]) || lb.uc[i] >= 0x80)
			lb.uc[i] = '.';
	}
	lb.uc[i] = '\0';
	out += sprintf(out, "  %s", lb.uc);

	return thislinelen;
}
