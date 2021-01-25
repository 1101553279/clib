#ifndef __PRINT_H__
#define __PRINT_H__

#include "color.h"
#include <stdio.h>

#define log(fmt, args...)	do{ printf(fmt, ##args); }while(0)
#define logc(color, fmt, args...)  do{ printf(color fmt COLOR_FONT_NONE, ##args);} while(0)

#define log_red(fmt, args...)	do{ printf(COLOR_FONT_RED fmt COLOR_FONT_NONE, ##args);}while(0)
#define log_grn(fmt, args...)	do{ printf(COLOR_FONT_GREN fmt COLOR_FONT_NONE, ##args);}while(0)
#define log_yel(fmt, args...)	do{ printf(COLOR_FONT_YELL fmt COLOR_FONT_NONE, ##args);}while(0)
#define log_whi(fmt, args...)	do{ printf(COLOR_FONT_WHIT fmt COLOR_FONT_NONE, ##args);}while(0)


#define log_blk(fmt, args...)	do{ printf(PRINT_ATTR_BLNK fmt PRINT_ATTR_UBLK, ##args); }while(0)

#endif
