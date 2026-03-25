.syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

    .global OSStartHighRdy
    .global OSCtxSw
    .global OSIntCtxSw

    .extern OSTaskSwHook
    .extern OSRunning
    .extern OSTCBHighRdy

    .section .text
    .align 2


    .thumb_func
OSStartHighRdy:
    /* 1. 从 uCOS 变量中找到最高优先级任务的栈顶指针 */
    ldr     r0, =OSTCBHighRdy
    ldr     r1, [r0]        
    ldr     r2, [r1]        /* r2 = 当前栈顶地址 */

    /* 2. 按照我们刚才在 C 语言里的逆序，先把 R4-R11 弹出来 */
    ldmia   r2!, {r4-r11}

    /* 3. 把剩下的栈（还有R0-R3等硬件寄存器）转交给专用的 PSP 进程堆栈指针 */
    msr     psp, r2

    /* 4. 修改控制寄存器，让 CPU 以后使用 PSP 堆栈 */
    movs    r0, #2
    msr     control, r0
    isb                     /* 刷新指令流水线 */

    /* 5. 开启全局中断 */
    cpsie   i

    /* 6. 继续弹出剩下的基本寄存器 */
    pop     {r0-r3, r12, lr}

    /* 7. 关键一步！弹出我们刚才塞进去的 Task1 地址到 r1 */
    pop     {r1, r2}        /* r1 = PC (Task1地址), r2 = xPSR */
    
    /* 8. 扣动扳机，正式跳入任务！ */
    bx      r1

/* 任务级切换（当前仅单任务，保留空实现） */
    .thumb_func
OSCtxSw:
    BX      LR

/* 中断级切换（当前仅单任务，保留空实现） */
    .thumb_func
OSIntCtxSw:
    BX      LR