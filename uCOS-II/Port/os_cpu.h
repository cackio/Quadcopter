//数据类型
typedef unsigned char BOOLEAN; // 定义布尔类型
typedef unsigned char INT8U; // 无符号8位整数
typedef signed char INT8S; // 有符号8位整数
typedef unsigned short INT16U; // 无符号16位整数
typedef signed short INT16S; // 有符号16位整数
typedef unsigned int INT32U; // 无符号32位整数
typedef signed int INT32S; // 有符号32位整数
typedef float FP32; // 单精度浮点数
typedef double FP64; // 双精度浮点数
typedef unsigned int OS_STK; // 每个栈条目为32位宽
typedef unsigned int OS_CPU_SR;// 定义CPU状态寄存器的大小（PSR=32位）
#define OS_ENTER_CRITICAL() do { } while (0)
#define OS_EXIT_CRITICAL() do { } while (0)
#define OS_TASK_SW() OSCtxSw()
void OSCtxSw (void);
void OSIntCtxSw (void);
void OSStartHighRdy (void);
void OS_CPU_SysTickInit (void);