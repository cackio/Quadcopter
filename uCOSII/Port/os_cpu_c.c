#include "os_cpu.h"
#include "led.h"
// 系统初始化函数，通常⽤于设置硬件相关的设置
//void SystemInit(){}	// 与system_stm32f4xx.c冲突，暂时注释
// 定义空的钩子函数
void OSInitHookBegin(){}
void OSInitHookEnd(){}
void OSTCBInitHook(){}
void OSTaskCreateHook(){}
void OSTaskIdleHook(){}
void OSTaskReturnHook(){}
void OSTaskSwHook(){}
void OSTimeTickHook(void){
	static INT32U count = 0;
    if (++count >= 500) { // 假设时钟节拍是 1ms，则 500ms 翻转一次
        LED_Toggle();
        count = 0;
    }}
void OSTaskStatHook(){}
void OSTaskDelHook(){}

// 任务堆栈初始化函数，为新任务分配堆栈空间
OS_STK *OSTaskStkInit(void (*task)(void *p_arg), void *p_arg, OS_STK *ptos,
INT16U opt){
return ptos;
}
// 定义一个空的CPU异常栈基地址
//void OS_CPU_ExceptStkBase(){}
OS_STK *OS_CPU_ExceptStkBase;          // 全局变量定义
// 设置内核可⽤的优先级边界
void OS_KA_BASEPRI_Boundary(){}
	
	
	