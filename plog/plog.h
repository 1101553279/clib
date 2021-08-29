#ifndef __PLOG_H__
#define __PLOG_H__

#include "btype.h"

/* key list */
#define PLOG_KEY    (0x1)
#define PLOG_RUN    (0x1<<1)
#define PLOG_CMD    (0x2<<1)
#define PLOG_TICK   (0x3<<1)
#define PLOG_CFG    (0x4<<1)

#define plog(mod, fmt, args...)\
    do{\
        if(plog_key_is(PLOG_KEY) && plog_key_is(PLOG_##mod))  \
            plog_out(#mod, fmt, __func__, __LINE__, ##args);\
    }while(0)

void plog_init(void);
void plog_init_append(void);
u8_t plog_key_is(u32_t mods);
void plog_out(char *mod, const char *fmt, const char *func, int line, ...);
void plog_uninit(void);
u16_t plog_dump(char *buff, u16_t len);

#endif
