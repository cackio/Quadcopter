#include "ucos_ii.h"
#include "led.h"

#define TASK1_STK_SIZE  128
OS_STK Task1Stk[TASK1_STK_SIZE];  // 任务的堆栈内存

void Task1(void *p_arg)
{
		(void)p_arg;
    
    LED_Init(); // 初始化硬件
    LED_On();   // 点亮小灯
    
    /* 任务必须是一个死循环，既然不切换，就在这原地踏步 */
    while (1) {
        // 灯亮着，代表任务正在运行！
    }
}

int main(void)
{
		OSInit(); // 1. 初始化内核
    
    // 2. 创建第一个任务 (注意堆栈是从高向低生长的，所以传入数组末尾地址)
    OSTaskCreate(Task1, 
                 (void *)0, 
                 &Task1Stk[TASK1_STK_SIZE - 1], 
                 10); // 优先级 10
                 
    OSStart(); // 3. 启动内核，CPU 将跳入 Task1

    while(1);  // 正常情况下，永远不会执行到这里
    return 0;
}