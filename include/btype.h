#ifndef __BTYPE_H__
#define __BTYPE_H__


typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   int    u32_t;
typedef signed     int    s32_t;

#ifndef __cplusplus
typedef enum {FALSE = 0, TRUE = !FALSE} bool;
#endif

#endif
