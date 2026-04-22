#ifndef  OS_CPU_H
#define  OS_CPU_H

#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {                                    /* See Note #1.                                       */
#endif

#ifndef  OS_CPU_EXCEPT_STK_SIZE
#define  OS_CPU_EXCEPT_STK_SIZE      256u        /* Default exception stack size is 256 OS_STK entries */
#endif

#ifndef  __TARGET_FPU_SOFTVFP
#define  OS_CPU_ARM_FP_EN              1u
#else
#define  OS_CPU_ARM_FP_EN              0u
#endif

#ifndef CPU_CFG_KA_IPL_BOUNDARY
#error  "CPU_CFG_KA_IPL_BOUNDARY        not #define'd in 'app_cfg.h'    "   /* See Note # 1 & 2        */
#else
#if (CPU_CFG_KA_IPL_BOUNDARY == 0u)
#error  "CPU_CFG_KA_IPL_BOUNDARY        should be > 0 "
#endif
#endif

#ifndef CPU_CFG_NVIC_PRIO_BITS
#error  "CPU_CFG_NVIC_PRIO_BITS         not #define'd in 'app_cfg.h'    "   /* See Note # 3            */
#else
#if (CPU_CFG_KA_IPL_BOUNDARY >= (1u << CPU_CFG_NVIC_PRIO_BITS))
#error  "CPU_CFG_KA_IPL_BOUNDARY        should not be set to higher than max programable priority level "
#endif
#endif


typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */

typedef unsigned int   OS_STK;                   /* Each stack entry is 32-bit wide                    */
typedef unsigned int   OS_CPU_SR;                /* Define size of CPU status register (PSR = 32 bits) */


#define  OS_CRITICAL_METHOD   3u

#if OS_CRITICAL_METHOD == 3u
                                                 /* Save current BASEPRI priority lvl for exception... */
                                                 /* .. and set BASEPRI to CPU_CFG_KA_IPL_BOUNDARY      */
#define  OS_ENTER_CRITICAL()  do { cpu_sr = OS_CPU_SR_Save(CPU_CFG_KA_IPL_BOUNDARY << (8u - CPU_CFG_NVIC_PRIO_BITS));} while (0)
                                                 /* Restore CPU BASEPRI priority level.                */
#define  OS_EXIT_CRITICAL()   do { OS_CPU_SR_Restore(cpu_sr);} while (0)
#endif


/*
*********************************************************************************************************
*                                        Cortex-M Miscellaneous
*********************************************************************************************************
*/

#define  OS_STK_GROWTH        1u                 /* Stack grows from HIGH to LOW memory on ARM         */

#define  OS_TASK_SW()         OSCtxSw()


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

OS_CPU_EXT  OS_STK   OS_CPU_ExceptStk[OS_CPU_EXCEPT_STK_SIZE];
OS_CPU_EXT  OS_STK  *OS_CPU_ExceptStkBase;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

#if OS_CRITICAL_METHOD == 3u                     /* See OS_CPU_A.ASM                                   */
OS_CPU_SR  OS_CPU_SR_Save         (OS_CPU_SR  new_basepri);
void       OS_CPU_SR_Restore      (OS_CPU_SR  cpu_sr);
#endif

void       OSCtxSw                (void);
void       OSIntCtxSw             (void);
void       OSStartHighRdy         (void);

                                                 /* See OS_CPU_C.C                                     */
void       OS_CPU_SysTickInit     (INT32U     cnts);
void       OS_CPU_SysTickInitFreq (INT32U     cpu_freq);

void       OS_CPU_SysTickHandler  (void);
void       OS_CPU_PendSVHandler   (void);

#if (OS_CPU_ARM_FP_EN > 0u)
void       OS_CPU_FP_Reg_Push     (OS_STK    *stkPtr);
void       OS_CPU_FP_Reg_Pop      (OS_STK    *stkPtr);
#endif

/*
*********************************************************************************************************
*                                   EXTERNAL C LANGUAGE LINKAGE END
*********************************************************************************************************
*/

#ifdef __cplusplus
}                                                /* End of 'extern'al C lang linkage.                  */
#endif




#endif