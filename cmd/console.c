#include "console.h"
#include "usrv.h"
#include <stdio.h>
#include "command.h"

#define CS_UPORT   3000

static int cs_callback(struct usrv *usrv, unsigned char *buff, unsigned int len)
{
    int ret = 0;
//    printf("%u:%s\r\n", len, (char *)buff);

    cmd_exe(USRV_CLIENT(usrv), buff, len);

#if 0    
    ret = usrv_client_reply(USRV_CLIENT(usrv), buff, len);
    if(ret < 0)
        printf("reply fail!\n");
    else
        printf("reply success!\n");
#endif
    return 0;
}

void cs_init(struct console* cs)
{
    int ret = 0;

    ret = usrv_init(&cs->usrv, CS_UPORT, cs_callback);
    if(0 == ret)
        printf("usrv init success: %u\r\n", CS_UPORT);
    else
        printf("usrv init fail!\r\n");
    return;
}
