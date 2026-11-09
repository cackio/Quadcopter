#ifndef TEST_UCOS_H
#define TEST_UCOS_H
#include <stdint.h>
typedef uint8_t INT8U;
typedef uint32_t OS_STK;
typedef uint32_t OS_CPU_SR;
typedef struct { unsigned value; } OS_EVENT;
typedef struct { unsigned OSFree; } OS_STK_DATA;
#define OS_ERR_NONE 0
#define OS_TRUE 1
#define OS_TASK_OPT_STK_CHK 1
#define OS_TASK_OPT_STK_CLR 2
#define OS_ENTER_CRITICAL() ((void)cpu_sr)
#define OS_EXIT_CRITICAL() ((void)cpu_sr)
extern unsigned OSRunning;
OS_EVENT *OSSemCreate(unsigned);
unsigned OSSemPost(OS_EVENT *);
unsigned OSSemAccept(OS_EVENT *);
void OSSemPend(OS_EVENT *,unsigned,INT8U *);
void OSTimeDly(unsigned);
unsigned OSTaskCreateExt(void (*)(void *),void *,OS_STK *,unsigned,unsigned,OS_STK *,unsigned,void *,unsigned);
void OSTaskNameSet(unsigned,INT8U *,INT8U *);
unsigned OSTaskStkChk(unsigned,OS_STK_DATA *);
#endif
