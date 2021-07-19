#ifndef __COLOR_H__
#define __COLOR_H__


#define COLOR_FONT_NONE "\033[m"
#define COLOR_FONT_RED  "\033[31m"
#define COLOR_FONT_GREN "\033[32m"
#define COLOR_FONT_YELL "\033[33m"
#define COLOR_FONT_BLUE "\033[34m"
#define COLOR_FONT_PURP	"\033[35m"	/* 紫色 */
#define COLOR_FONT_CYAN	"\033[36m"	/* 青色 */
#define COLOR_FONT_WHIT "\033[37m"	/* 白色 */



#define COLOR_BACK_BLCK "\033[40m"
#define COLOR_BACK_RED  "\033[41m"
#define COLOR_BACK_GREN "\033[42m"
#define COLOR_BACK_YELL "\033[43m"
#define COLOR_BACK_BLUE "\033[44m"
#define COLOR_BACK_PURP	"\033[45m"
#define COLOR_BACK_CYAN	"\033[46m"
#define COLOR_BACK_WHIT	"\033[47m"

#define PRINT_ATTR_BLNK	"\033[5m"
#define PRINT_ATTR_UBLK	"\033[25m"


#if 0
/*输出属性设置*/
#define PRINT_ATTR_REC  printf("\033[0m");  //重新设置属性到缺省设置 
#define PRINT_ATTR_BOL  printf("\033[1m");  //设置粗体 
#define PRINT_ATTR_LIG  printf("\033[2m");  //设置一半亮度(模拟彩色显示器的颜色) 
#define PRINT_ATTR_LIN  printf("\033[4m");  //设置下划线(模拟彩色显示器的颜色) 
#define PRINT_ATTR_GLI  printf("\033[5m");  //设置闪烁 
#define PRINT_ATTR_REV  printf("\033[7m");  //设置反向图象 
#define PRINT_ATTR_THI  printf("\033[22m"); //设置一般密度 
#define PRINT_ATTR_ULIN  printf("\033[24m");//关闭下划线 
#define PRINT_ATTR_UGLI  printf("\033[25m");//关闭闪烁 
#define PRINT_ATTR_UREV  printf("\033[27m");//关闭反向图象
#endif
#endif
