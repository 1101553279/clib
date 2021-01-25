#include "util.h"
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
