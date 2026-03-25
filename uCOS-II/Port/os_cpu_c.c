#include "os_cpu.h"
#include "os_cfg.h"

/* 系统初始化函数，通常用于设置硬件相关的设置 */
void SystemInit(void) {*((volatile unsigned int *)0xE000ED88) |= (0xF << 20);}

/* 定义空的钩子函数 */
void OSInitHookBegin(void) {}
void OSInitHookEnd(void) {}
void OSTCBInitHook(void) {}
void OSTaskCreateHook(void) {}
void OSTaskIdleHook(void) {}
void OSTaskReturnHook(void) {}
void OSTaskSwHook(void) {}
void OSTaskDelHook(void) {}
void OSTaskStatHook(void) {}
void OSTimeTickHook(void) {}

OS_STK *OSTaskStkInit (void (*task)(void *p_arg), void *p_arg, OS_STK *ptos, INT16U opt)
{
    OS_STK *stk;
    (void)opt;
    stk = ptos;                    /* 获取堆栈顶指针 (已经指向数组末尾) */

    /* --- 模拟 Cortex-M 硬件自动入栈的寄存器 --- */
    *(--stk) = (INT32U)0x01000000L;/* xPSR: 必须设置 Thumb 状态位 */
    *(--stk) = (INT32U)task;       /* PC: 任务入口地址 (也就是 Task1 的地址) */
    *(--stk) = (INT32U)0xFFFFFFFEL;/* LR: 返回地址 (任务不该返回，设为错误值) */
    *(--stk) = (INT32U)0x12121212L;/* R12 */
    *(--stk) = (INT32U)0x03030303L;/* R3  */
    *(--stk) = (INT32U)0x02020202L;/* R2  */
    *(--stk) = (INT32U)0x01010101L;/* R1  */
    *(--stk) = (INT32U)p_arg;      /* R0: 传给任务的参数 p_arg */

    /* --- 模拟需要软件手动入栈的寄存器 --- */
    *(--stk) = (INT32U)0x11111111L;/* R11 */
    *(--stk) = (INT32U)0x10101010L;/* R10 */
    *(--stk) = (INT32U)0x09090909L;/* R9  */
    *(--stk) = (INT32U)0x08080808L;/* R8  */
    *(--stk) = (INT32U)0x07070707L;/* R7  */
    *(--stk) = (INT32U)0x06060606L;/* R6  */
    *(--stk) = (INT32U)0x05050505L;/* R5  */
    *(--stk) = (INT32U)0x04040404L;/* R4  */

    return (stk); /* 返回最终的栈顶地址给 TCB 控制块 */
}