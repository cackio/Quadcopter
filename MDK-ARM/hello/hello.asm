
========================================================================

** ELF Header Information

    File Name: hello\hello.axf

    Machine class: ELFCLASS32 (32-bit)
    Data encoding: ELFDATA2LSB (Little endian)
    Header version: EV_CURRENT (Current version)
    Operating System ABI: none
    ABI Version: 0
    File Type: ET_EXEC (Executable) (2)
    Machine: EM_ARM (ARM)

    Image Entry point: 0x08000195
    Flags: EF_ARM_HASENTRY + EF_ARM_ABI_FLOAT_HARD (0x05000402)

    ARM ELF revision: 5 (ABI version 2)

    Conforms to Hard float procedure-call standard

    Built with
    Component: Arm Compiler for Embedded 6.24 Tool: armasm [5f371400]
    Component: Arm Compiler for Embedded 6.24 Tool: armlink [5f371500]

    Header size: 52 bytes (0x34)
    Program header entry size: 32 bytes (0x20)
    Section header entry size: 40 bytes (0x28)

    Program header entries: 1
    Section header entries: 15

    Program header offset: 61924 (0x0000f1e4)
    Section header offset: 61956 (0x0000f204)

    Section header string table index: 14

========================================================================

** Program header #0 (PT_LOAD) [PF_X + PF_W + PF_R + PF_ARM_ENTRY]
    Size : 7760 bytes (6112 bytes in file)
    Virtual address: 0x08000000 (Alignment 8)


========================================================================

** Section #1 'ER_IROM1' (SHT_PROGBITS) [SHF_ALLOC + SHF_EXECINSTR]
    Size   : 6096 bytes (alignment 4)
    Address: 0x08000000

    $d.realdata
    RESET
    __Vectors
        0x08000000:    20000680    ...     DCD    536872576
        0x08000004:    08000261    a...    DCD    134218337
        0x08000008:    08001509    ....    DCD    134223113
        0x0800000c:    0800144d    M...    DCD    134222925
        0x08000010:    08001505    ....    DCD    134223109
        0x08000014:    08000459    Y...    DCD    134218841
        0x08000018:    080016ad    ....    DCD    134223533
        0x0800001c:    00000000    ....    DCD    0
        0x08000020:    00000000    ....    DCD    0
        0x08000024:    00000000    ....    DCD    0
        0x08000028:    00000000    ....    DCD    0
        0x0800002c:    0800157d    }...    DCD    134223229
        0x08000030:    0800045d    ]...    DCD    134218845
        0x08000034:    00000000    ....    DCD    0
        0x08000038:    08001579    y...    DCD    134223225
        0x0800003c:    080015d5    ....    DCD    134223317
        0x08000040:    0800027b    {...    DCD    134218363
        0x08000044:    0800027b    {...    DCD    134218363
        0x08000048:    0800027b    {...    DCD    134218363
        0x0800004c:    0800027b    {...    DCD    134218363
        0x08000050:    0800027b    {...    DCD    134218363
        0x08000054:    0800027b    {...    DCD    134218363
        0x08000058:    0800027b    {...    DCD    134218363
        0x0800005c:    0800027b    {...    DCD    134218363
        0x08000060:    0800027b    {...    DCD    134218363
        0x08000064:    0800027b    {...    DCD    134218363
        0x08000068:    0800027b    {...    DCD    134218363
        0x0800006c:    0800027b    {...    DCD    134218363
        0x08000070:    0800027b    {...    DCD    134218363
        0x08000074:    0800027b    {...    DCD    134218363
        0x08000078:    0800027b    {...    DCD    134218363
        0x0800007c:    0800027b    {...    DCD    134218363
        0x08000080:    0800027b    {...    DCD    134218363
        0x08000084:    0800027b    {...    DCD    134218363
        0x08000088:    0800027b    {...    DCD    134218363
        0x0800008c:    00000000    ....    DCD    0
        0x08000090:    00000000    ....    DCD    0
        0x08000094:    00000000    ....    DCD    0
        0x08000098:    00000000    ....    DCD    0
        0x0800009c:    0800027b    {...    DCD    134218363
        0x080000a0:    0800027b    {...    DCD    134218363
        0x080000a4:    0800027b    {...    DCD    134218363
        0x080000a8:    0800027b    {...    DCD    134218363
        0x080000ac:    0800027b    {...    DCD    134218363
        0x080000b0:    0800027b    {...    DCD    134218363
        0x080000b4:    0800027b    {...    DCD    134218363
        0x080000b8:    0800027b    {...    DCD    134218363
        0x080000bc:    0800027b    {...    DCD    134218363
        0x080000c0:    0800027b    {...    DCD    134218363
        0x080000c4:    0800027b    {...    DCD    134218363
        0x080000c8:    0800027b    {...    DCD    134218363
        0x080000cc:    0800027b    {...    DCD    134218363
        0x080000d0:    0800027b    {...    DCD    134218363
        0x080000d4:    0800027b    {...    DCD    134218363
        0x080000d8:    0800027b    {...    DCD    134218363
        0x080000dc:    00000000    ....    DCD    0
        0x080000e0:    0800027b    {...    DCD    134218363
        0x080000e4:    0800027b    {...    DCD    134218363
        0x080000e8:    0800027b    {...    DCD    134218363
        0x080000ec:    00000000    ....    DCD    0
        0x080000f0:    00000000    ....    DCD    0
        0x080000f4:    00000000    ....    DCD    0
        0x080000f8:    00000000    ....    DCD    0
        0x080000fc:    0800027b    {...    DCD    134218363
        0x08000100:    00000000    ....    DCD    0
        0x08000104:    0800027b    {...    DCD    134218363
        0x08000108:    0800027b    {...    DCD    134218363
        0x0800010c:    0800027b    {...    DCD    134218363
        0x08000110:    00000000    ....    DCD    0
        0x08000114:    00000000    ....    DCD    0
        0x08000118:    00000000    ....    DCD    0
        0x0800011c:    00000000    ....    DCD    0
        0x08000120:    0800027b    {...    DCD    134218363
        0x08000124:    0800027b    {...    DCD    134218363
        0x08000128:    0800027b    {...    DCD    134218363
        0x0800012c:    0800027b    {...    DCD    134218363
        0x08000130:    0800027b    {...    DCD    134218363
        0x08000134:    00000000    ....    DCD    0
        0x08000138:    00000000    ....    DCD    0
        0x0800013c:    00000000    ....    DCD    0
        0x08000140:    00000000    ....    DCD    0
        0x08000144:    00000000    ....    DCD    0
        0x08000148:    00000000    ....    DCD    0
        0x0800014c:    0800027b    {...    DCD    134218363
        0x08000150:    0800027b    {...    DCD    134218363
        0x08000154:    0800027b    {...    DCD    134218363
        0x08000158:    0800027b    {...    DCD    134218363
        0x0800015c:    0800027b    {...    DCD    134218363
        0x08000160:    0800027b    {...    DCD    134218363
        0x08000164:    0800027b    {...    DCD    134218363
        0x08000168:    00000000    ....    DCD    0
        0x0800016c:    00000000    ....    DCD    0
        0x08000170:    00000000    ....    DCD    0
        0x08000174:    00000000    ....    DCD    0
        0x08000178:    00000000    ....    DCD    0
        0x0800017c:    00000000    ....    DCD    0
        0x08000180:    00000000    ....    DCD    0
        0x08000184:    0800027b    {...    DCD    134218363
        0x08000188:    00000000    ....    DCD    0
        0x0800018c:    00000000    ....    DCD    0
        0x08000190:    0800027b    {...    DCD    134218363
    $t
    !!!main
    __Vectors_End
    __main
        0x08000194:    f000f802    ....    BL       __scatterload ; 0x800019c
        0x08000198:    f000f852    ..R.    BL       __rt_entry ; 0x8000240
    !!!scatter
    __scatterload
    __scatterload_rt2
    __scatterload_rt2_thumb_only
        0x0800019c:    a614        ..      ADR      r6,{pc}+0x54 ; 0x80001f0
        0x0800019e:    e8960c00    ....    LDM      r6,{r10,r11}
        0x080001a2:    44b2        .D      ADD      r10,r10,r6
        0x080001a4:    44b3        .D      ADD      r11,r11,r6
    __scatterload_loop
        0x080001a6:    45da        .E      CMP      r10,r11
        0x080001a8:    d101        ..      BNE      0x80001ae ; __scatterload_loop + 8
        0x080001aa:    f000f849    ..I.    BL       __rt_entry ; 0x8000240
        0x080001ae:    e8ba000f    ....    LDM      r10!,{r0-r3}
        0x080001b2:    a60f        ..      ADR      r6,{pc}+0x3e ; 0x80001f0
        0x080001b4:    e8960030    ..0.    LDM      r6,{r4,r5}
        0x080001b8:    eba50504    ....    SUB      r5,r5,r4
        0x080001bc:    eb040706    ....    ADD      r7,r4,r6
        0x080001c0:    ebaa0407    ....    SUB      r4,r10,r7
        0x080001c4:    f1a70701    ....    SUB      r7,r7,#1
        0x080001c8:    ea4f1515    O...    LSR      r5,r5,#4
        0x080001cc:    ea4f1434    O.4.    ROR      r4,r4,#4
        0x080001d0:    f1a40401    ....    SUB      r4,r4,#1
        0x080001d4:    42ac        .B      CMP      r4,r5
        0x080001d6:    d301        ..      BCC      0x80001dc ; __scatterload_loop + 54
        0x080001d8:    f000f832    ..2.    BL       __rt_entry ; 0x8000240
        0x080001dc:    f0130f01    ....    TST      r3,#1
        0x080001e0:    bf18        ..      IT       NE
        0x080001e2:    1afb        ..      SUBNE    r3,r7,r3
        0x080001e4:    f0430301    C...    ORR      r3,r3,#1
        0x080001e8:    f000f801    ....    BL       0x80001ee ; __scatterload_loop + 72
        0x080001ec:    e7db        ..      B        __scatterload_loop ; 0x80001a6
        0x080001ee:    4718        .G      BX       r3
    $d
        0x080001f0:    000015c0    ....    DCD    5568
        0x080001f4:    000015e0    ....    DCD    5600
    $t
    !!handler_copy
    __scatterload_copy
        0x080001f8:    3a10        .:      SUBS     r2,r2,#0x10
        0x080001fa:    bf24        $.      ITT      CS
        0x080001fc:    c878        x.      LDMCS    r0!,{r3-r6}
        0x080001fe:    c178        x.      STMCS    r1!,{r3-r6}
        0x08000200:    d8fa        ..      BHI      __scatterload_copy ; 0x80001f8
        0x08000202:    0752        R.      LSLS     r2,r2,#29
        0x08000204:    bf24        $.      ITT      CS
        0x08000206:    c830        0.      LDMCS    r0!,{r4,r5}
        0x08000208:    c130        0.      STMCS    r1!,{r4,r5}
        0x0800020a:    bf44        D.      ITT      MI
        0x0800020c:    6804        .h      LDRMI    r4,[r0,#0]
        0x0800020e:    600c        .`      STRMI    r4,[r1,#0]
        0x08000210:    4770        pG      BX       lr
        0x08000212:    0000        ..      MOVS     r0,r0
    !!handler_null
    __scatterload_null
        0x08000214:    4770        pG      BX       lr
        0x08000216:    0000        ..      MOVS     r0,r0
    !!handler_zi
    __scatterload_zeroinit
        0x08000218:    2300        .#      MOVS     r3,#0
        0x0800021a:    2400        .$      MOVS     r4,#0
        0x0800021c:    2500        .%      MOVS     r5,#0
        0x0800021e:    2600        .&      MOVS     r6,#0
        0x08000220:    3a10        .:      SUBS     r2,r2,#0x10
        0x08000222:    bf28        (.      IT       CS
        0x08000224:    c178        x.      STMCS    r1!,{r3-r6}
        0x08000226:    d8fb        ..      BHI      0x8000220 ; __scatterload_zeroinit + 8
        0x08000228:    0752        R.      LSLS     r2,r2,#29
        0x0800022a:    bf28        (.      IT       CS
        0x0800022c:    c130        0.      STMCS    r1!,{r4,r5}
        0x0800022e:    bf48        H.      IT       MI
        0x08000230:    600b        .`      STRMI    r3,[r1,#0]
        0x08000232:    4770        pG      BX       lr
    .ARM.Collect$$libinit$$00000000
    __rt_lib_init
        0x08000234:    b51f        ..      PUSH     {r0-r4,lr}
    .ARM.Collect$$libinit$$00000001
    __rt_lib_init_fp_1
        0x08000236:    f001faa6    ....    BL       _fp_init ; 0x8001786
    .ARM.Collect$$libinit$$00000004
    .ARM.Collect$$libinit$$00000006
    .ARM.Collect$$libinit$$0000000C
    .ARM.Collect$$libinit$$0000000E
    .ARM.Collect$$libinit$$00000010
    .ARM.Collect$$libinit$$00000013
    .ARM.Collect$$libinit$$00000015
    .ARM.Collect$$libinit$$00000017
    .ARM.Collect$$libinit$$00000019
    .ARM.Collect$$libinit$$0000001B
    .ARM.Collect$$libinit$$0000001D
    .ARM.Collect$$libinit$$0000001F
    .ARM.Collect$$libinit$$00000021
    .ARM.Collect$$libinit$$00000023
    .ARM.Collect$$libinit$$00000025
    .ARM.Collect$$libinit$$00000027
    .ARM.Collect$$libinit$$0000002E
    .ARM.Collect$$libinit$$00000030
    .ARM.Collect$$libinit$$00000032
    .ARM.Collect$$libinit$$00000034
    .ARM.Collect$$libinit$$00000035
    __rt_lib_init_alloca_1
    __rt_lib_init_argv_1
    __rt_lib_init_atexit_1
    __rt_lib_init_clock_1
    __rt_lib_init_cpp_1
    __rt_lib_init_exceptions_1
    __rt_lib_init_fp_trap_1
    __rt_lib_init_getenv_1
    __rt_lib_init_heap_1
    __rt_lib_init_lc_collate_1
    __rt_lib_init_lc_ctype_1
    __rt_lib_init_lc_monetary_1
    __rt_lib_init_lc_numeric_1
    __rt_lib_init_lc_time_1
    __rt_lib_init_preinit_1
    __rt_lib_init_rand_1
    __rt_lib_init_relocate_pie_1
    __rt_lib_init_return
    __rt_lib_init_signal_1
    __rt_lib_init_stdio_1
    __rt_lib_init_user_alloc_1
        0x0800023a:    bd1f        ..      POP      {r0-r4,pc}
    .ARM.Collect$$libshutdown$$00000000
    __rt_lib_shutdown
        0x0800023c:    b510        ..      PUSH     {r4,lr}
    .ARM.Collect$$libshutdown$$00000002
    .ARM.Collect$$libshutdown$$00000004
    .ARM.Collect$$libshutdown$$00000007
    .ARM.Collect$$libshutdown$$0000000A
    .ARM.Collect$$libshutdown$$0000000C
    .ARM.Collect$$libshutdown$$0000000F
    .ARM.Collect$$libshutdown$$00000010
    __rt_lib_shutdown_cpp_1
    __rt_lib_shutdown_fp_trap_1
    __rt_lib_shutdown_heap_1
    __rt_lib_shutdown_return
    __rt_lib_shutdown_signal_1
    __rt_lib_shutdown_stdio_1
    __rt_lib_shutdown_user_alloc_1
        0x0800023e:    bd10        ..      POP      {r4,pc}
    .ARM.Collect$$rtentry$$00000000
    .ARM.Collect$$rtentry$$00000002
    .ARM.Collect$$rtentry$$00000004
    __rt_entry
    __rt_entry_presh_1
    __rt_entry_sh
        0x08000240:    f000f8d0    ....    BL       __user_setup_stackheap ; 0x80003e4
        0x08000244:    4611        .F      MOV      r1,r2
    .ARM.Collect$$rtentry$$00000009
    .ARM.Collect$$rtentry$$0000000A
    __rt_entry_li
    __rt_entry_postsh_1
        0x08000246:    f7fffff5    ....    BL       __rt_lib_init ; 0x8000234
    .ARM.Collect$$rtentry$$0000000C
    .ARM.Collect$$rtentry$$0000000D
    __rt_entry_main
    __rt_entry_postli_1
        0x0800024a:    f001fa79    ..y.    BL       main ; 0x8001740
        0x0800024e:    f000f8ee    ....    BL       exit ; 0x800042e
    .ARM.Collect$$rtexit$$00000000
    __rt_exit
        0x08000252:    b403        ..      PUSH     {r0,r1}
    .ARM.Collect$$rtexit$$00000002
    .ARM.Collect$$rtexit$$00000003
    __rt_exit_ls
    __rt_exit_prels_1
        0x08000254:    f7fffff2    ....    BL       __rt_lib_shutdown ; 0x800023c
    .ARM.Collect$$rtexit$$00000004
    __rt_exit_exit
        0x08000258:    bc03        ..      POP      {r0,r1}
        0x0800025a:    f000f8f5    ....    BL       _sys_exit ; 0x8000448
        0x0800025e:    0000        ..      MOVS     r0,r0
    .text
    $v0
    Reset_Handler
        0x08000260:    4809        .H      LDR      r0,[pc,#36] ; [0x8000288] = 0x8001699
        0x08000262:    4780        .G      BLX      r0
        0x08000264:    4809        .H      LDR      r0,[pc,#36] ; [0x800028c] = 0x8000195
        0x08000266:    4700        .G      BX       r0
        0x08000268:    e7fe        ..      B        0x8000268 ; Reset_Handler + 8
        0x0800026a:    e7fe        ..      B        0x800026a ; Reset_Handler + 10
        0x0800026c:    e7fe        ..      B        0x800026c ; Reset_Handler + 12
        0x0800026e:    e7fe        ..      B        0x800026e ; Reset_Handler + 14
        0x08000270:    e7fe        ..      B        0x8000270 ; Reset_Handler + 16
        0x08000272:    e7fe        ..      B        0x8000272 ; Reset_Handler + 18
        0x08000274:    e7fe        ..      B        0x8000274 ; Reset_Handler + 20
        0x08000276:    e7fe        ..      B        0x8000276 ; Reset_Handler + 22
        0x08000278:    e7fe        ..      B        0x8000278 ; Reset_Handler + 24
    ADC_IRQHandler
    DMA1_Stream0_IRQHandler
    DMA1_Stream1_IRQHandler
    DMA1_Stream2_IRQHandler
    DMA1_Stream3_IRQHandler
    DMA1_Stream4_IRQHandler
    DMA1_Stream5_IRQHandler
    DMA1_Stream6_IRQHandler
    DMA1_Stream7_IRQHandler
    DMA2_Stream0_IRQHandler
    DMA2_Stream1_IRQHandler
    DMA2_Stream2_IRQHandler
    DMA2_Stream3_IRQHandler
    DMA2_Stream4_IRQHandler
    DMA2_Stream5_IRQHandler
    DMA2_Stream6_IRQHandler
    DMA2_Stream7_IRQHandler
    EXTI0_IRQHandler
    EXTI15_10_IRQHandler
    EXTI1_IRQHandler
    EXTI2_IRQHandler
    EXTI3_IRQHandler
    EXTI4_IRQHandler
    EXTI9_5_IRQHandler
    FLASH_IRQHandler
    FPU_IRQHandler
    I2C1_ER_IRQHandler
    I2C1_EV_IRQHandler
    I2C2_ER_IRQHandler
    I2C2_EV_IRQHandler
    I2C3_ER_IRQHandler
    I2C3_EV_IRQHandler
    OTG_FS_IRQHandler
    OTG_FS_WKUP_IRQHandler
    PVD_IRQHandler
    RCC_IRQHandler
    RTC_Alarm_IRQHandler
    RTC_WKUP_IRQHandler
    SDIO_IRQHandler
    SPI1_IRQHandler
    SPI2_IRQHandler
    SPI3_IRQHandler
    SPI4_IRQHandler
    TAMP_STAMP_IRQHandler
    TIM1_BRK_TIM9_IRQHandler
    TIM1_CC_IRQHandler
    TIM1_TRG_COM_TIM11_IRQHandler
    TIM1_UP_TIM10_IRQHandler
    TIM2_IRQHandler
    TIM3_IRQHandler
    TIM4_IRQHandler
    TIM5_IRQHandler
    USART1_IRQHandler
    USART2_IRQHandler
    USART6_IRQHandler
    WWDG_IRQHandler
        0x0800027a:    e7fe        ..      B        ADC_IRQHandler ; 0x800027a
    __user_initial_stackheap
        0x0800027c:    4804        .H      LDR      r0,[pc,#16] ; [0x8000290] = 0x20000080
        0x0800027e:    4905        .I      LDR      r1,[pc,#20] ; [0x8000294] = 0x20000680
        0x08000280:    4a05        .J      LDR      r2,[pc,#20] ; [0x8000298] = 0x20000280
        0x08000282:    4b06        .K      LDR      r3,[pc,#24] ; [0x800029c] = 0x20000280
        0x08000284:    4770        pG      BX       lr
    $d
        0x08000286:    0000        ..      DCW    0
        0x08000288:    08001699    ....    DCD    134223513
        0x0800028c:    08000195    ....    DCD    134218133
        0x08000290:    20000080    ...     DCD    536871040
        0x08000294:    20000680    ...     DCD    536872576
        0x08000298:    20000280    ...     DCD    536871552
        0x0800029c:    20000280    ...     DCD    536871552
    $t
    .text
    __aeabi_uldivmod
    _ll_udiv
        0x080002a0:    ea530c02    S...    ORRS     r12,r3,r2
        0x080002a4:    f0008069    ..i.    BEQ.W    0x800037a ; __aeabi_uldivmod + 218
        0x080002a8:    e92d4bf0    -..K    PUSH     {r4-r9,r11,lr}
        0x080002ac:    f04f0600    O...    MOV      r6,#0
        0x080002b0:    2b00        .+      CMP      r3,#0
        0x080002b2:    bf1f        ..      ITTTT    NE
        0x080002b4:    fab3f583    ....    CLZNE    r5,r3
        0x080002b8:    fa03f405    ....    LSLNE    r4,r3,r5
        0x080002bc:    fa24f605    $...    LSRNE    r6,r4,r5
        0x080002c0:    405e        ^@      EORNE    r6,r6,r3
        0x080002c2:    bf12        ..      ITEE     NE
        0x080002c4:    4316        .C      ORRNE    r6,r6,r2
        0x080002c6:    fab2f582    ....    CLZEQ    r5,r2
        0x080002ca:    fa02f405    ....    LSLEQ    r4,r2,r5
        0x080002ce:    f1c50520    .. .    RSB      r5,r5,#0x20
        0x080002d2:    bf1e        ..      ITTT     NE
        0x080002d4:    fa22fc05    "...    LSRNE    r12,r2,r5
        0x080002d8:    ea44040c    D...    ORRNE    r4,r4,r12
        0x080002dc:    3520         5      ADDNE    r5,r5,#0x20
        0x080002de:    ea564c04    V..L    ORRS     r12,r6,r4,LSL #16
        0x080002e2:    ea4f4414    O..D    LSR      r4,r4,#16
        0x080002e6:    bf18        ..      IT       NE
        0x080002e8:    1c64        d.      ADDNE    r4,r4,#1
        0x080002ea:    f04f0800    O...    MOV      r8,#0
        0x080002ee:    f04f0900    O...    MOV      r9,#0
        0x080002f2:    4290        .B      CMP      r0,r2
        0x080002f4:    eb710c03    q...    SBCS     r12,r1,r3
        0x080002f8:    d339        9.      BCC      0x800036e ; __aeabi_uldivmod + 206
        0x080002fa:    2900        .)      CMP      r1,#0
        0x080002fc:    bf19        ..      ITTEE    NE
        0x080002fe:    fab1f781    ....    CLZNE    r7,r1
        0x08000302:    fa01f607    ....    LSLNE    r6,r1,r7
        0x08000306:    fab0f780    ....    CLZEQ    r7,r0
        0x0800030a:    fa00f607    ....    LSLEQ    r6,r0,r7
        0x0800030e:    f1c70720    .. .    RSB      r7,r7,#0x20
        0x08000312:    bf1e        ..      ITTT     NE
        0x08000314:    fa20fc07     ...    LSRNE    r12,r0,r7
        0x08000318:    ea46060c    F...    ORRNE    r6,r6,r12
        0x0800031c:    3720         7      ADDNE    r7,r7,#0x20
        0x0800031e:    fbb6fcf4    ....    UDIV     r12,r6,r4
        0x08000322:    eba70705    ....    SUB      r7,r7,r5
        0x08000326:    3f10        .?      SUBS     r7,r7,#0x10
        0x08000328:    f0070b1f    ....    AND      r11,r7,#0x1f
        0x0800032c:    f1cb0620    .. .    RSB      r6,r11,#0x20
        0x08000330:    fa2cf606    ,...    LSR      r6,r12,r6
        0x08000334:    fa0cfb0b    ....    LSL      r11,r12,r11
        0x08000338:    bf44        D.      ITT      MI
        0x0800033a:    46b3        .F      MOVMI    r11,r6
        0x0800033c:    2600        .&      MOVMI    r6,#0
        0x0800033e:    2f20         /      CMP      r7,#0x20
        0x08000340:    bfa4        ..      ITT      GE
        0x08000342:    465e        ^F      MOVGE    r6,r11
        0x08000344:    f04f0b00    O...    MOVGE    r11,#0
        0x08000348:    ea5b0c06    [...    ORRS     r12,r11,r6
        0x0800034c:    bf08        ..      IT       EQ
        0x0800034e:    f04f0b01    O...    MOVEQ    r11,#1
        0x08000352:    eb19090b    ....    ADDS     r9,r9,r11
        0x08000356:    eb480806    H...    ADC      r8,r8,r6
        0x0800035a:    fbab7c02    ...|    UMULL    r7,r12,r11,r2
        0x0800035e:    fb06cc02    ....    MLA      r12,r6,r2,r12
        0x08000362:    fb0bcc03    ....    MLA      r12,r11,r3,r12
        0x08000366:    1bc0        ..      SUBS     r0,r0,r7
        0x08000368:    eb71010c    q...    SBCS     r1,r1,r12
        0x0800036c:    e7c1        ..      B        0x80002f2 ; __aeabi_uldivmod + 82
        0x0800036e:    460b        .F      MOV      r3,r1
        0x08000370:    4602        .F      MOV      r2,r0
        0x08000372:    4641        AF      MOV      r1,r8
        0x08000374:    4648        HF      MOV      r0,r9
        0x08000376:    e8bd8bf0    ....    POP      {r4-r9,r11,pc}
        0x0800037a:    b513        ..      PUSH     {r0,r1,r4,lr}
        0x0800037c:    4308        .C      ORRS     r0,r0,r1
        0x0800037e:    bf18        ..      IT       NE
        0x08000380:    f04f30ff    O..0    MOVNE    r0,#0xffffffff
        0x08000384:    4601        .F      MOV      r1,r0
        0x08000386:    f3af8000    ....    NOP.W    
        0x0800038a:    e8bd401c    ...@    POP      {r2-r4,lr}
        0x0800038e:    4770        pG      BX       lr
    .text
    __aeabi_memclr4
    __aeabi_memclr8
    __rt_memclr_w
        0x08000390:    f04f0200    O...    MOV      r2,#0
    _memset_w
        0x08000394:    b500        ..      PUSH     {lr}
        0x08000396:    4613        .F      MOV      r3,r2
        0x08000398:    4694        .F      MOV      r12,r2
        0x0800039a:    4696        .F      MOV      lr,r2
        0x0800039c:    3920         9      SUBS     r1,r1,#0x20
        0x0800039e:    bf22        ".      ITTT     CS
        0x080003a0:    e8a0500c    ...P    STMCS    r0!,{r2,r3,r12,lr}
        0x080003a4:    e8a0500c    ...P    STMCS    r0!,{r2,r3,r12,lr}
        0x080003a8:    f1b10120    .. .    SUBSCS   r1,r1,#0x20
        0x080003ac:    f4bfaff7    ....    BCS.W    0x800039e ; _memset_w + 10
        0x080003b0:    0709        ..      LSLS     r1,r1,#28
        0x080003b2:    bf28        (.      IT       CS
        0x080003b4:    e8a0500c    ...P    STMCS    r0!,{r2,r3,r12,lr}
        0x080003b8:    bf48        H.      IT       MI
        0x080003ba:    c00c        ..      STMMI    r0!,{r2,r3}
        0x080003bc:    f85deb04    ]...    POP      {lr}
        0x080003c0:    0089        ..      LSLS     r1,r1,#2
        0x080003c2:    bf28        (.      IT       CS
        0x080003c4:    f8402b04    @..+    STRCS    r2,[r0],#4
        0x080003c8:    bf08        ..      IT       EQ
        0x080003ca:    4770        pG      BXEQ     lr
        0x080003cc:    bf48        H.      IT       MI
        0x080003ce:    f8202b02     ..+    STRHMI   r2,[r0],#2
        0x080003d2:    f0114f80    ...O    TST      r1,#0x40000000
        0x080003d6:    bf18        ..      IT       NE
        0x080003d8:    f8002b01    ...+    STRBNE   r2,[r0],#1
        0x080003dc:    4770        pG      BX       lr
    .text
    __use_two_region_memory
        0x080003de:    4770        pG      BX       lr
    __rt_heap_escrow$2region
        0x080003e0:    4770        pG      BX       lr
    __rt_heap_expand$2region
        0x080003e2:    4770        pG      BX       lr
    .text
    __user_setup_stackheap
        0x080003e4:    4675        uF      MOV      r5,lr
        0x080003e6:    f000f82b    ..+.    BL       __user_libspace ; 0x8000440
        0x080003ea:    46ae        .F      MOV      lr,r5
        0x080003ec:    0005        ..      MOVS     r5,r0
        0x080003ee:    4669        iF      MOV      r1,sp
        0x080003f0:    4653        SF      MOV      r3,r10
        0x080003f2:    f0200007     ...    BIC      r0,r0,#7
        0x080003f6:    4685        .F      MOV      sp,r0
        0x080003f8:    b018        ..      ADD      sp,sp,#0x60
        0x080003fa:    b520         .      PUSH     {r5,lr}
        0x080003fc:    f7ffff3e    ..>.    BL       __user_initial_stackheap ; 0x800027c
        0x08000400:    e8bd4020    .. @    POP      {r5,lr}
        0x08000404:    f04f0600    O...    MOV      r6,#0
        0x08000408:    f04f0700    O...    MOV      r7,#0
        0x0800040c:    f04f0800    O...    MOV      r8,#0
        0x08000410:    f04f0b00    O...    MOV      r11,#0
        0x08000414:    46ac        .F      MOV      r12,r5
        0x08000416:    e8ac09c0    ....    STM      r12!,{r6-r8,r11}
        0x0800041a:    e8ac09c0    ....    STM      r12!,{r6-r8,r11}
        0x0800041e:    e8ac09c0    ....    STM      r12!,{r6-r8,r11}
        0x08000422:    e8ac09c0    ....    STM      r12!,{r6-r8,r11}
        0x08000426:    f0210107    !...    BIC      r1,r1,#7
        0x0800042a:    468d        .F      MOV      sp,r1
        0x0800042c:    4770        pG      BX       lr
    .text
    exit
        0x0800042e:    b510        ..      PUSH     {r4,lr}
        0x08000430:    4604        .F      MOV      r4,r0
        0x08000432:    f3af8000    ....    NOP.W    
        0x08000436:    4620         F      MOV      r0,r4
        0x08000438:    e8bd4010    ...@    POP      {r4,lr}
        0x0800043c:    f7ffbf09    ....    B.W      __rt_exit ; 0x8000252
    .text
    __user_libspace
    __user_perproc_libspace
    __user_perthread_libspace
        0x08000440:    4800        .H      LDR      r0,[pc,#0] ; [0x8000444] = 0x20000010
        0x08000442:    4770        pG      BX       lr
    $d
        0x08000444:    20000010    ...     DCD    536870928
    $t
    .text
    _sys_exit
        0x08000448:    4901        .I      LDR      r1,[pc,#4] ; [0x8000450] = 0x20026
        0x0800044a:    2018        .       MOVS     r0,#0x18
        0x0800044c:    beab        ..      BKPT     #0xab
        0x0800044e:    e7fe        ..      B        0x800044e ; _sys_exit + 6
    $d
        0x08000450:    00020026    &...    DCD    131110
    $t
    .text
    __I$use$semihosting
    __use_no_semihosting_swi
        0x08000454:    4770        pG      BX       lr
    .text
    __semihosting_library_function
        0x08000456:    0000        ..      MOVS     r0,r0
    BusFault_Handler
        0x08000458:    e7ff        ..      B        0x800045a ; BusFault_Handler + 2
        0x0800045a:    e7fe        ..      B        0x800045a ; BusFault_Handler + 2
    DebugMon_Handler
        0x0800045c:    4770        pG      BX       lr
        0x0800045e:    0000        ..      MOVS     r0,r0
    Error_Handler
        0x08000460:    b672        r.      CPSID    i
        0x08000462:    e7ff        ..      B        0x8000464 ; Error_Handler + 4
        0x08000464:    e7fe        ..      B        0x8000464 ; Error_Handler + 4
        0x08000466:    0000        ..      MOVS     r0,r0
    HAL_Delay
        0x08000468:    b580        ..      PUSH     {r7,lr}
        0x0800046a:    b084        ..      SUB      sp,sp,#0x10
        0x0800046c:    9003        ..      STR      r0,[sp,#0xc]
        0x0800046e:    f000f9e7    ....    BL       HAL_GetTick ; 0x8000840
        0x08000472:    9002        ..      STR      r0,[sp,#8]
        0x08000474:    9803        ..      LDR      r0,[sp,#0xc]
        0x08000476:    9001        ..      STR      r0,[sp,#4]
        0x08000478:    9801        ..      LDR      r0,[sp,#4]
        0x0800047a:    3001        .0      ADDS     r0,#1
        0x0800047c:    b148        H.      CBZ      r0,0x8000492 ; HAL_Delay + 42
        0x0800047e:    e7ff        ..      B        0x8000480 ; HAL_Delay + 24
        0x08000480:    f2400004    @...    MOVW     r0,#4
        0x08000484:    f2c20000    ....    MOVT     r0,#0x2000
        0x08000488:    7801        .x      LDRB     r1,[r0,#0]
        0x0800048a:    9801        ..      LDR      r0,[sp,#4]
        0x0800048c:    4408        .D      ADD      r0,r0,r1
        0x0800048e:    9001        ..      STR      r0,[sp,#4]
        0x08000490:    e7ff        ..      B        0x8000492 ; HAL_Delay + 42
        0x08000492:    e7ff        ..      B        0x8000494 ; HAL_Delay + 44
        0x08000494:    f000f9d4    ....    BL       HAL_GetTick ; 0x8000840
        0x08000498:    9902        ..      LDR      r1,[sp,#8]
        0x0800049a:    1a40        @.      SUBS     r0,r0,r1
        0x0800049c:    9901        ..      LDR      r1,[sp,#4]
        0x0800049e:    4288        .B      CMP      r0,r1
        0x080004a0:    d201        ..      BCS      0x80004a6 ; HAL_Delay + 62
        0x080004a2:    e7ff        ..      B        0x80004a4 ; HAL_Delay + 60
        0x080004a4:    e7f6        ..      B        0x8000494 ; HAL_Delay + 44
        0x080004a6:    b004        ..      ADD      sp,sp,#0x10
        0x080004a8:    bd80        ..      POP      {r7,pc}
        0x080004aa:    0000        ..      MOVS     r0,r0
    HAL_GPIO_Init
        0x080004ac:    b08b        ..      SUB      sp,sp,#0x2c
        0x080004ae:    900a        ..      STR      r0,[sp,#0x28]
        0x080004b0:    9109        ..      STR      r1,[sp,#0x24]
        0x080004b2:    2000        .       MOVS     r0,#0
        0x080004b4:    9007        ..      STR      r0,[sp,#0x1c]
        0x080004b6:    9006        ..      STR      r0,[sp,#0x18]
        0x080004b8:    9005        ..      STR      r0,[sp,#0x14]
        0x080004ba:    9008        ..      STR      r0,[sp,#0x20]
        0x080004bc:    e7ff        ..      B        0x80004be ; HAL_GPIO_Init + 18
        0x080004be:    9808        ..      LDR      r0,[sp,#0x20]
        0x080004c0:    280f        .(      CMP      r0,#0xf
        0x080004c2:    f20081a2    ....    BHI.W    0x800080a ; HAL_GPIO_Init + 862
        0x080004c6:    e7ff        ..      B        0x80004c8 ; HAL_GPIO_Init + 28
        0x080004c8:    9908        ..      LDR      r1,[sp,#0x20]
        0x080004ca:    2001        .       MOVS     r0,#1
        0x080004cc:    4088        .@      LSLS     r0,r0,r1
        0x080004ce:    9007        ..      STR      r0,[sp,#0x1c]
        0x080004d0:    9809        ..      LDR      r0,[sp,#0x24]
        0x080004d2:    6800        .h      LDR      r0,[r0,#0]
        0x080004d4:    9907        ..      LDR      r1,[sp,#0x1c]
        0x080004d6:    4008        .@      ANDS     r0,r0,r1
        0x080004d8:    9006        ..      STR      r0,[sp,#0x18]
        0x080004da:    9806        ..      LDR      r0,[sp,#0x18]
        0x080004dc:    9907        ..      LDR      r1,[sp,#0x1c]
        0x080004de:    4288        .B      CMP      r0,r1
        0x080004e0:    f040818e    @...    BNE.W    0x8000800 ; HAL_GPIO_Init + 852
        0x080004e4:    e7ff        ..      B        0x80004e6 ; HAL_GPIO_Init + 58
        0x080004e6:    9809        ..      LDR      r0,[sp,#0x24]
        0x080004e8:    6840        @h      LDR      r0,[r0,#4]
        0x080004ea:    f0000003    ....    AND      r0,r0,#3
        0x080004ee:    2801        .(      CMP      r0,#1
        0x080004f0:    d007        ..      BEQ      0x8000502 ; HAL_GPIO_Init + 86
        0x080004f2:    e7ff        ..      B        0x80004f4 ; HAL_GPIO_Init + 72
        0x080004f4:    9809        ..      LDR      r0,[sp,#0x24]
        0x080004f6:    6840        @h      LDR      r0,[r0,#4]
        0x080004f8:    f0000003    ....    AND      r0,r0,#3
        0x080004fc:    2802        .(      CMP      r0,#2
        0x080004fe:    d12f        /.      BNE      0x8000560 ; HAL_GPIO_Init + 180
        0x08000500:    e7ff        ..      B        0x8000502 ; HAL_GPIO_Init + 86
        0x08000502:    980a        ..      LDR      r0,[sp,#0x28]
        0x08000504:    6880        .h      LDR      r0,[r0,#8]
        0x08000506:    9005        ..      STR      r0,[sp,#0x14]
        0x08000508:    9808        ..      LDR      r0,[sp,#0x20]
        0x0800050a:    0041        A.      LSLS     r1,r0,#1
        0x0800050c:    2003        .       MOVS     r0,#3
        0x0800050e:    fa00f101    ....    LSL      r1,r0,r1
        0x08000512:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000514:    4388        .C      BICS     r0,r0,r1
        0x08000516:    9005        ..      STR      r0,[sp,#0x14]
        0x08000518:    9809        ..      LDR      r0,[sp,#0x24]
        0x0800051a:    68c0        .h      LDR      r0,[r0,#0xc]
        0x0800051c:    9908        ..      LDR      r1,[sp,#0x20]
        0x0800051e:    0049        I.      LSLS     r1,r1,#1
        0x08000520:    fa00f101    ....    LSL      r1,r0,r1
        0x08000524:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000526:    4308        .C      ORRS     r0,r0,r1
        0x08000528:    9005        ..      STR      r0,[sp,#0x14]
        0x0800052a:    9805        ..      LDR      r0,[sp,#0x14]
        0x0800052c:    990a        ..      LDR      r1,[sp,#0x28]
        0x0800052e:    6088        .`      STR      r0,[r1,#8]
        0x08000530:    980a        ..      LDR      r0,[sp,#0x28]
        0x08000532:    6840        @h      LDR      r0,[r0,#4]
        0x08000534:    9005        ..      STR      r0,[sp,#0x14]
        0x08000536:    9908        ..      LDR      r1,[sp,#0x20]
        0x08000538:    2001        .       MOVS     r0,#1
        0x0800053a:    fa00f101    ....    LSL      r1,r0,r1
        0x0800053e:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000540:    4388        .C      BICS     r0,r0,r1
        0x08000542:    9005        ..      STR      r0,[sp,#0x14]
        0x08000544:    9809        ..      LDR      r0,[sp,#0x24]
        0x08000546:    6840        @h      LDR      r0,[r0,#4]
        0x08000548:    f3c01000    ....    UBFX     r0,r0,#4,#1
        0x0800054c:    9908        ..      LDR      r1,[sp,#0x20]
        0x0800054e:    fa00f101    ....    LSL      r1,r0,r1
        0x08000552:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000554:    4308        .C      ORRS     r0,r0,r1
        0x08000556:    9005        ..      STR      r0,[sp,#0x14]
        0x08000558:    9805        ..      LDR      r0,[sp,#0x14]
        0x0800055a:    990a        ..      LDR      r1,[sp,#0x28]
        0x0800055c:    6048        H`      STR      r0,[r1,#4]
        0x0800055e:    e7ff        ..      B        0x8000560 ; HAL_GPIO_Init + 180
        0x08000560:    9809        ..      LDR      r0,[sp,#0x24]
        0x08000562:    6840        @h      LDR      r0,[r0,#4]
        0x08000564:    f0000003    ....    AND      r0,r0,#3
        0x08000568:    2803        .(      CMP      r0,#3
        0x0800056a:    d018        ..      BEQ      0x800059e ; HAL_GPIO_Init + 242
        0x0800056c:    e7ff        ..      B        0x800056e ; HAL_GPIO_Init + 194
        0x0800056e:    980a        ..      LDR      r0,[sp,#0x28]
        0x08000570:    68c0        .h      LDR      r0,[r0,#0xc]
        0x08000572:    9005        ..      STR      r0,[sp,#0x14]
        0x08000574:    9808        ..      LDR      r0,[sp,#0x20]
        0x08000576:    0041        A.      LSLS     r1,r0,#1
        0x08000578:    2003        .       MOVS     r0,#3
        0x0800057a:    fa00f101    ....    LSL      r1,r0,r1
        0x0800057e:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000580:    4388        .C      BICS     r0,r0,r1
        0x08000582:    9005        ..      STR      r0,[sp,#0x14]
        0x08000584:    9809        ..      LDR      r0,[sp,#0x24]
        0x08000586:    6880        .h      LDR      r0,[r0,#8]
        0x08000588:    9908        ..      LDR      r1,[sp,#0x20]
        0x0800058a:    0049        I.      LSLS     r1,r1,#1
        0x0800058c:    fa00f101    ....    LSL      r1,r0,r1
        0x08000590:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000592:    4308        .C      ORRS     r0,r0,r1
        0x08000594:    9005        ..      STR      r0,[sp,#0x14]
        0x08000596:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000598:    990a        ..      LDR      r1,[sp,#0x28]
        0x0800059a:    60c8        .`      STR      r0,[r1,#0xc]
        0x0800059c:    e7ff        ..      B        0x800059e ; HAL_GPIO_Init + 242
        0x0800059e:    9809        ..      LDR      r0,[sp,#0x24]
        0x080005a0:    6840        @h      LDR      r0,[r0,#4]
        0x080005a2:    f0000003    ....    AND      r0,r0,#3
        0x080005a6:    2802        .(      CMP      r0,#2
        0x080005a8:    d126        &.      BNE      0x80005f8 ; HAL_GPIO_Init + 332
        0x080005aa:    e7ff        ..      B        0x80005ac ; HAL_GPIO_Init + 256
        0x080005ac:    980a        ..      LDR      r0,[sp,#0x28]
        0x080005ae:    9908        ..      LDR      r1,[sp,#0x20]
        0x080005b0:    f06f0203    o...    MVN      r2,#3
        0x080005b4:    ea020151    ..Q.    AND      r1,r2,r1,LSR #1
        0x080005b8:    4408        .D      ADD      r0,r0,r1
        0x080005ba:    6a00        .j      LDR      r0,[r0,#0x20]
        0x080005bc:    9005        ..      STR      r0,[sp,#0x14]
        0x080005be:    9808        ..      LDR      r0,[sp,#0x20]
        0x080005c0:    f0000007    ....    AND      r0,r0,#7
        0x080005c4:    0081        ..      LSLS     r1,r0,#2
        0x080005c6:    200f        .       MOVS     r0,#0xf
        0x080005c8:    fa00f101    ....    LSL      r1,r0,r1
        0x080005cc:    9805        ..      LDR      r0,[sp,#0x14]
        0x080005ce:    4388        .C      BICS     r0,r0,r1
        0x080005d0:    9005        ..      STR      r0,[sp,#0x14]
        0x080005d2:    9809        ..      LDR      r0,[sp,#0x24]
        0x080005d4:    6900        .i      LDR      r0,[r0,#0x10]
        0x080005d6:    9908        ..      LDR      r1,[sp,#0x20]
        0x080005d8:    f0010107    ....    AND      r1,r1,#7
        0x080005dc:    0089        ..      LSLS     r1,r1,#2
        0x080005de:    fa00f101    ....    LSL      r1,r0,r1
        0x080005e2:    9805        ..      LDR      r0,[sp,#0x14]
        0x080005e4:    4308        .C      ORRS     r0,r0,r1
        0x080005e6:    9005        ..      STR      r0,[sp,#0x14]
        0x080005e8:    9805        ..      LDR      r0,[sp,#0x14]
        0x080005ea:    990a        ..      LDR      r1,[sp,#0x28]
        0x080005ec:    9b08        ..      LDR      r3,[sp,#0x20]
        0x080005ee:    ea020253    ..S.    AND      r2,r2,r3,LSR #1
        0x080005f2:    4411        .D      ADD      r1,r1,r2
        0x080005f4:    6208        .b      STR      r0,[r1,#0x20]
        0x080005f6:    e7ff        ..      B        0x80005f8 ; HAL_GPIO_Init + 332
        0x080005f8:    980a        ..      LDR      r0,[sp,#0x28]
        0x080005fa:    6800        .h      LDR      r0,[r0,#0]
        0x080005fc:    9005        ..      STR      r0,[sp,#0x14]
        0x080005fe:    9808        ..      LDR      r0,[sp,#0x20]
        0x08000600:    0041        A.      LSLS     r1,r0,#1
        0x08000602:    2003        .       MOVS     r0,#3
        0x08000604:    fa00f101    ....    LSL      r1,r0,r1
        0x08000608:    9805        ..      LDR      r0,[sp,#0x14]
        0x0800060a:    4388        .C      BICS     r0,r0,r1
        0x0800060c:    9005        ..      STR      r0,[sp,#0x14]
        0x0800060e:    9809        ..      LDR      r0,[sp,#0x24]
        0x08000610:    6840        @h      LDR      r0,[r0,#4]
        0x08000612:    f0000003    ....    AND      r0,r0,#3
        0x08000616:    9908        ..      LDR      r1,[sp,#0x20]
        0x08000618:    0049        I.      LSLS     r1,r1,#1
        0x0800061a:    fa00f101    ....    LSL      r1,r0,r1
        0x0800061e:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000620:    4308        .C      ORRS     r0,r0,r1
        0x08000622:    9005        ..      STR      r0,[sp,#0x14]
        0x08000624:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000626:    990a        ..      LDR      r1,[sp,#0x28]
        0x08000628:    6008        .`      STR      r0,[r1,#0]
        0x0800062a:    9809        ..      LDR      r0,[sp,#0x24]
        0x0800062c:    7980        .y      LDRB     r0,[r0,#6]
        0x0800062e:    0780        ..      LSLS     r0,r0,#30
        0x08000630:    2800        .(      CMP      r0,#0
        0x08000632:    f00080e4    ....    BEQ.W    0x80007fe ; HAL_GPIO_Init + 850
        0x08000636:    e7ff        ..      B        0x8000638 ; HAL_GPIO_Init + 396
        0x08000638:    e7ff        ..      B        0x800063a ; HAL_GPIO_Init + 398
        0x0800063a:    2000        .       MOVS     r0,#0
        0x0800063c:    9004        ..      STR      r0,[sp,#0x10]
        0x0800063e:    f6430044    C.D.    MOV      r0,#0x3844
        0x08000642:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000646:    6801        .h      LDR      r1,[r0,#0]
        0x08000648:    f4414180    A..A    ORR      r1,r1,#0x4000
        0x0800064c:    6001        .`      STR      r1,[r0,#0]
        0x0800064e:    6800        .h      LDR      r0,[r0,#0]
        0x08000650:    f4004080    ...@    AND      r0,r0,#0x4000
        0x08000654:    9004        ..      STR      r0,[sp,#0x10]
        0x08000656:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000658:    e7ff        ..      B        0x800065a ; HAL_GPIO_Init + 430
        0x0800065a:    9808        ..      LDR      r0,[sp,#0x20]
        0x0800065c:    f0200003     ...    BIC      r0,r0,#3
        0x08000660:    f6430108    C...    MOV      r1,#0x3808
        0x08000664:    f2c40101    ....    MOVT     r1,#0x4001
        0x08000668:    5840        @X      LDR      r0,[r0,r1]
        0x0800066a:    9005        ..      STR      r0,[sp,#0x14]
        0x0800066c:    9808        ..      LDR      r0,[sp,#0x20]
        0x0800066e:    f0000003    ....    AND      r0,r0,#3
        0x08000672:    0081        ..      LSLS     r1,r0,#2
        0x08000674:    200f        .       MOVS     r0,#0xf
        0x08000676:    fa00f101    ....    LSL      r1,r0,r1
        0x0800067a:    9805        ..      LDR      r0,[sp,#0x14]
        0x0800067c:    4388        .C      BICS     r0,r0,r1
        0x0800067e:    9005        ..      STR      r0,[sp,#0x14]
        0x08000680:    980a        ..      LDR      r0,[sp,#0x28]
        0x08000682:    2100        .!      MOVS     r1,#0
        0x08000684:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000688:    4288        .B      CMP      r0,r1
        0x0800068a:    d103        ..      BNE      0x8000694 ; HAL_GPIO_Init + 488
        0x0800068c:    e7ff        ..      B        0x800068e ; HAL_GPIO_Init + 482
        0x0800068e:    2000        .       MOVS     r0,#0
        0x08000690:    9003        ..      STR      r0,[sp,#0xc]
        0x08000692:    e034        4.      B        0x80006fe ; HAL_GPIO_Init + 594
        0x08000694:    980a        ..      LDR      r0,[sp,#0x28]
        0x08000696:    f2404100    @..A    MOVW     r1,#0x400
        0x0800069a:    f2c40102    ....    MOVT     r1,#0x4002
        0x0800069e:    4288        .B      CMP      r0,r1
        0x080006a0:    d103        ..      BNE      0x80006aa ; HAL_GPIO_Init + 510
        0x080006a2:    e7ff        ..      B        0x80006a4 ; HAL_GPIO_Init + 504
        0x080006a4:    2001        .       MOVS     r0,#1
        0x080006a6:    9002        ..      STR      r0,[sp,#8]
        0x080006a8:    e026        &.      B        0x80006f8 ; HAL_GPIO_Init + 588
        0x080006aa:    980a        ..      LDR      r0,[sp,#0x28]
        0x080006ac:    f6400100    @...    MOVW     r1,#0x800
        0x080006b0:    f2c40102    ....    MOVT     r1,#0x4002
        0x080006b4:    4288        .B      CMP      r0,r1
        0x080006b6:    d103        ..      BNE      0x80006c0 ; HAL_GPIO_Init + 532
        0x080006b8:    e7ff        ..      B        0x80006ba ; HAL_GPIO_Init + 526
        0x080006ba:    2002        .       MOVS     r0,#2
        0x080006bc:    9001        ..      STR      r0,[sp,#4]
        0x080006be:    e018        ..      B        0x80006f2 ; HAL_GPIO_Init + 582
        0x080006c0:    980a        ..      LDR      r0,[sp,#0x28]
        0x080006c2:    f6404100    @..A    MOVW     r1,#0xc00
        0x080006c6:    f2c40102    ....    MOVT     r1,#0x4002
        0x080006ca:    4288        .B      CMP      r0,r1
        0x080006cc:    d103        ..      BNE      0x80006d6 ; HAL_GPIO_Init + 554
        0x080006ce:    e7ff        ..      B        0x80006d0 ; HAL_GPIO_Init + 548
        0x080006d0:    2003        .       MOVS     r0,#3
        0x080006d2:    9000        ..      STR      r0,[sp,#0]
        0x080006d4:    e00a        ..      B        0x80006ec ; HAL_GPIO_Init + 576
        0x080006d6:    990a        ..      LDR      r1,[sp,#0x28]
        0x080006d8:    f2410200    A...    MOVW     r2,#0x1000
        0x080006dc:    f2c40202    ....    MOVT     r2,#0x4002
        0x080006e0:    2007        .       MOVS     r0,#7
        0x080006e2:    4291        .B      CMP      r1,r2
        0x080006e4:    bf08        ..      IT       EQ
        0x080006e6:    2004        .       MOVEQ    r0,#4
        0x080006e8:    9000        ..      STR      r0,[sp,#0]
        0x080006ea:    e7ff        ..      B        0x80006ec ; HAL_GPIO_Init + 576
        0x080006ec:    9800        ..      LDR      r0,[sp,#0]
        0x080006ee:    9001        ..      STR      r0,[sp,#4]
        0x080006f0:    e7ff        ..      B        0x80006f2 ; HAL_GPIO_Init + 582
        0x080006f2:    9801        ..      LDR      r0,[sp,#4]
        0x080006f4:    9002        ..      STR      r0,[sp,#8]
        0x080006f6:    e7ff        ..      B        0x80006f8 ; HAL_GPIO_Init + 588
        0x080006f8:    9802        ..      LDR      r0,[sp,#8]
        0x080006fa:    9003        ..      STR      r0,[sp,#0xc]
        0x080006fc:    e7ff        ..      B        0x80006fe ; HAL_GPIO_Init + 594
        0x080006fe:    9803        ..      LDR      r0,[sp,#0xc]
        0x08000700:    b2c0        ..      UXTB     r0,r0
        0x08000702:    9908        ..      LDR      r1,[sp,#0x20]
        0x08000704:    f0010103    ....    AND      r1,r1,#3
        0x08000708:    0089        ..      LSLS     r1,r1,#2
        0x0800070a:    fa00f101    ....    LSL      r1,r0,r1
        0x0800070e:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000710:    4308        .C      ORRS     r0,r0,r1
        0x08000712:    9005        ..      STR      r0,[sp,#0x14]
        0x08000714:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000716:    9908        ..      LDR      r1,[sp,#0x20]
        0x08000718:    f0210103    !...    BIC      r1,r1,#3
        0x0800071c:    f6430208    C...    MOV      r2,#0x3808
        0x08000720:    f2c40201    ....    MOVT     r2,#0x4001
        0x08000724:    5088        .P      STR      r0,[r1,r2]
        0x08000726:    f6434008    C..@    MOV      r0,#0x3c08
        0x0800072a:    f2c40001    ....    MOVT     r0,#0x4001
        0x0800072e:    6800        .h      LDR      r0,[r0,#0]
        0x08000730:    9005        ..      STR      r0,[sp,#0x14]
        0x08000732:    9906        ..      LDR      r1,[sp,#0x18]
        0x08000734:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000736:    4388        .C      BICS     r0,r0,r1
        0x08000738:    9005        ..      STR      r0,[sp,#0x14]
        0x0800073a:    9809        ..      LDR      r0,[sp,#0x24]
        0x0800073c:    7980        .y      LDRB     r0,[r0,#6]
        0x0800073e:    06c0        ..      LSLS     r0,r0,#27
        0x08000740:    2800        .(      CMP      r0,#0
        0x08000742:    d505        ..      BPL      0x8000750 ; HAL_GPIO_Init + 676
        0x08000744:    e7ff        ..      B        0x8000746 ; HAL_GPIO_Init + 666
        0x08000746:    9906        ..      LDR      r1,[sp,#0x18]
        0x08000748:    9805        ..      LDR      r0,[sp,#0x14]
        0x0800074a:    4308        .C      ORRS     r0,r0,r1
        0x0800074c:    9005        ..      STR      r0,[sp,#0x14]
        0x0800074e:    e7ff        ..      B        0x8000750 ; HAL_GPIO_Init + 676
        0x08000750:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000752:    f6434108    C..A    MOV      r1,#0x3c08
        0x08000756:    f2c40101    ....    MOVT     r1,#0x4001
        0x0800075a:    6008        .`      STR      r0,[r1,#0]
        0x0800075c:    f643400c    C..@    MOV      r0,#0x3c0c
        0x08000760:    f2c40001    ....    MOVT     r0,#0x4001
        0x08000764:    6800        .h      LDR      r0,[r0,#0]
        0x08000766:    9005        ..      STR      r0,[sp,#0x14]
        0x08000768:    9906        ..      LDR      r1,[sp,#0x18]
        0x0800076a:    9805        ..      LDR      r0,[sp,#0x14]
        0x0800076c:    4388        .C      BICS     r0,r0,r1
        0x0800076e:    9005        ..      STR      r0,[sp,#0x14]
        0x08000770:    9809        ..      LDR      r0,[sp,#0x24]
        0x08000772:    7980        .y      LDRB     r0,[r0,#6]
        0x08000774:    0680        ..      LSLS     r0,r0,#26
        0x08000776:    2800        .(      CMP      r0,#0
        0x08000778:    d505        ..      BPL      0x8000786 ; HAL_GPIO_Init + 730
        0x0800077a:    e7ff        ..      B        0x800077c ; HAL_GPIO_Init + 720
        0x0800077c:    9906        ..      LDR      r1,[sp,#0x18]
        0x0800077e:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000780:    4308        .C      ORRS     r0,r0,r1
        0x08000782:    9005        ..      STR      r0,[sp,#0x14]
        0x08000784:    e7ff        ..      B        0x8000786 ; HAL_GPIO_Init + 730
        0x08000786:    9805        ..      LDR      r0,[sp,#0x14]
        0x08000788:    f643410c    C..A    MOV      r1,#0x3c0c
        0x0800078c:    f2c40101    ....    MOVT     r1,#0x4001
        0x08000790:    6008        .`      STR      r0,[r1,#0]
        0x08000792:    f6434004    C..@    MOV      r0,#0x3c04
        0x08000796:    f2c40001    ....    MOVT     r0,#0x4001
        0x0800079a:    6800        .h      LDR      r0,[r0,#0]
        0x0800079c:    9005        ..      STR      r0,[sp,#0x14]
        0x0800079e:    9906        ..      LDR      r1,[sp,#0x18]
        0x080007a0:    9805        ..      LDR      r0,[sp,#0x14]
        0x080007a2:    4388        .C      BICS     r0,r0,r1
        0x080007a4:    9005        ..      STR      r0,[sp,#0x14]
        0x080007a6:    9809        ..      LDR      r0,[sp,#0x24]
        0x080007a8:    7980        .y      LDRB     r0,[r0,#6]
        0x080007aa:    0780        ..      LSLS     r0,r0,#30
        0x080007ac:    2800        .(      CMP      r0,#0
        0x080007ae:    d505        ..      BPL      0x80007bc ; HAL_GPIO_Init + 784
        0x080007b0:    e7ff        ..      B        0x80007b2 ; HAL_GPIO_Init + 774
        0x080007b2:    9906        ..      LDR      r1,[sp,#0x18]
        0x080007b4:    9805        ..      LDR      r0,[sp,#0x14]
        0x080007b6:    4308        .C      ORRS     r0,r0,r1
        0x080007b8:    9005        ..      STR      r0,[sp,#0x14]
        0x080007ba:    e7ff        ..      B        0x80007bc ; HAL_GPIO_Init + 784
        0x080007bc:    9805        ..      LDR      r0,[sp,#0x14]
        0x080007be:    f6434104    C..A    MOV      r1,#0x3c04
        0x080007c2:    f2c40101    ....    MOVT     r1,#0x4001
        0x080007c6:    6008        .`      STR      r0,[r1,#0]
        0x080007c8:    f6434000    C..@    MOVW     r0,#0x3c00
        0x080007cc:    f2c40001    ....    MOVT     r0,#0x4001
        0x080007d0:    6800        .h      LDR      r0,[r0,#0]
        0x080007d2:    9005        ..      STR      r0,[sp,#0x14]
        0x080007d4:    9906        ..      LDR      r1,[sp,#0x18]
        0x080007d6:    9805        ..      LDR      r0,[sp,#0x14]
        0x080007d8:    4388        .C      BICS     r0,r0,r1
        0x080007da:    9005        ..      STR      r0,[sp,#0x14]
        0x080007dc:    9809        ..      LDR      r0,[sp,#0x24]
        0x080007de:    7980        .y      LDRB     r0,[r0,#6]
        0x080007e0:    07c0        ..      LSLS     r0,r0,#31
        0x080007e2:    b128        (.      CBZ      r0,0x80007f0 ; HAL_GPIO_Init + 836
        0x080007e4:    e7ff        ..      B        0x80007e6 ; HAL_GPIO_Init + 826
        0x080007e6:    9906        ..      LDR      r1,[sp,#0x18]
        0x080007e8:    9805        ..      LDR      r0,[sp,#0x14]
        0x080007ea:    4308        .C      ORRS     r0,r0,r1
        0x080007ec:    9005        ..      STR      r0,[sp,#0x14]
        0x080007ee:    e7ff        ..      B        0x80007f0 ; HAL_GPIO_Init + 836
        0x080007f0:    9805        ..      LDR      r0,[sp,#0x14]
        0x080007f2:    f6434100    C..A    MOVW     r1,#0x3c00
        0x080007f6:    f2c40101    ....    MOVT     r1,#0x4001
        0x080007fa:    6008        .`      STR      r0,[r1,#0]
        0x080007fc:    e7ff        ..      B        0x80007fe ; HAL_GPIO_Init + 850
        0x080007fe:    e7ff        ..      B        0x8000800 ; HAL_GPIO_Init + 852
        0x08000800:    e7ff        ..      B        0x8000802 ; HAL_GPIO_Init + 854
        0x08000802:    9808        ..      LDR      r0,[sp,#0x20]
        0x08000804:    3001        .0      ADDS     r0,#1
        0x08000806:    9008        ..      STR      r0,[sp,#0x20]
        0x08000808:    e659        Y.      B        0x80004be ; HAL_GPIO_Init + 18
        0x0800080a:    b00b        ..      ADD      sp,sp,#0x2c
        0x0800080c:    4770        pG      BX       lr
        0x0800080e:    0000        ..      MOVS     r0,r0
    HAL_GPIO_WritePin
        0x08000810:    b082        ..      SUB      sp,sp,#8
        0x08000812:    9001        ..      STR      r0,[sp,#4]
        0x08000814:    f8ad1002    ....    STRH     r1,[sp,#2]
        0x08000818:    f88d2001    ...     STRB     r2,[sp,#1]
        0x0800081c:    f89d0001    ....    LDRB     r0,[sp,#1]
        0x08000820:    b128        (.      CBZ      r0,0x800082e ; HAL_GPIO_WritePin + 30
        0x08000822:    e7ff        ..      B        0x8000824 ; HAL_GPIO_WritePin + 20
        0x08000824:    f8bd0002    ....    LDRH     r0,[sp,#2]
        0x08000828:    9901        ..      LDR      r1,[sp,#4]
        0x0800082a:    6188        .a      STR      r0,[r1,#0x18]
        0x0800082c:    e005        ..      B        0x800083a ; HAL_GPIO_WritePin + 42
        0x0800082e:    f8bd0002    ....    LDRH     r0,[sp,#2]
        0x08000832:    0400        ..      LSLS     r0,r0,#16
        0x08000834:    9901        ..      LDR      r1,[sp,#4]
        0x08000836:    6188        .a      STR      r0,[r1,#0x18]
        0x08000838:    e7ff        ..      B        0x800083a ; HAL_GPIO_WritePin + 42
        0x0800083a:    b002        ..      ADD      sp,sp,#8
        0x0800083c:    4770        pG      BX       lr
        0x0800083e:    0000        ..      MOVS     r0,r0
    HAL_GetTick
        0x08000840:    f240007c    @.|.    MOVW     r0,#0x7c
        0x08000844:    f2c20000    ....    MOVT     r0,#0x2000
        0x08000848:    6800        .h      LDR      r0,[r0,#0]
        0x0800084a:    4770        pG      BX       lr
    HAL_IWDG_Init
        0x0800084c:    b580        ..      PUSH     {r7,lr}
        0x0800084e:    b084        ..      SUB      sp,sp,#0x10
        0x08000850:    9002        ..      STR      r0,[sp,#8]
        0x08000852:    9802        ..      LDR      r0,[sp,#8]
        0x08000854:    b920         .      CBNZ     r0,0x8000860 ; HAL_IWDG_Init + 20
        0x08000856:    e7ff        ..      B        0x8000858 ; HAL_IWDG_Init + 12
        0x08000858:    2001        .       MOVS     r0,#1
        0x0800085a:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x0800085e:    e039        9.      B        0x80008d4 ; HAL_IWDG_Init + 136
        0x08000860:    9802        ..      LDR      r0,[sp,#8]
        0x08000862:    6801        .h      LDR      r1,[r0,#0]
        0x08000864:    f64c40cc    L..@    MOV      r0,#0xcccc
        0x08000868:    6008        .`      STR      r0,[r1,#0]
        0x0800086a:    9802        ..      LDR      r0,[sp,#8]
        0x0800086c:    6801        .h      LDR      r1,[r0,#0]
        0x0800086e:    f2455055    E.UP    MOV      r0,#0x5555
        0x08000872:    6008        .`      STR      r0,[r1,#0]
        0x08000874:    9802        ..      LDR      r0,[sp,#8]
        0x08000876:    6801        .h      LDR      r1,[r0,#0]
        0x08000878:    6840        @h      LDR      r0,[r0,#4]
        0x0800087a:    6048        H`      STR      r0,[r1,#4]
        0x0800087c:    9802        ..      LDR      r0,[sp,#8]
        0x0800087e:    6801        .h      LDR      r1,[r0,#0]
        0x08000880:    6880        .h      LDR      r0,[r0,#8]
        0x08000882:    6088        .`      STR      r0,[r1,#8]
        0x08000884:    f7ffffdc    ....    BL       HAL_GetTick ; 0x8000840
        0x08000888:    9001        ..      STR      r0,[sp,#4]
        0x0800088a:    e7ff        ..      B        0x800088c ; HAL_IWDG_Init + 64
        0x0800088c:    9802        ..      LDR      r0,[sp,#8]
        0x0800088e:    6800        .h      LDR      r0,[r0,#0]
        0x08000890:    68c0        .h      LDR      r0,[r0,#0xc]
        0x08000892:    0780        ..      LSLS     r0,r0,#30
        0x08000894:    b1a8        ..      CBZ      r0,0x80008c2 ; HAL_IWDG_Init + 118
        0x08000896:    e7ff        ..      B        0x8000898 ; HAL_IWDG_Init + 76
        0x08000898:    f7ffffd2    ....    BL       HAL_GetTick ; 0x8000840
        0x0800089c:    9901        ..      LDR      r1,[sp,#4]
        0x0800089e:    1a40        @.      SUBS     r0,r0,r1
        0x080008a0:    f6410102    A...    MOV      r1,#0x1802
        0x080008a4:    4288        .B      CMP      r0,r1
        0x080008a6:    d30b        ..      BCC      0x80008c0 ; HAL_IWDG_Init + 116
        0x080008a8:    e7ff        ..      B        0x80008aa ; HAL_IWDG_Init + 94
        0x080008aa:    9802        ..      LDR      r0,[sp,#8]
        0x080008ac:    6800        .h      LDR      r0,[r0,#0]
        0x080008ae:    68c0        .h      LDR      r0,[r0,#0xc]
        0x080008b0:    0780        ..      LSLS     r0,r0,#30
        0x080008b2:    b120         .      CBZ      r0,0x80008be ; HAL_IWDG_Init + 114
        0x080008b4:    e7ff        ..      B        0x80008b6 ; HAL_IWDG_Init + 106
        0x080008b6:    2003        .       MOVS     r0,#3
        0x080008b8:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x080008bc:    e00a        ..      B        0x80008d4 ; HAL_IWDG_Init + 136
        0x080008be:    e7ff        ..      B        0x80008c0 ; HAL_IWDG_Init + 116
        0x080008c0:    e7e4        ..      B        0x800088c ; HAL_IWDG_Init + 64
        0x080008c2:    9802        ..      LDR      r0,[sp,#8]
        0x080008c4:    6801        .h      LDR      r1,[r0,#0]
        0x080008c6:    f64a20aa    J..     MOV      r0,#0xaaaa
        0x080008ca:    6008        .`      STR      r0,[r1,#0]
        0x080008cc:    2000        .       MOVS     r0,#0
        0x080008ce:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x080008d2:    e7ff        ..      B        0x80008d4 ; HAL_IWDG_Init + 136
        0x080008d4:    f89d000f    ....    LDRB     r0,[sp,#0xf]
        0x080008d8:    b004        ..      ADD      sp,sp,#0x10
        0x080008da:    bd80        ..      POP      {r7,pc}
    HAL_IncTick
        0x080008dc:    f2400004    @...    MOVW     r0,#4
        0x080008e0:    f2c20000    ....    MOVT     r0,#0x2000
        0x080008e4:    7802        .x      LDRB     r2,[r0,#0]
        0x080008e6:    f240017c    @.|.    MOVW     r1,#0x7c
        0x080008ea:    f2c20100    ....    MOVT     r1,#0x2000
        0x080008ee:    6808        .h      LDR      r0,[r1,#0]
        0x080008f0:    4410        .D      ADD      r0,r0,r2
        0x080008f2:    6008        .`      STR      r0,[r1,#0]
        0x080008f4:    4770        pG      BX       lr
        0x080008f6:    0000        ..      MOVS     r0,r0
    HAL_Init
        0x080008f8:    b580        ..      PUSH     {r7,lr}
        0x080008fa:    f6434100    C..A    MOVW     r1,#0x3c00
        0x080008fe:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000902:    6808        .h      LDR      r0,[r1,#0]
        0x08000904:    f4407000    @..p    ORR      r0,r0,#0x200
        0x08000908:    6008        .`      STR      r0,[r1,#0]
        0x0800090a:    6808        .h      LDR      r0,[r1,#0]
        0x0800090c:    f4406080    @..`    ORR      r0,r0,#0x400
        0x08000910:    6008        .`      STR      r0,[r1,#0]
        0x08000912:    6808        .h      LDR      r0,[r1,#0]
        0x08000914:    f4407080    @..p    ORR      r0,r0,#0x100
        0x08000918:    6008        .`      STR      r0,[r1,#0]
        0x0800091a:    2003        .       MOVS     r0,#3
        0x0800091c:    f000f880    ....    BL       HAL_NVIC_SetPriorityGrouping ; 0x8000a20
        0x08000920:    200f        .       MOVS     r0,#0xf
        0x08000922:    f000f805    ....    BL       HAL_InitTick ; 0x8000930
        0x08000926:    f000f83b    ..;.    BL       HAL_MspInit ; 0x80009a0
        0x0800092a:    2000        .       MOVS     r0,#0
        0x0800092c:    bd80        ..      POP      {r7,pc}
        0x0800092e:    0000        ..      MOVS     r0,r0
    HAL_InitTick
        0x08000930:    b580        ..      PUSH     {r7,lr}
        0x08000932:    b082        ..      SUB      sp,sp,#8
        0x08000934:    9000        ..      STR      r0,[sp,#0]
        0x08000936:    f2400000    @...    MOVW     r0,#0
        0x0800093a:    f2c20000    ....    MOVT     r0,#0x2000
        0x0800093e:    6800        .h      LDR      r0,[r0,#0]
        0x08000940:    f2400104    @...    MOVW     r1,#4
        0x08000944:    f2c20100    ....    MOVT     r1,#0x2000
        0x08000948:    780a        .x      LDRB     r2,[r1,#0]
        0x0800094a:    f44f717a    O.zq    MOV      r1,#0x3e8
        0x0800094e:    fbb1f1f2    ....    UDIV     r1,r1,r2
        0x08000952:    fbb0f0f1    ....    UDIV     r0,r0,r1
        0x08000956:    f000fd71    ..q.    BL       HAL_SYSTICK_Config ; 0x800143c
        0x0800095a:    b120         .      CBZ      r0,0x8000966 ; HAL_InitTick + 54
        0x0800095c:    e7ff        ..      B        0x800095e ; HAL_InitTick + 46
        0x0800095e:    2001        .       MOVS     r0,#1
        0x08000960:    f88d0007    ....    STRB     r0,[sp,#7]
        0x08000964:    e018        ..      B        0x8000998 ; HAL_InitTick + 104
        0x08000966:    9800        ..      LDR      r0,[sp,#0]
        0x08000968:    280f        .(      CMP      r0,#0xf
        0x0800096a:    d80d        ..      BHI      0x8000988 ; HAL_InitTick + 88
        0x0800096c:    e7ff        ..      B        0x800096e ; HAL_InitTick + 62
        0x0800096e:    9900        ..      LDR      r1,[sp,#0]
        0x08000970:    f04f30ff    O..0    MOV      r0,#0xffffffff
        0x08000974:    2200        ."      MOVS     r2,#0
        0x08000976:    f000f839    ..9.    BL       HAL_NVIC_SetPriority ; 0x80009ec
        0x0800097a:    9800        ..      LDR      r0,[sp,#0]
        0x0800097c:    f2400108    @...    MOVW     r1,#8
        0x08000980:    f2c20100    ....    MOVT     r1,#0x2000
        0x08000984:    6008        .`      STR      r0,[r1,#0]
        0x08000986:    e003        ..      B        0x8000990 ; HAL_InitTick + 96
        0x08000988:    2001        .       MOVS     r0,#1
        0x0800098a:    f88d0007    ....    STRB     r0,[sp,#7]
        0x0800098e:    e003        ..      B        0x8000998 ; HAL_InitTick + 104
        0x08000990:    2000        .       MOVS     r0,#0
        0x08000992:    f88d0007    ....    STRB     r0,[sp,#7]
        0x08000996:    e7ff        ..      B        0x8000998 ; HAL_InitTick + 104
        0x08000998:    f89d0007    ....    LDRB     r0,[sp,#7]
        0x0800099c:    b002        ..      ADD      sp,sp,#8
        0x0800099e:    bd80        ..      POP      {r7,pc}
    HAL_MspInit
        0x080009a0:    b082        ..      SUB      sp,sp,#8
        0x080009a2:    e7ff        ..      B        0x80009a4 ; HAL_MspInit + 4
        0x080009a4:    2000        .       MOVS     r0,#0
        0x080009a6:    9001        ..      STR      r0,[sp,#4]
        0x080009a8:    f6430044    C.D.    MOV      r0,#0x3844
        0x080009ac:    f2c40002    ....    MOVT     r0,#0x4002
        0x080009b0:    6801        .h      LDR      r1,[r0,#0]
        0x080009b2:    f4414180    A..A    ORR      r1,r1,#0x4000
        0x080009b6:    6001        .`      STR      r1,[r0,#0]
        0x080009b8:    6800        .h      LDR      r0,[r0,#0]
        0x080009ba:    f4004080    ...@    AND      r0,r0,#0x4000
        0x080009be:    9001        ..      STR      r0,[sp,#4]
        0x080009c0:    9801        ..      LDR      r0,[sp,#4]
        0x080009c2:    e7ff        ..      B        0x80009c4 ; HAL_MspInit + 36
        0x080009c4:    e7ff        ..      B        0x80009c6 ; HAL_MspInit + 38
        0x080009c6:    2000        .       MOVS     r0,#0
        0x080009c8:    9000        ..      STR      r0,[sp,#0]
        0x080009ca:    f6430040    C.@.    MOVW     r0,#0x3840
        0x080009ce:    f2c40002    ....    MOVT     r0,#0x4002
        0x080009d2:    6801        .h      LDR      r1,[r0,#0]
        0x080009d4:    f0415180    A..Q    ORR      r1,r1,#0x10000000
        0x080009d8:    6001        .`      STR      r1,[r0,#0]
        0x080009da:    6800        .h      LDR      r0,[r0,#0]
        0x080009dc:    f0005080    ...P    AND      r0,r0,#0x10000000
        0x080009e0:    9000        ..      STR      r0,[sp,#0]
        0x080009e2:    9800        ..      LDR      r0,[sp,#0]
        0x080009e4:    e7ff        ..      B        0x80009e6 ; HAL_MspInit + 70
        0x080009e6:    b002        ..      ADD      sp,sp,#8
        0x080009e8:    4770        pG      BX       lr
        0x080009ea:    0000        ..      MOVS     r0,r0
    HAL_NVIC_SetPriority
        0x080009ec:    b580        ..      PUSH     {r7,lr}
        0x080009ee:    b086        ..      SUB      sp,sp,#0x18
        0x080009f0:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x080009f4:    9104        ..      STR      r1,[sp,#0x10]
        0x080009f6:    9203        ..      STR      r2,[sp,#0xc]
        0x080009f8:    2000        .       MOVS     r0,#0
        0x080009fa:    9002        ..      STR      r0,[sp,#8]
        0x080009fc:    f000fe58    ..X.    BL       __NVIC_GetPriorityGrouping ; 0x80016b0
        0x08000a00:    9002        ..      STR      r0,[sp,#8]
        0x08000a02:    f99d0017    ....    LDRSB    r0,[sp,#0x17]
        0x08000a06:    9001        ..      STR      r0,[sp,#4]
        0x08000a08:    9802        ..      LDR      r0,[sp,#8]
        0x08000a0a:    9904        ..      LDR      r1,[sp,#0x10]
        0x08000a0c:    9a03        ..      LDR      r2,[sp,#0xc]
        0x08000a0e:    f000fd7d    ..}.    BL       NVIC_EncodePriority ; 0x800150c
        0x08000a12:    4601        .F      MOV      r1,r0
        0x08000a14:    9801        ..      LDR      r0,[sp,#4]
        0x08000a16:    f000fe53    ..S.    BL       __NVIC_SetPriority ; 0x80016c0
        0x08000a1a:    b006        ..      ADD      sp,sp,#0x18
        0x08000a1c:    bd80        ..      POP      {r7,pc}
        0x08000a1e:    0000        ..      MOVS     r0,r0
    HAL_NVIC_SetPriorityGrouping
        0x08000a20:    b580        ..      PUSH     {r7,lr}
        0x08000a22:    b082        ..      SUB      sp,sp,#8
        0x08000a24:    9001        ..      STR      r0,[sp,#4]
        0x08000a26:    9801        ..      LDR      r0,[sp,#4]
        0x08000a28:    f000fe6c    ..l.    BL       __NVIC_SetPriorityGrouping ; 0x8001704
        0x08000a2c:    b002        ..      ADD      sp,sp,#8
        0x08000a2e:    bd80        ..      POP      {r7,pc}
    HAL_RCC_ClockConfig
        0x08000a30:    b580        ..      PUSH     {r7,lr}
        0x08000a32:    b084        ..      SUB      sp,sp,#0x10
        0x08000a34:    9002        ..      STR      r0,[sp,#8]
        0x08000a36:    9101        ..      STR      r1,[sp,#4]
        0x08000a38:    9802        ..      LDR      r0,[sp,#8]
        0x08000a3a:    b920         .      CBNZ     r0,0x8000a46 ; HAL_RCC_ClockConfig + 22
        0x08000a3c:    e7ff        ..      B        0x8000a3e ; HAL_RCC_ClockConfig + 14
        0x08000a3e:    2001        .       MOVS     r0,#1
        0x08000a40:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000a44:    e121        !.      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000a46:    9801        ..      LDR      r0,[sp,#4]
        0x08000a48:    f6434100    C..A    MOVW     r1,#0x3c00
        0x08000a4c:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000a50:    6809        .h      LDR      r1,[r1,#0]
        0x08000a52:    f0010107    ....    AND      r1,r1,#7
        0x08000a56:    4288        .B      CMP      r0,r1
        0x08000a58:    d912        ..      BLS      0x8000a80 ; HAL_RCC_ClockConfig + 80
        0x08000a5a:    e7ff        ..      B        0x8000a5c ; HAL_RCC_ClockConfig + 44
        0x08000a5c:    9901        ..      LDR      r1,[sp,#4]
        0x08000a5e:    f6434000    C..@    MOVW     r0,#0x3c00
        0x08000a62:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000a66:    7001        .p      STRB     r1,[r0,#0]
        0x08000a68:    6800        .h      LDR      r0,[r0,#0]
        0x08000a6a:    f0000007    ....    AND      r0,r0,#7
        0x08000a6e:    9901        ..      LDR      r1,[sp,#4]
        0x08000a70:    4288        .B      CMP      r0,r1
        0x08000a72:    d004        ..      BEQ      0x8000a7e ; HAL_RCC_ClockConfig + 78
        0x08000a74:    e7ff        ..      B        0x8000a76 ; HAL_RCC_ClockConfig + 70
        0x08000a76:    2001        .       MOVS     r0,#1
        0x08000a78:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000a7c:    e105        ..      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000a7e:    e7ff        ..      B        0x8000a80 ; HAL_RCC_ClockConfig + 80
        0x08000a80:    9802        ..      LDR      r0,[sp,#8]
        0x08000a82:    7800        .x      LDRB     r0,[r0,#0]
        0x08000a84:    0780        ..      LSLS     r0,r0,#30
        0x08000a86:    2800        .(      CMP      r0,#0
        0x08000a88:    d52a        *.      BPL      0x8000ae0 ; HAL_RCC_ClockConfig + 176
        0x08000a8a:    e7ff        ..      B        0x8000a8c ; HAL_RCC_ClockConfig + 92
        0x08000a8c:    9802        ..      LDR      r0,[sp,#8]
        0x08000a8e:    7800        .x      LDRB     r0,[r0,#0]
        0x08000a90:    0740        @.      LSLS     r0,r0,#29
        0x08000a92:    2800        .(      CMP      r0,#0
        0x08000a94:    d509        ..      BPL      0x8000aaa ; HAL_RCC_ClockConfig + 122
        0x08000a96:    e7ff        ..      B        0x8000a98 ; HAL_RCC_ClockConfig + 104
        0x08000a98:    f6430108    C...    MOV      r1,#0x3808
        0x08000a9c:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000aa0:    6808        .h      LDR      r0,[r1,#0]
        0x08000aa2:    f44050e0    @..P    ORR      r0,r0,#0x1c00
        0x08000aa6:    6008        .`      STR      r0,[r1,#0]
        0x08000aa8:    e7ff        ..      B        0x8000aaa ; HAL_RCC_ClockConfig + 122
        0x08000aaa:    9802        ..      LDR      r0,[sp,#8]
        0x08000aac:    7800        .x      LDRB     r0,[r0,#0]
        0x08000aae:    0700        ..      LSLS     r0,r0,#28
        0x08000ab0:    2800        .(      CMP      r0,#0
        0x08000ab2:    d509        ..      BPL      0x8000ac8 ; HAL_RCC_ClockConfig + 152
        0x08000ab4:    e7ff        ..      B        0x8000ab6 ; HAL_RCC_ClockConfig + 134
        0x08000ab6:    f6430108    C...    MOV      r1,#0x3808
        0x08000aba:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000abe:    6808        .h      LDR      r0,[r1,#0]
        0x08000ac0:    f4404060    @.`@    ORR      r0,r0,#0xe000
        0x08000ac4:    6008        .`      STR      r0,[r1,#0]
        0x08000ac6:    e7ff        ..      B        0x8000ac8 ; HAL_RCC_ClockConfig + 152
        0x08000ac8:    f6430108    C...    MOV      r1,#0x3808
        0x08000acc:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000ad0:    6808        .h      LDR      r0,[r1,#0]
        0x08000ad2:    f02000f0     ...    BIC      r0,r0,#0xf0
        0x08000ad6:    9a02        ..      LDR      r2,[sp,#8]
        0x08000ad8:    6892        .h      LDR      r2,[r2,#8]
        0x08000ada:    4310        .C      ORRS     r0,r0,r2
        0x08000adc:    6008        .`      STR      r0,[r1,#0]
        0x08000ade:    e7ff        ..      B        0x8000ae0 ; HAL_RCC_ClockConfig + 176
        0x08000ae0:    9802        ..      LDR      r0,[sp,#8]
        0x08000ae2:    7800        .x      LDRB     r0,[r0,#0]
        0x08000ae4:    07c0        ..      LSLS     r0,r0,#31
        0x08000ae6:    2800        .(      CMP      r0,#0
        0x08000ae8:    d06e        n.      BEQ      0x8000bc8 ; HAL_RCC_ClockConfig + 408
        0x08000aea:    e7ff        ..      B        0x8000aec ; HAL_RCC_ClockConfig + 188
        0x08000aec:    9802        ..      LDR      r0,[sp,#8]
        0x08000aee:    6840        @h      LDR      r0,[r0,#4]
        0x08000af0:    2801        .(      CMP      r0,#1
        0x08000af2:    d111        ..      BNE      0x8000b18 ; HAL_RCC_ClockConfig + 232
        0x08000af4:    e7ff        ..      B        0x8000af6 ; HAL_RCC_ClockConfig + 198
        0x08000af6:    f6430000    C...    MOVW     r0,#0x3800
        0x08000afa:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000afe:    6800        .h      LDR      r0,[r0,#0]
        0x08000b00:    f4003100    ...1    AND      r1,r0,#0x20000
        0x08000b04:    2000        .       MOVS     r0,#0
        0x08000b06:    ebb04f51    ..QO    CMP      r0,r1,LSR #17
        0x08000b0a:    d104        ..      BNE      0x8000b16 ; HAL_RCC_ClockConfig + 230
        0x08000b0c:    e7ff        ..      B        0x8000b0e ; HAL_RCC_ClockConfig + 222
        0x08000b0e:    2001        .       MOVS     r0,#1
        0x08000b10:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000b14:    e0b9        ..      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000b16:    e02c        ,.      B        0x8000b72 ; HAL_RCC_ClockConfig + 322
        0x08000b18:    9802        ..      LDR      r0,[sp,#8]
        0x08000b1a:    6840        @h      LDR      r0,[r0,#4]
        0x08000b1c:    2802        .(      CMP      r0,#2
        0x08000b1e:    d005        ..      BEQ      0x8000b2c ; HAL_RCC_ClockConfig + 252
        0x08000b20:    e7ff        ..      B        0x8000b22 ; HAL_RCC_ClockConfig + 242
        0x08000b22:    9802        ..      LDR      r0,[sp,#8]
        0x08000b24:    6840        @h      LDR      r0,[r0,#4]
        0x08000b26:    2803        .(      CMP      r0,#3
        0x08000b28:    d111        ..      BNE      0x8000b4e ; HAL_RCC_ClockConfig + 286
        0x08000b2a:    e7ff        ..      B        0x8000b2c ; HAL_RCC_ClockConfig + 252
        0x08000b2c:    f6430000    C...    MOVW     r0,#0x3800
        0x08000b30:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000b34:    6800        .h      LDR      r0,[r0,#0]
        0x08000b36:    f0007100    ...q    AND      r1,r0,#0x2000000
        0x08000b3a:    2000        .       MOVS     r0,#0
        0x08000b3c:    ebb06f51    ..Qo    CMP      r0,r1,LSR #25
        0x08000b40:    d104        ..      BNE      0x8000b4c ; HAL_RCC_ClockConfig + 284
        0x08000b42:    e7ff        ..      B        0x8000b44 ; HAL_RCC_ClockConfig + 276
        0x08000b44:    2001        .       MOVS     r0,#1
        0x08000b46:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000b4a:    e09e        ..      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000b4c:    e010        ..      B        0x8000b70 ; HAL_RCC_ClockConfig + 320
        0x08000b4e:    f6430000    C...    MOVW     r0,#0x3800
        0x08000b52:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000b56:    6800        .h      LDR      r0,[r0,#0]
        0x08000b58:    f0000102    ....    AND      r1,r0,#2
        0x08000b5c:    2000        .       MOVS     r0,#0
        0x08000b5e:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x08000b62:    d104        ..      BNE      0x8000b6e ; HAL_RCC_ClockConfig + 318
        0x08000b64:    e7ff        ..      B        0x8000b66 ; HAL_RCC_ClockConfig + 310
        0x08000b66:    2001        .       MOVS     r0,#1
        0x08000b68:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000b6c:    e08d        ..      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000b6e:    e7ff        ..      B        0x8000b70 ; HAL_RCC_ClockConfig + 320
        0x08000b70:    e7ff        ..      B        0x8000b72 ; HAL_RCC_ClockConfig + 322
        0x08000b72:    f6430108    C...    MOV      r1,#0x3808
        0x08000b76:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000b7a:    6808        .h      LDR      r0,[r1,#0]
        0x08000b7c:    f0200003     ...    BIC      r0,r0,#3
        0x08000b80:    9a02        ..      LDR      r2,[sp,#8]
        0x08000b82:    6852        Rh      LDR      r2,[r2,#4]
        0x08000b84:    4310        .C      ORRS     r0,r0,r2
        0x08000b86:    6008        .`      STR      r0,[r1,#0]
        0x08000b88:    f7fffe5a    ..Z.    BL       HAL_GetTick ; 0x8000840
        0x08000b8c:    9000        ..      STR      r0,[sp,#0]
        0x08000b8e:    e7ff        ..      B        0x8000b90 ; HAL_RCC_ClockConfig + 352
        0x08000b90:    f6430008    C...    MOV      r0,#0x3808
        0x08000b94:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000b98:    6800        .h      LDR      r0,[r0,#0]
        0x08000b9a:    f000000c    ....    AND      r0,r0,#0xc
        0x08000b9e:    9902        ..      LDR      r1,[sp,#8]
        0x08000ba0:    6849        Ih      LDR      r1,[r1,#4]
        0x08000ba2:    ebb00f81    ....    CMP      r0,r1,LSL #2
        0x08000ba6:    d00e        ..      BEQ      0x8000bc6 ; HAL_RCC_ClockConfig + 406
        0x08000ba8:    e7ff        ..      B        0x8000baa ; HAL_RCC_ClockConfig + 378
        0x08000baa:    f7fffe49    ..I.    BL       HAL_GetTick ; 0x8000840
        0x08000bae:    9900        ..      LDR      r1,[sp,#0]
        0x08000bb0:    1a40        @.      SUBS     r0,r0,r1
        0x08000bb2:    f2413189    A..1    MOV      r1,#0x1389
        0x08000bb6:    4288        .B      CMP      r0,r1
        0x08000bb8:    d304        ..      BCC      0x8000bc4 ; HAL_RCC_ClockConfig + 404
        0x08000bba:    e7ff        ..      B        0x8000bbc ; HAL_RCC_ClockConfig + 396
        0x08000bbc:    2003        .       MOVS     r0,#3
        0x08000bbe:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000bc2:    e062        b.      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000bc4:    e7e4        ..      B        0x8000b90 ; HAL_RCC_ClockConfig + 352
        0x08000bc6:    e7ff        ..      B        0x8000bc8 ; HAL_RCC_ClockConfig + 408
        0x08000bc8:    9801        ..      LDR      r0,[sp,#4]
        0x08000bca:    f6434100    C..A    MOVW     r1,#0x3c00
        0x08000bce:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000bd2:    6809        .h      LDR      r1,[r1,#0]
        0x08000bd4:    f0010107    ....    AND      r1,r1,#7
        0x08000bd8:    4288        .B      CMP      r0,r1
        0x08000bda:    d212        ..      BCS      0x8000c02 ; HAL_RCC_ClockConfig + 466
        0x08000bdc:    e7ff        ..      B        0x8000bde ; HAL_RCC_ClockConfig + 430
        0x08000bde:    9901        ..      LDR      r1,[sp,#4]
        0x08000be0:    f6434000    C..@    MOVW     r0,#0x3c00
        0x08000be4:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000be8:    7001        .p      STRB     r1,[r0,#0]
        0x08000bea:    6800        .h      LDR      r0,[r0,#0]
        0x08000bec:    f0000007    ....    AND      r0,r0,#7
        0x08000bf0:    9901        ..      LDR      r1,[sp,#4]
        0x08000bf2:    4288        .B      CMP      r0,r1
        0x08000bf4:    d004        ..      BEQ      0x8000c00 ; HAL_RCC_ClockConfig + 464
        0x08000bf6:    e7ff        ..      B        0x8000bf8 ; HAL_RCC_ClockConfig + 456
        0x08000bf8:    2001        .       MOVS     r0,#1
        0x08000bfa:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000bfe:    e044        D.      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000c00:    e7ff        ..      B        0x8000c02 ; HAL_RCC_ClockConfig + 466
        0x08000c02:    9802        ..      LDR      r0,[sp,#8]
        0x08000c04:    7800        .x      LDRB     r0,[r0,#0]
        0x08000c06:    0740        @.      LSLS     r0,r0,#29
        0x08000c08:    2800        .(      CMP      r0,#0
        0x08000c0a:    d50c        ..      BPL      0x8000c26 ; HAL_RCC_ClockConfig + 502
        0x08000c0c:    e7ff        ..      B        0x8000c0e ; HAL_RCC_ClockConfig + 478
        0x08000c0e:    f6430108    C...    MOV      r1,#0x3808
        0x08000c12:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000c16:    6808        .h      LDR      r0,[r1,#0]
        0x08000c18:    f42050e0     ..P    BIC      r0,r0,#0x1c00
        0x08000c1c:    9a02        ..      LDR      r2,[sp,#8]
        0x08000c1e:    68d2        .h      LDR      r2,[r2,#0xc]
        0x08000c20:    4310        .C      ORRS     r0,r0,r2
        0x08000c22:    6008        .`      STR      r0,[r1,#0]
        0x08000c24:    e7ff        ..      B        0x8000c26 ; HAL_RCC_ClockConfig + 502
        0x08000c26:    9802        ..      LDR      r0,[sp,#8]
        0x08000c28:    7800        .x      LDRB     r0,[r0,#0]
        0x08000c2a:    0700        ..      LSLS     r0,r0,#28
        0x08000c2c:    2800        .(      CMP      r0,#0
        0x08000c2e:    d50d        ..      BPL      0x8000c4c ; HAL_RCC_ClockConfig + 540
        0x08000c30:    e7ff        ..      B        0x8000c32 ; HAL_RCC_ClockConfig + 514
        0x08000c32:    f6430108    C...    MOV      r1,#0x3808
        0x08000c36:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000c3a:    6808        .h      LDR      r0,[r1,#0]
        0x08000c3c:    f4204060     .`@    BIC      r0,r0,#0xe000
        0x08000c40:    9a02        ..      LDR      r2,[sp,#8]
        0x08000c42:    6912        .i      LDR      r2,[r2,#0x10]
        0x08000c44:    ea4000c2    @...    ORR      r0,r0,r2,LSL #3
        0x08000c48:    6008        .`      STR      r0,[r1,#0]
        0x08000c4a:    e7ff        ..      B        0x8000c4c ; HAL_RCC_ClockConfig + 540
        0x08000c4c:    f000f822    ..".    BL       HAL_RCC_GetSysClockFreq ; 0x8000c94
        0x08000c50:    f6430108    C...    MOV      r1,#0x3808
        0x08000c54:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000c58:    6809        .h      LDR      r1,[r1,#0]
        0x08000c5a:    b2c9        ..      UXTB     r1,r1
        0x08000c5c:    090a        ..      LSRS     r2,r1,#4
        0x08000c5e:    f24171a0    A..q    MOVW     r1,#0x17a0
        0x08000c62:    f6c00100    ....    MOVT     r1,#0x800
        0x08000c66:    5c89        .\      LDRB     r1,[r1,r2]
        0x08000c68:    40c8        .@      LSRS     r0,r0,r1
        0x08000c6a:    f2400100    @...    MOVW     r1,#0
        0x08000c6e:    f2c20100    ....    MOVT     r1,#0x2000
        0x08000c72:    6008        .`      STR      r0,[r1,#0]
        0x08000c74:    f2400008    @...    MOVW     r0,#8
        0x08000c78:    f2c20000    ....    MOVT     r0,#0x2000
        0x08000c7c:    6800        .h      LDR      r0,[r0,#0]
        0x08000c7e:    f7fffe57    ..W.    BL       HAL_InitTick ; 0x8000930
        0x08000c82:    2000        .       MOVS     r0,#0
        0x08000c84:    f88d000f    ....    STRB     r0,[sp,#0xf]
        0x08000c88:    e7ff        ..      B        0x8000c8a ; HAL_RCC_ClockConfig + 602
        0x08000c8a:    f89d000f    ....    LDRB     r0,[sp,#0xf]
        0x08000c8e:    b004        ..      ADD      sp,sp,#0x10
        0x08000c90:    bd80        ..      POP      {r7,pc}
        0x08000c92:    0000        ..      MOVS     r0,r0
    HAL_RCC_GetSysClockFreq
        0x08000c94:    b580        ..      PUSH     {r7,lr}
        0x08000c96:    b086        ..      SUB      sp,sp,#0x18
        0x08000c98:    2000        .       MOVS     r0,#0
        0x08000c9a:    9005        ..      STR      r0,[sp,#0x14]
        0x08000c9c:    9004        ..      STR      r0,[sp,#0x10]
        0x08000c9e:    9003        ..      STR      r0,[sp,#0xc]
        0x08000ca0:    9002        ..      STR      r0,[sp,#8]
        0x08000ca2:    f6430008    C...    MOV      r0,#0x3808
        0x08000ca6:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000caa:    6800        .h      LDR      r0,[r0,#0]
        0x08000cac:    f000000c    ....    AND      r0,r0,#0xc
        0x08000cb0:    4601        .F      MOV      r1,r0
        0x08000cb2:    9101        ..      STR      r1,[sp,#4]
        0x08000cb4:    b140        @.      CBZ      r0,0x8000cc8 ; HAL_RCC_GetSysClockFreq + 52
        0x08000cb6:    e7ff        ..      B        0x8000cb8 ; HAL_RCC_GetSysClockFreq + 36
        0x08000cb8:    9801        ..      LDR      r0,[sp,#4]
        0x08000cba:    2804        .(      CMP      r0,#4
        0x08000cbc:    d00a        ..      BEQ      0x8000cd4 ; HAL_RCC_GetSysClockFreq + 64
        0x08000cbe:    e7ff        ..      B        0x8000cc0 ; HAL_RCC_GetSysClockFreq + 44
        0x08000cc0:    9801        ..      LDR      r0,[sp,#4]
        0x08000cc2:    2808        .(      CMP      r0,#8
        0x08000cc4:    d00c        ..      BEQ      0x8000ce0 ; HAL_RCC_GetSysClockFreq + 76
        0x08000cc6:    e04f        O.      B        0x8000d68 ; HAL_RCC_GetSysClockFreq + 212
        0x08000cc8:    f2424000    B..@    MOVW     r0,#0x2400
        0x08000ccc:    f2c000f4    ....    MOVT     r0,#0xf4
        0x08000cd0:    9002        ..      STR      r0,[sp,#8]
        0x08000cd2:    e04f        O.      B        0x8000d74 ; HAL_RCC_GetSysClockFreq + 224
        0x08000cd4:    f6470040    G.@.    MOV      r0,#0x7840
        0x08000cd8:    f2c0107d    ..}.    MOVT     r0,#0x17d
        0x08000cdc:    9002        ..      STR      r0,[sp,#8]
        0x08000cde:    e049        I.      B        0x8000d74 ; HAL_RCC_GetSysClockFreq + 224
        0x08000ce0:    f6430004    C...    MOV      r0,#0x3804
        0x08000ce4:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000ce8:    6801        .h      LDR      r1,[r0,#0]
        0x08000cea:    f001013f    ..?.    AND      r1,r1,#0x3f
        0x08000cee:    9105        ..      STR      r1,[sp,#0x14]
        0x08000cf0:    6800        .h      LDR      r0,[r0,#0]
        0x08000cf2:    0240        @.      LSLS     r0,r0,#9
        0x08000cf4:    2800        .(      CMP      r0,#0
        0x08000cf6:    d513        ..      BPL      0x8000d20 ; HAL_RCC_GetSysClockFreq + 140
        0x08000cf8:    e7ff        ..      B        0x8000cfa ; HAL_RCC_GetSysClockFreq + 102
        0x08000cfa:    f6430004    C...    MOV      r0,#0x3804
        0x08000cfe:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000d02:    6800        .h      LDR      r0,[r0,#0]
        0x08000d04:    f3c01088    ....    UBFX     r0,r0,#6,#9
        0x08000d08:    f6470140    G.@.    MOV      r1,#0x7840
        0x08000d0c:    f2c0117d    ..}.    MOVT     r1,#0x17d
        0x08000d10:    fba00101    ....    UMULL    r0,r1,r0,r1
        0x08000d14:    9a05        ..      LDR      r2,[sp,#0x14]
        0x08000d16:    2300        .#      MOVS     r3,#0
        0x08000d18:    f7fffac2    ....    BL       __aeabi_uldivmod ; 0x80002a0
        0x08000d1c:    9004        ..      STR      r0,[sp,#0x10]
        0x08000d1e:    e012        ..      B        0x8000d46 ; HAL_RCC_GetSysClockFreq + 178
        0x08000d20:    f6430004    C...    MOV      r0,#0x3804
        0x08000d24:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000d28:    6800        .h      LDR      r0,[r0,#0]
        0x08000d2a:    f3c01088    ....    UBFX     r0,r0,#6,#9
        0x08000d2e:    f2424100    B..A    MOVW     r1,#0x2400
        0x08000d32:    f2c001f4    ....    MOVT     r1,#0xf4
        0x08000d36:    fba00101    ....    UMULL    r0,r1,r0,r1
        0x08000d3a:    9a05        ..      LDR      r2,[sp,#0x14]
        0x08000d3c:    2300        .#      MOVS     r3,#0
        0x08000d3e:    f7fffaaf    ....    BL       __aeabi_uldivmod ; 0x80002a0
        0x08000d42:    9004        ..      STR      r0,[sp,#0x10]
        0x08000d44:    e7ff        ..      B        0x8000d46 ; HAL_RCC_GetSysClockFreq + 178
        0x08000d46:    f6430004    C...    MOV      r0,#0x3804
        0x08000d4a:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000d4e:    6800        .h      LDR      r0,[r0,#0]
        0x08000d50:    f4003140    ..@1    AND      r1,r0,#0x30000
        0x08000d54:    2002        .       MOVS     r0,#2
        0x08000d56:    eb0030d1    ...0    ADD      r0,r0,r1,LSR #15
        0x08000d5a:    9003        ..      STR      r0,[sp,#0xc]
        0x08000d5c:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000d5e:    9903        ..      LDR      r1,[sp,#0xc]
        0x08000d60:    fbb0f0f1    ....    UDIV     r0,r0,r1
        0x08000d64:    9002        ..      STR      r0,[sp,#8]
        0x08000d66:    e005        ..      B        0x8000d74 ; HAL_RCC_GetSysClockFreq + 224
        0x08000d68:    f2424000    B..@    MOVW     r0,#0x2400
        0x08000d6c:    f2c000f4    ....    MOVT     r0,#0xf4
        0x08000d70:    9002        ..      STR      r0,[sp,#8]
        0x08000d72:    e7ff        ..      B        0x8000d74 ; HAL_RCC_GetSysClockFreq + 224
        0x08000d74:    9802        ..      LDR      r0,[sp,#8]
        0x08000d76:    b006        ..      ADD      sp,sp,#0x18
        0x08000d78:    bd80        ..      POP      {r7,pc}
        0x08000d7a:    0000        ..      MOVS     r0,r0
    HAL_RCC_OscConfig
        0x08000d7c:    b580        ..      PUSH     {r7,lr}
        0x08000d7e:    b086        ..      SUB      sp,sp,#0x18
        0x08000d80:    9004        ..      STR      r0,[sp,#0x10]
        0x08000d82:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000d84:    b920         .      CBNZ     r0,0x8000d90 ; HAL_RCC_OscConfig + 20
        0x08000d86:    e7ff        ..      B        0x8000d88 ; HAL_RCC_OscConfig + 12
        0x08000d88:    2001        .       MOVS     r0,#1
        0x08000d8a:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08000d8e:    e350        P.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08000d90:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000d92:    7800        .x      LDRB     r0,[r0,#0]
        0x08000d94:    07c0        ..      LSLS     r0,r0,#31
        0x08000d96:    2800        .(      CMP      r0,#0
        0x08000d98:    f00080a5    ....    BEQ.W    0x8000ee6 ; HAL_RCC_OscConfig + 362
        0x08000d9c:    e7ff        ..      B        0x8000d9e ; HAL_RCC_OscConfig + 34
        0x08000d9e:    f6430008    C...    MOV      r0,#0x3808
        0x08000da2:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000da6:    6800        .h      LDR      r0,[r0,#0]
        0x08000da8:    f000000c    ....    AND      r0,r0,#0xc
        0x08000dac:    2804        .(      CMP      r0,#4
        0x08000dae:    d013        ..      BEQ      0x8000dd8 ; HAL_RCC_OscConfig + 92
        0x08000db0:    e7ff        ..      B        0x8000db2 ; HAL_RCC_OscConfig + 54
        0x08000db2:    f6430008    C...    MOV      r0,#0x3808
        0x08000db6:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000dba:    6800        .h      LDR      r0,[r0,#0]
        0x08000dbc:    f000000c    ....    AND      r0,r0,#0xc
        0x08000dc0:    2808        .(      CMP      r0,#8
        0x08000dc2:    d11e        ..      BNE      0x8000e02 ; HAL_RCC_OscConfig + 134
        0x08000dc4:    e7ff        ..      B        0x8000dc6 ; HAL_RCC_OscConfig + 74
        0x08000dc6:    f6430004    C...    MOV      r0,#0x3804
        0x08000dca:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000dce:    6800        .h      LDR      r0,[r0,#0]
        0x08000dd0:    0240        @.      LSLS     r0,r0,#9
        0x08000dd2:    2800        .(      CMP      r0,#0
        0x08000dd4:    d515        ..      BPL      0x8000e02 ; HAL_RCC_OscConfig + 134
        0x08000dd6:    e7ff        ..      B        0x8000dd8 ; HAL_RCC_OscConfig + 92
        0x08000dd8:    f6430000    C...    MOVW     r0,#0x3800
        0x08000ddc:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000de0:    6800        .h      LDR      r0,[r0,#0]
        0x08000de2:    f4003100    ...1    AND      r1,r0,#0x20000
        0x08000de6:    2000        .       MOVS     r0,#0
        0x08000de8:    ebb04f51    ..QO    CMP      r0,r1,LSR #17
        0x08000dec:    d008        ..      BEQ      0x8000e00 ; HAL_RCC_OscConfig + 132
        0x08000dee:    e7ff        ..      B        0x8000df0 ; HAL_RCC_OscConfig + 116
        0x08000df0:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000df2:    6840        @h      LDR      r0,[r0,#4]
        0x08000df4:    b920         .      CBNZ     r0,0x8000e00 ; HAL_RCC_OscConfig + 132
        0x08000df6:    e7ff        ..      B        0x8000df8 ; HAL_RCC_OscConfig + 124
        0x08000df8:    2001        .       MOVS     r0,#1
        0x08000dfa:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08000dfe:    e318        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08000e00:    e070        p.      B        0x8000ee4 ; HAL_RCC_OscConfig + 360
        0x08000e02:    e7ff        ..      B        0x8000e04 ; HAL_RCC_OscConfig + 136
        0x08000e04:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000e06:    6840        @h      LDR      r0,[r0,#4]
        0x08000e08:    f5b03f80    ...?    CMP      r0,#0x10000
        0x08000e0c:    d109        ..      BNE      0x8000e22 ; HAL_RCC_OscConfig + 166
        0x08000e0e:    e7ff        ..      B        0x8000e10 ; HAL_RCC_OscConfig + 148
        0x08000e10:    f6430100    C...    MOVW     r1,#0x3800
        0x08000e14:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000e18:    6808        .h      LDR      r0,[r1,#0]
        0x08000e1a:    f4403080    @..0    ORR      r0,r0,#0x10000
        0x08000e1e:    6008        .`      STR      r0,[r1,#0]
        0x08000e20:    e020         .      B        0x8000e64 ; HAL_RCC_OscConfig + 232
        0x08000e22:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000e24:    6840        @h      LDR      r0,[r0,#4]
        0x08000e26:    f5b02fa0    .../    CMP      r0,#0x50000
        0x08000e2a:    d10d        ..      BNE      0x8000e48 ; HAL_RCC_OscConfig + 204
        0x08000e2c:    e7ff        ..      B        0x8000e2e ; HAL_RCC_OscConfig + 178
        0x08000e2e:    f6430100    C...    MOVW     r1,#0x3800
        0x08000e32:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000e36:    6808        .h      LDR      r0,[r1,#0]
        0x08000e38:    f4402080    @..     ORR      r0,r0,#0x40000
        0x08000e3c:    6008        .`      STR      r0,[r1,#0]
        0x08000e3e:    6808        .h      LDR      r0,[r1,#0]
        0x08000e40:    f4403080    @..0    ORR      r0,r0,#0x10000
        0x08000e44:    6008        .`      STR      r0,[r1,#0]
        0x08000e46:    e00c        ..      B        0x8000e62 ; HAL_RCC_OscConfig + 230
        0x08000e48:    f6430100    C...    MOVW     r1,#0x3800
        0x08000e4c:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000e50:    6808        .h      LDR      r0,[r1,#0]
        0x08000e52:    f4203080     ..0    BIC      r0,r0,#0x10000
        0x08000e56:    6008        .`      STR      r0,[r1,#0]
        0x08000e58:    6808        .h      LDR      r0,[r1,#0]
        0x08000e5a:    f4202080     ..     BIC      r0,r0,#0x40000
        0x08000e5e:    6008        .`      STR      r0,[r1,#0]
        0x08000e60:    e7ff        ..      B        0x8000e62 ; HAL_RCC_OscConfig + 230
        0x08000e62:    e7ff        ..      B        0x8000e64 ; HAL_RCC_OscConfig + 232
        0x08000e64:    e7ff        ..      B        0x8000e66 ; HAL_RCC_OscConfig + 234
        0x08000e66:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000e68:    6840        @h      LDR      r0,[r0,#4]
        0x08000e6a:    b1e8        ..      CBZ      r0,0x8000ea8 ; HAL_RCC_OscConfig + 300
        0x08000e6c:    e7ff        ..      B        0x8000e6e ; HAL_RCC_OscConfig + 242
        0x08000e6e:    f7fffce7    ....    BL       HAL_GetTick ; 0x8000840
        0x08000e72:    9003        ..      STR      r0,[sp,#0xc]
        0x08000e74:    e7ff        ..      B        0x8000e76 ; HAL_RCC_OscConfig + 250
        0x08000e76:    f6430000    C...    MOVW     r0,#0x3800
        0x08000e7a:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000e7e:    6800        .h      LDR      r0,[r0,#0]
        0x08000e80:    f4003100    ...1    AND      r1,r0,#0x20000
        0x08000e84:    2000        .       MOVS     r0,#0
        0x08000e86:    ebb04f51    ..QO    CMP      r0,r1,LSR #17
        0x08000e8a:    d10c        ..      BNE      0x8000ea6 ; HAL_RCC_OscConfig + 298
        0x08000e8c:    e7ff        ..      B        0x8000e8e ; HAL_RCC_OscConfig + 274
        0x08000e8e:    f7fffcd7    ....    BL       HAL_GetTick ; 0x8000840
        0x08000e92:    9903        ..      LDR      r1,[sp,#0xc]
        0x08000e94:    1a40        @.      SUBS     r0,r0,r1
        0x08000e96:    2865        e(      CMP      r0,#0x65
        0x08000e98:    d304        ..      BCC      0x8000ea4 ; HAL_RCC_OscConfig + 296
        0x08000e9a:    e7ff        ..      B        0x8000e9c ; HAL_RCC_OscConfig + 288
        0x08000e9c:    2003        .       MOVS     r0,#3
        0x08000e9e:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08000ea2:    e2c6        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08000ea4:    e7e7        ..      B        0x8000e76 ; HAL_RCC_OscConfig + 250
        0x08000ea6:    e01c        ..      B        0x8000ee2 ; HAL_RCC_OscConfig + 358
        0x08000ea8:    f7fffcca    ....    BL       HAL_GetTick ; 0x8000840
        0x08000eac:    9003        ..      STR      r0,[sp,#0xc]
        0x08000eae:    e7ff        ..      B        0x8000eb0 ; HAL_RCC_OscConfig + 308
        0x08000eb0:    f6430000    C...    MOVW     r0,#0x3800
        0x08000eb4:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000eb8:    6800        .h      LDR      r0,[r0,#0]
        0x08000eba:    f4003100    ...1    AND      r1,r0,#0x20000
        0x08000ebe:    2000        .       MOVS     r0,#0
        0x08000ec0:    ebb04f51    ..QO    CMP      r0,r1,LSR #17
        0x08000ec4:    d00c        ..      BEQ      0x8000ee0 ; HAL_RCC_OscConfig + 356
        0x08000ec6:    e7ff        ..      B        0x8000ec8 ; HAL_RCC_OscConfig + 332
        0x08000ec8:    f7fffcba    ....    BL       HAL_GetTick ; 0x8000840
        0x08000ecc:    9903        ..      LDR      r1,[sp,#0xc]
        0x08000ece:    1a40        @.      SUBS     r0,r0,r1
        0x08000ed0:    2865        e(      CMP      r0,#0x65
        0x08000ed2:    d304        ..      BCC      0x8000ede ; HAL_RCC_OscConfig + 354
        0x08000ed4:    e7ff        ..      B        0x8000ed6 ; HAL_RCC_OscConfig + 346
        0x08000ed6:    2003        .       MOVS     r0,#3
        0x08000ed8:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08000edc:    e2a9        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08000ede:    e7e7        ..      B        0x8000eb0 ; HAL_RCC_OscConfig + 308
        0x08000ee0:    e7ff        ..      B        0x8000ee2 ; HAL_RCC_OscConfig + 358
        0x08000ee2:    e7ff        ..      B        0x8000ee4 ; HAL_RCC_OscConfig + 360
        0x08000ee4:    e7ff        ..      B        0x8000ee6 ; HAL_RCC_OscConfig + 362
        0x08000ee6:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000ee8:    7800        .x      LDRB     r0,[r0,#0]
        0x08000eea:    0780        ..      LSLS     r0,r0,#30
        0x08000eec:    2800        .(      CMP      r0,#0
        0x08000eee:    f1408096    @...    BPL.W    0x800101e ; HAL_RCC_OscConfig + 674
        0x08000ef2:    e7ff        ..      B        0x8000ef4 ; HAL_RCC_OscConfig + 376
        0x08000ef4:    f6430008    C...    MOV      r0,#0x3808
        0x08000ef8:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000efc:    6800        .h      LDR      r0,[r0,#0]
        0x08000efe:    f0100f0c    ....    TST      r0,#0xc
        0x08000f02:    d013        ..      BEQ      0x8000f2c ; HAL_RCC_OscConfig + 432
        0x08000f04:    e7ff        ..      B        0x8000f06 ; HAL_RCC_OscConfig + 394
        0x08000f06:    f6430008    C...    MOV      r0,#0x3808
        0x08000f0a:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000f0e:    6800        .h      LDR      r0,[r0,#0]
        0x08000f10:    f000000c    ....    AND      r0,r0,#0xc
        0x08000f14:    2808        .(      CMP      r0,#8
        0x08000f16:    d12c        ,.      BNE      0x8000f72 ; HAL_RCC_OscConfig + 502
        0x08000f18:    e7ff        ..      B        0x8000f1a ; HAL_RCC_OscConfig + 414
        0x08000f1a:    f6430004    C...    MOV      r0,#0x3804
        0x08000f1e:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000f22:    6800        .h      LDR      r0,[r0,#0]
        0x08000f24:    0240        @.      LSLS     r0,r0,#9
        0x08000f26:    2800        .(      CMP      r0,#0
        0x08000f28:    d423        #.      BMI      0x8000f72 ; HAL_RCC_OscConfig + 502
        0x08000f2a:    e7ff        ..      B        0x8000f2c ; HAL_RCC_OscConfig + 432
        0x08000f2c:    f6430000    C...    MOVW     r0,#0x3800
        0x08000f30:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000f34:    6800        .h      LDR      r0,[r0,#0]
        0x08000f36:    f0000102    ....    AND      r1,r0,#2
        0x08000f3a:    2000        .       MOVS     r0,#0
        0x08000f3c:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x08000f40:    d009        ..      BEQ      0x8000f56 ; HAL_RCC_OscConfig + 474
        0x08000f42:    e7ff        ..      B        0x8000f44 ; HAL_RCC_OscConfig + 456
        0x08000f44:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000f46:    68c0        .h      LDR      r0,[r0,#0xc]
        0x08000f48:    2801        .(      CMP      r0,#1
        0x08000f4a:    d004        ..      BEQ      0x8000f56 ; HAL_RCC_OscConfig + 474
        0x08000f4c:    e7ff        ..      B        0x8000f4e ; HAL_RCC_OscConfig + 466
        0x08000f4e:    2001        .       MOVS     r0,#1
        0x08000f50:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08000f54:    e26d        m.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08000f56:    f6430100    C...    MOVW     r1,#0x3800
        0x08000f5a:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000f5e:    6808        .h      LDR      r0,[r1,#0]
        0x08000f60:    f02000f8     ...    BIC      r0,r0,#0xf8
        0x08000f64:    9a04        ..      LDR      r2,[sp,#0x10]
        0x08000f66:    6912        .i      LDR      r2,[r2,#0x10]
        0x08000f68:    ea4000c2    @...    ORR      r0,r0,r2,LSL #3
        0x08000f6c:    6008        .`      STR      r0,[r1,#0]
        0x08000f6e:    e7ff        ..      B        0x8000f70 ; HAL_RCC_OscConfig + 500
        0x08000f70:    e054        T.      B        0x800101c ; HAL_RCC_OscConfig + 672
        0x08000f72:    9804        ..      LDR      r0,[sp,#0x10]
        0x08000f74:    68c0        .h      LDR      r0,[r0,#0xc]
        0x08000f76:    b370        p.      CBZ      r0,0x8000fd6 ; HAL_RCC_OscConfig + 602
        0x08000f78:    e7ff        ..      B        0x8000f7a ; HAL_RCC_OscConfig + 510
        0x08000f7a:    2100        .!      MOVS     r1,#0
        0x08000f7c:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x08000f80:    2001        .       MOVS     r0,#1
        0x08000f82:    6008        .`      STR      r0,[r1,#0]
        0x08000f84:    f7fffc5c    ..\.    BL       HAL_GetTick ; 0x8000840
        0x08000f88:    9003        ..      STR      r0,[sp,#0xc]
        0x08000f8a:    e7ff        ..      B        0x8000f8c ; HAL_RCC_OscConfig + 528
        0x08000f8c:    f6430000    C...    MOVW     r0,#0x3800
        0x08000f90:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000f94:    6800        .h      LDR      r0,[r0,#0]
        0x08000f96:    f0000102    ....    AND      r1,r0,#2
        0x08000f9a:    2000        .       MOVS     r0,#0
        0x08000f9c:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x08000fa0:    d10c        ..      BNE      0x8000fbc ; HAL_RCC_OscConfig + 576
        0x08000fa2:    e7ff        ..      B        0x8000fa4 ; HAL_RCC_OscConfig + 552
        0x08000fa4:    f7fffc4c    ..L.    BL       HAL_GetTick ; 0x8000840
        0x08000fa8:    9903        ..      LDR      r1,[sp,#0xc]
        0x08000faa:    1a40        @.      SUBS     r0,r0,r1
        0x08000fac:    2803        .(      CMP      r0,#3
        0x08000fae:    d304        ..      BCC      0x8000fba ; HAL_RCC_OscConfig + 574
        0x08000fb0:    e7ff        ..      B        0x8000fb2 ; HAL_RCC_OscConfig + 566
        0x08000fb2:    2003        .       MOVS     r0,#3
        0x08000fb4:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08000fb8:    e23b        ;.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08000fba:    e7e7        ..      B        0x8000f8c ; HAL_RCC_OscConfig + 528
        0x08000fbc:    f6430100    C...    MOVW     r1,#0x3800
        0x08000fc0:    f2c40102    ....    MOVT     r1,#0x4002
        0x08000fc4:    6808        .h      LDR      r0,[r1,#0]
        0x08000fc6:    f02000f8     ...    BIC      r0,r0,#0xf8
        0x08000fca:    9a04        ..      LDR      r2,[sp,#0x10]
        0x08000fcc:    6912        .i      LDR      r2,[r2,#0x10]
        0x08000fce:    ea4000c2    @...    ORR      r0,r0,r2,LSL #3
        0x08000fd2:    6008        .`      STR      r0,[r1,#0]
        0x08000fd4:    e021        !.      B        0x800101a ; HAL_RCC_OscConfig + 670
        0x08000fd6:    2100        .!      MOVS     r1,#0
        0x08000fd8:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x08000fdc:    2000        .       MOVS     r0,#0
        0x08000fde:    6008        .`      STR      r0,[r1,#0]
        0x08000fe0:    f7fffc2e    ....    BL       HAL_GetTick ; 0x8000840
        0x08000fe4:    9003        ..      STR      r0,[sp,#0xc]
        0x08000fe6:    e7ff        ..      B        0x8000fe8 ; HAL_RCC_OscConfig + 620
        0x08000fe8:    f6430000    C...    MOVW     r0,#0x3800
        0x08000fec:    f2c40002    ....    MOVT     r0,#0x4002
        0x08000ff0:    6800        .h      LDR      r0,[r0,#0]
        0x08000ff2:    f0000102    ....    AND      r1,r0,#2
        0x08000ff6:    2000        .       MOVS     r0,#0
        0x08000ff8:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x08000ffc:    d00c        ..      BEQ      0x8001018 ; HAL_RCC_OscConfig + 668
        0x08000ffe:    e7ff        ..      B        0x8001000 ; HAL_RCC_OscConfig + 644
        0x08001000:    f7fffc1e    ....    BL       HAL_GetTick ; 0x8000840
        0x08001004:    9903        ..      LDR      r1,[sp,#0xc]
        0x08001006:    1a40        @.      SUBS     r0,r0,r1
        0x08001008:    2803        .(      CMP      r0,#3
        0x0800100a:    d304        ..      BCC      0x8001016 ; HAL_RCC_OscConfig + 666
        0x0800100c:    e7ff        ..      B        0x800100e ; HAL_RCC_OscConfig + 658
        0x0800100e:    2003        .       MOVS     r0,#3
        0x08001010:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001014:    e20d        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001016:    e7e7        ..      B        0x8000fe8 ; HAL_RCC_OscConfig + 620
        0x08001018:    e7ff        ..      B        0x800101a ; HAL_RCC_OscConfig + 670
        0x0800101a:    e7ff        ..      B        0x800101c ; HAL_RCC_OscConfig + 672
        0x0800101c:    e7ff        ..      B        0x800101e ; HAL_RCC_OscConfig + 674
        0x0800101e:    9804        ..      LDR      r0,[sp,#0x10]
        0x08001020:    7800        .x      LDRB     r0,[r0,#0]
        0x08001022:    0700        ..      LSLS     r0,r0,#28
        0x08001024:    2800        .(      CMP      r0,#0
        0x08001026:    d54b        K.      BPL      0x80010c0 ; HAL_RCC_OscConfig + 836
        0x08001028:    e7ff        ..      B        0x800102a ; HAL_RCC_OscConfig + 686
        0x0800102a:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800102c:    6940        @i      LDR      r0,[r0,#0x14]
        0x0800102e:    b318        ..      CBZ      r0,0x8001078 ; HAL_RCC_OscConfig + 764
        0x08001030:    e7ff        ..      B        0x8001032 ; HAL_RCC_OscConfig + 694
        0x08001032:    f6406180    @..a    MOVW     r1,#0xe80
        0x08001036:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x0800103a:    2001        .       MOVS     r0,#1
        0x0800103c:    6008        .`      STR      r0,[r1,#0]
        0x0800103e:    f7fffbff    ....    BL       HAL_GetTick ; 0x8000840
        0x08001042:    9003        ..      STR      r0,[sp,#0xc]
        0x08001044:    e7ff        ..      B        0x8001046 ; HAL_RCC_OscConfig + 714
        0x08001046:    f6430074    C.t.    MOV      r0,#0x3874
        0x0800104a:    f2c40002    ....    MOVT     r0,#0x4002
        0x0800104e:    6800        .h      LDR      r0,[r0,#0]
        0x08001050:    f0000102    ....    AND      r1,r0,#2
        0x08001054:    2000        .       MOVS     r0,#0
        0x08001056:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x0800105a:    d10c        ..      BNE      0x8001076 ; HAL_RCC_OscConfig + 762
        0x0800105c:    e7ff        ..      B        0x800105e ; HAL_RCC_OscConfig + 738
        0x0800105e:    f7fffbef    ....    BL       HAL_GetTick ; 0x8000840
        0x08001062:    9903        ..      LDR      r1,[sp,#0xc]
        0x08001064:    1a40        @.      SUBS     r0,r0,r1
        0x08001066:    2803        .(      CMP      r0,#3
        0x08001068:    d304        ..      BCC      0x8001074 ; HAL_RCC_OscConfig + 760
        0x0800106a:    e7ff        ..      B        0x800106c ; HAL_RCC_OscConfig + 752
        0x0800106c:    2003        .       MOVS     r0,#3
        0x0800106e:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001072:    e1de        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001074:    e7e7        ..      B        0x8001046 ; HAL_RCC_OscConfig + 714
        0x08001076:    e022        ".      B        0x80010be ; HAL_RCC_OscConfig + 834
        0x08001078:    f6406180    @..a    MOVW     r1,#0xe80
        0x0800107c:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x08001080:    2000        .       MOVS     r0,#0
        0x08001082:    6008        .`      STR      r0,[r1,#0]
        0x08001084:    f7fffbdc    ....    BL       HAL_GetTick ; 0x8000840
        0x08001088:    9003        ..      STR      r0,[sp,#0xc]
        0x0800108a:    e7ff        ..      B        0x800108c ; HAL_RCC_OscConfig + 784
        0x0800108c:    f6430074    C.t.    MOV      r0,#0x3874
        0x08001090:    f2c40002    ....    MOVT     r0,#0x4002
        0x08001094:    6800        .h      LDR      r0,[r0,#0]
        0x08001096:    f0000102    ....    AND      r1,r0,#2
        0x0800109a:    2000        .       MOVS     r0,#0
        0x0800109c:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x080010a0:    d00c        ..      BEQ      0x80010bc ; HAL_RCC_OscConfig + 832
        0x080010a2:    e7ff        ..      B        0x80010a4 ; HAL_RCC_OscConfig + 808
        0x080010a4:    f7fffbcc    ....    BL       HAL_GetTick ; 0x8000840
        0x080010a8:    9903        ..      LDR      r1,[sp,#0xc]
        0x080010aa:    1a40        @.      SUBS     r0,r0,r1
        0x080010ac:    2803        .(      CMP      r0,#3
        0x080010ae:    d304        ..      BCC      0x80010ba ; HAL_RCC_OscConfig + 830
        0x080010b0:    e7ff        ..      B        0x80010b2 ; HAL_RCC_OscConfig + 822
        0x080010b2:    2003        .       MOVS     r0,#3
        0x080010b4:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x080010b8:    e1bb        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x080010ba:    e7e7        ..      B        0x800108c ; HAL_RCC_OscConfig + 784
        0x080010bc:    e7ff        ..      B        0x80010be ; HAL_RCC_OscConfig + 834
        0x080010be:    e7ff        ..      B        0x80010c0 ; HAL_RCC_OscConfig + 836
        0x080010c0:    9804        ..      LDR      r0,[sp,#0x10]
        0x080010c2:    7800        .x      LDRB     r0,[r0,#0]
        0x080010c4:    0740        @.      LSLS     r0,r0,#29
        0x080010c6:    2800        .(      CMP      r0,#0
        0x080010c8:    f14080ce    @...    BPL.W    0x8001268 ; HAL_RCC_OscConfig + 1260
        0x080010cc:    e7ff        ..      B        0x80010ce ; HAL_RCC_OscConfig + 850
        0x080010ce:    2000        .       MOVS     r0,#0
        0x080010d0:    f88d0007    ....    STRB     r0,[sp,#7]
        0x080010d4:    f6430040    C.@.    MOVW     r0,#0x3840
        0x080010d8:    f2c40002    ....    MOVT     r0,#0x4002
        0x080010dc:    6800        .h      LDR      r0,[r0,#0]
        0x080010de:    00c0        ..      LSLS     r0,r0,#3
        0x080010e0:    2800        .(      CMP      r0,#0
        0x080010e2:    d415        ..      BMI      0x8001110 ; HAL_RCC_OscConfig + 916
        0x080010e4:    e7ff        ..      B        0x80010e6 ; HAL_RCC_OscConfig + 874
        0x080010e6:    e7ff        ..      B        0x80010e8 ; HAL_RCC_OscConfig + 876
        0x080010e8:    2000        .       MOVS     r0,#0
        0x080010ea:    9000        ..      STR      r0,[sp,#0]
        0x080010ec:    f6430040    C.@.    MOVW     r0,#0x3840
        0x080010f0:    f2c40002    ....    MOVT     r0,#0x4002
        0x080010f4:    6801        .h      LDR      r1,[r0,#0]
        0x080010f6:    f0415180    A..Q    ORR      r1,r1,#0x10000000
        0x080010fa:    6001        .`      STR      r1,[r0,#0]
        0x080010fc:    6800        .h      LDR      r0,[r0,#0]
        0x080010fe:    f0005080    ...P    AND      r0,r0,#0x10000000
        0x08001102:    9000        ..      STR      r0,[sp,#0]
        0x08001104:    9800        ..      LDR      r0,[sp,#0]
        0x08001106:    e7ff        ..      B        0x8001108 ; HAL_RCC_OscConfig + 908
        0x08001108:    2001        .       MOVS     r0,#1
        0x0800110a:    f88d0007    ....    STRB     r0,[sp,#7]
        0x0800110e:    e7ff        ..      B        0x8001110 ; HAL_RCC_OscConfig + 916
        0x08001110:    f2470000    G...    MOVW     r0,#0x7000
        0x08001114:    f2c40000    ....    MOVT     r0,#0x4000
        0x08001118:    6800        .h      LDR      r0,[r0,#0]
        0x0800111a:    05c0        ..      LSLS     r0,r0,#23
        0x0800111c:    2800        .(      CMP      r0,#0
        0x0800111e:    d422        ".      BMI      0x8001166 ; HAL_RCC_OscConfig + 1002
        0x08001120:    e7ff        ..      B        0x8001122 ; HAL_RCC_OscConfig + 934
        0x08001122:    f2470100    G...    MOVW     r1,#0x7000
        0x08001126:    f2c40100    ....    MOVT     r1,#0x4000
        0x0800112a:    6808        .h      LDR      r0,[r1,#0]
        0x0800112c:    f4407080    @..p    ORR      r0,r0,#0x100
        0x08001130:    6008        .`      STR      r0,[r1,#0]
        0x08001132:    f7fffb85    ....    BL       HAL_GetTick ; 0x8000840
        0x08001136:    9003        ..      STR      r0,[sp,#0xc]
        0x08001138:    e7ff        ..      B        0x800113a ; HAL_RCC_OscConfig + 958
        0x0800113a:    f2470000    G...    MOVW     r0,#0x7000
        0x0800113e:    f2c40000    ....    MOVT     r0,#0x4000
        0x08001142:    6800        .h      LDR      r0,[r0,#0]
        0x08001144:    05c0        ..      LSLS     r0,r0,#23
        0x08001146:    2800        .(      CMP      r0,#0
        0x08001148:    d40c        ..      BMI      0x8001164 ; HAL_RCC_OscConfig + 1000
        0x0800114a:    e7ff        ..      B        0x800114c ; HAL_RCC_OscConfig + 976
        0x0800114c:    f7fffb78    ..x.    BL       HAL_GetTick ; 0x8000840
        0x08001150:    9903        ..      LDR      r1,[sp,#0xc]
        0x08001152:    1a40        @.      SUBS     r0,r0,r1
        0x08001154:    2803        .(      CMP      r0,#3
        0x08001156:    d304        ..      BCC      0x8001162 ; HAL_RCC_OscConfig + 998
        0x08001158:    e7ff        ..      B        0x800115a ; HAL_RCC_OscConfig + 990
        0x0800115a:    2003        .       MOVS     r0,#3
        0x0800115c:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001160:    e167        g.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001162:    e7ea        ..      B        0x800113a ; HAL_RCC_OscConfig + 958
        0x08001164:    e7ff        ..      B        0x8001166 ; HAL_RCC_OscConfig + 1002
        0x08001166:    e7ff        ..      B        0x8001168 ; HAL_RCC_OscConfig + 1004
        0x08001168:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800116a:    6880        .h      LDR      r0,[r0,#8]
        0x0800116c:    2801        .(      CMP      r0,#1
        0x0800116e:    d109        ..      BNE      0x8001184 ; HAL_RCC_OscConfig + 1032
        0x08001170:    e7ff        ..      B        0x8001172 ; HAL_RCC_OscConfig + 1014
        0x08001172:    f6430170    C.p.    MOV      r1,#0x3870
        0x08001176:    f2c40102    ....    MOVT     r1,#0x4002
        0x0800117a:    6808        .h      LDR      r0,[r1,#0]
        0x0800117c:    f0400001    @...    ORR      r0,r0,#1
        0x08001180:    6008        .`      STR      r0,[r1,#0]
        0x08001182:    e01f        ..      B        0x80011c4 ; HAL_RCC_OscConfig + 1096
        0x08001184:    9804        ..      LDR      r0,[sp,#0x10]
        0x08001186:    6880        .h      LDR      r0,[r0,#8]
        0x08001188:    2805        .(      CMP      r0,#5
        0x0800118a:    d10d        ..      BNE      0x80011a8 ; HAL_RCC_OscConfig + 1068
        0x0800118c:    e7ff        ..      B        0x800118e ; HAL_RCC_OscConfig + 1042
        0x0800118e:    f6430170    C.p.    MOV      r1,#0x3870
        0x08001192:    f2c40102    ....    MOVT     r1,#0x4002
        0x08001196:    6808        .h      LDR      r0,[r1,#0]
        0x08001198:    f0400004    @...    ORR      r0,r0,#4
        0x0800119c:    6008        .`      STR      r0,[r1,#0]
        0x0800119e:    6808        .h      LDR      r0,[r1,#0]
        0x080011a0:    f0400001    @...    ORR      r0,r0,#1
        0x080011a4:    6008        .`      STR      r0,[r1,#0]
        0x080011a6:    e00c        ..      B        0x80011c2 ; HAL_RCC_OscConfig + 1094
        0x080011a8:    f6430170    C.p.    MOV      r1,#0x3870
        0x080011ac:    f2c40102    ....    MOVT     r1,#0x4002
        0x080011b0:    6808        .h      LDR      r0,[r1,#0]
        0x080011b2:    f0200001     ...    BIC      r0,r0,#1
        0x080011b6:    6008        .`      STR      r0,[r1,#0]
        0x080011b8:    6808        .h      LDR      r0,[r1,#0]
        0x080011ba:    f0200004     ...    BIC      r0,r0,#4
        0x080011be:    6008        .`      STR      r0,[r1,#0]
        0x080011c0:    e7ff        ..      B        0x80011c2 ; HAL_RCC_OscConfig + 1094
        0x080011c2:    e7ff        ..      B        0x80011c4 ; HAL_RCC_OscConfig + 1096
        0x080011c4:    e7ff        ..      B        0x80011c6 ; HAL_RCC_OscConfig + 1098
        0x080011c6:    9804        ..      LDR      r0,[sp,#0x10]
        0x080011c8:    6880        .h      LDR      r0,[r0,#8]
        0x080011ca:    b1f8        ..      CBZ      r0,0x800120c ; HAL_RCC_OscConfig + 1168
        0x080011cc:    e7ff        ..      B        0x80011ce ; HAL_RCC_OscConfig + 1106
        0x080011ce:    f7fffb37    ..7.    BL       HAL_GetTick ; 0x8000840
        0x080011d2:    9003        ..      STR      r0,[sp,#0xc]
        0x080011d4:    e7ff        ..      B        0x80011d6 ; HAL_RCC_OscConfig + 1114
        0x080011d6:    f6430070    C.p.    MOV      r0,#0x3870
        0x080011da:    f2c40002    ....    MOVT     r0,#0x4002
        0x080011de:    6800        .h      LDR      r0,[r0,#0]
        0x080011e0:    f0000102    ....    AND      r1,r0,#2
        0x080011e4:    2000        .       MOVS     r0,#0
        0x080011e6:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x080011ea:    d10e        ..      BNE      0x800120a ; HAL_RCC_OscConfig + 1166
        0x080011ec:    e7ff        ..      B        0x80011ee ; HAL_RCC_OscConfig + 1138
        0x080011ee:    f7fffb27    ..'.    BL       HAL_GetTick ; 0x8000840
        0x080011f2:    9903        ..      LDR      r1,[sp,#0xc]
        0x080011f4:    1a40        @.      SUBS     r0,r0,r1
        0x080011f6:    f2413189    A..1    MOV      r1,#0x1389
        0x080011fa:    4288        .B      CMP      r0,r1
        0x080011fc:    d304        ..      BCC      0x8001208 ; HAL_RCC_OscConfig + 1164
        0x080011fe:    e7ff        ..      B        0x8001200 ; HAL_RCC_OscConfig + 1156
        0x08001200:    2003        .       MOVS     r0,#3
        0x08001202:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001206:    e114        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001208:    e7e5        ..      B        0x80011d6 ; HAL_RCC_OscConfig + 1114
        0x0800120a:    e01e        ..      B        0x800124a ; HAL_RCC_OscConfig + 1230
        0x0800120c:    f7fffb18    ....    BL       HAL_GetTick ; 0x8000840
        0x08001210:    9003        ..      STR      r0,[sp,#0xc]
        0x08001212:    e7ff        ..      B        0x8001214 ; HAL_RCC_OscConfig + 1176
        0x08001214:    f6430070    C.p.    MOV      r0,#0x3870
        0x08001218:    f2c40002    ....    MOVT     r0,#0x4002
        0x0800121c:    6800        .h      LDR      r0,[r0,#0]
        0x0800121e:    f0000102    ....    AND      r1,r0,#2
        0x08001222:    2000        .       MOVS     r0,#0
        0x08001224:    ebb00f51    ..Q.    CMP      r0,r1,LSR #1
        0x08001228:    d00e        ..      BEQ      0x8001248 ; HAL_RCC_OscConfig + 1228
        0x0800122a:    e7ff        ..      B        0x800122c ; HAL_RCC_OscConfig + 1200
        0x0800122c:    f7fffb08    ....    BL       HAL_GetTick ; 0x8000840
        0x08001230:    9903        ..      LDR      r1,[sp,#0xc]
        0x08001232:    1a40        @.      SUBS     r0,r0,r1
        0x08001234:    f2413189    A..1    MOV      r1,#0x1389
        0x08001238:    4288        .B      CMP      r0,r1
        0x0800123a:    d304        ..      BCC      0x8001246 ; HAL_RCC_OscConfig + 1226
        0x0800123c:    e7ff        ..      B        0x800123e ; HAL_RCC_OscConfig + 1218
        0x0800123e:    2003        .       MOVS     r0,#3
        0x08001240:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001244:    e0f5        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001246:    e7e5        ..      B        0x8001214 ; HAL_RCC_OscConfig + 1176
        0x08001248:    e7ff        ..      B        0x800124a ; HAL_RCC_OscConfig + 1230
        0x0800124a:    f89d0007    ....    LDRB     r0,[sp,#7]
        0x0800124e:    2801        .(      CMP      r0,#1
        0x08001250:    d109        ..      BNE      0x8001266 ; HAL_RCC_OscConfig + 1258
        0x08001252:    e7ff        ..      B        0x8001254 ; HAL_RCC_OscConfig + 1240
        0x08001254:    f6430140    C.@.    MOVW     r1,#0x3840
        0x08001258:    f2c40102    ....    MOVT     r1,#0x4002
        0x0800125c:    6808        .h      LDR      r0,[r1,#0]
        0x0800125e:    f0205080     ..P    BIC      r0,r0,#0x10000000
        0x08001262:    6008        .`      STR      r0,[r1,#0]
        0x08001264:    e7ff        ..      B        0x8001266 ; HAL_RCC_OscConfig + 1258
        0x08001266:    e7ff        ..      B        0x8001268 ; HAL_RCC_OscConfig + 1260
        0x08001268:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800126a:    6980        .i      LDR      r0,[r0,#0x18]
        0x0800126c:    2800        .(      CMP      r0,#0
        0x0800126e:    f00080dc    ....    BEQ.W    0x800142a ; HAL_RCC_OscConfig + 1710
        0x08001272:    e7ff        ..      B        0x8001274 ; HAL_RCC_OscConfig + 1272
        0x08001274:    f6430008    C...    MOV      r0,#0x3808
        0x08001278:    f2c40002    ....    MOVT     r0,#0x4002
        0x0800127c:    6800        .h      LDR      r0,[r0,#0]
        0x0800127e:    f000000c    ....    AND      r0,r0,#0xc
        0x08001282:    2808        .(      CMP      r0,#8
        0x08001284:    f0008085    ....    BEQ.W    0x8001392 ; HAL_RCC_OscConfig + 1558
        0x08001288:    e7ff        ..      B        0x800128a ; HAL_RCC_OscConfig + 1294
        0x0800128a:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800128c:    6980        .i      LDR      r0,[r0,#0x18]
        0x0800128e:    2802        .(      CMP      r0,#2
        0x08001290:    d15c        \.      BNE      0x800134c ; HAL_RCC_OscConfig + 1488
        0x08001292:    e7ff        ..      B        0x8001294 ; HAL_RCC_OscConfig + 1304
        0x08001294:    2160        `!      MOVS     r1,#0x60
        0x08001296:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x0800129a:    2000        .       MOVS     r0,#0
        0x0800129c:    6008        .`      STR      r0,[r1,#0]
        0x0800129e:    f7fffacf    ....    BL       HAL_GetTick ; 0x8000840
        0x080012a2:    9003        ..      STR      r0,[sp,#0xc]
        0x080012a4:    e7ff        ..      B        0x80012a6 ; HAL_RCC_OscConfig + 1322
        0x080012a6:    f6430000    C...    MOVW     r0,#0x3800
        0x080012aa:    f2c40002    ....    MOVT     r0,#0x4002
        0x080012ae:    6800        .h      LDR      r0,[r0,#0]
        0x080012b0:    f0007100    ...q    AND      r1,r0,#0x2000000
        0x080012b4:    2000        .       MOVS     r0,#0
        0x080012b6:    ebb06f51    ..Qo    CMP      r0,r1,LSR #25
        0x080012ba:    d00c        ..      BEQ      0x80012d6 ; HAL_RCC_OscConfig + 1370
        0x080012bc:    e7ff        ..      B        0x80012be ; HAL_RCC_OscConfig + 1346
        0x080012be:    f7fffabf    ....    BL       HAL_GetTick ; 0x8000840
        0x080012c2:    9903        ..      LDR      r1,[sp,#0xc]
        0x080012c4:    1a40        @.      SUBS     r0,r0,r1
        0x080012c6:    2803        .(      CMP      r0,#3
        0x080012c8:    d304        ..      BCC      0x80012d4 ; HAL_RCC_OscConfig + 1368
        0x080012ca:    e7ff        ..      B        0x80012cc ; HAL_RCC_OscConfig + 1360
        0x080012cc:    2003        .       MOVS     r0,#3
        0x080012ce:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x080012d2:    e0ae        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x080012d4:    e7e7        ..      B        0x80012a6 ; HAL_RCC_OscConfig + 1322
        0x080012d6:    9904        ..      LDR      r1,[sp,#0x10]
        0x080012d8:    69c8        .i      LDR      r0,[r1,#0x1c]
        0x080012da:    f8d1c020    .. .    LDR      r12,[r1,#0x20]
        0x080012de:    6a4b        Kj      LDR      r3,[r1,#0x24]
        0x080012e0:    6a8a        .j      LDR      r2,[r1,#0x28]
        0x080012e2:    ea40000c    @...    ORR      r0,r0,r12
        0x080012e6:    ea401083    @...    ORR      r0,r0,r3,LSL #6
        0x080012ea:    03d2        ..      LSLS     r2,r2,#15
        0x080012ec:    f64f73ff    O..s    MOV      r3,#0xffff
        0x080012f0:    439a        .C      BICS     r2,r2,r3
        0x080012f2:    f5a23280    ...2    SUB      r2,r2,#0x10000
        0x080012f6:    4310        .C      ORRS     r0,r0,r2
        0x080012f8:    6ac9        .j      LDR      r1,[r1,#0x2c]
        0x080012fa:    ea406001    @..`    ORR      r0,r0,r1,LSL #24
        0x080012fe:    f6430104    C...    MOV      r1,#0x3804
        0x08001302:    f2c40102    ....    MOVT     r1,#0x4002
        0x08001306:    6008        .`      STR      r0,[r1,#0]
        0x08001308:    2160        `!      MOVS     r1,#0x60
        0x0800130a:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x0800130e:    2001        .       MOVS     r0,#1
        0x08001310:    6008        .`      STR      r0,[r1,#0]
        0x08001312:    f7fffa95    ....    BL       HAL_GetTick ; 0x8000840
        0x08001316:    9003        ..      STR      r0,[sp,#0xc]
        0x08001318:    e7ff        ..      B        0x800131a ; HAL_RCC_OscConfig + 1438
        0x0800131a:    f6430000    C...    MOVW     r0,#0x3800
        0x0800131e:    f2c40002    ....    MOVT     r0,#0x4002
        0x08001322:    6800        .h      LDR      r0,[r0,#0]
        0x08001324:    f0007100    ...q    AND      r1,r0,#0x2000000
        0x08001328:    2000        .       MOVS     r0,#0
        0x0800132a:    ebb06f51    ..Qo    CMP      r0,r1,LSR #25
        0x0800132e:    d10c        ..      BNE      0x800134a ; HAL_RCC_OscConfig + 1486
        0x08001330:    e7ff        ..      B        0x8001332 ; HAL_RCC_OscConfig + 1462
        0x08001332:    f7fffa85    ....    BL       HAL_GetTick ; 0x8000840
        0x08001336:    9903        ..      LDR      r1,[sp,#0xc]
        0x08001338:    1a40        @.      SUBS     r0,r0,r1
        0x0800133a:    2803        .(      CMP      r0,#3
        0x0800133c:    d304        ..      BCC      0x8001348 ; HAL_RCC_OscConfig + 1484
        0x0800133e:    e7ff        ..      B        0x8001340 ; HAL_RCC_OscConfig + 1476
        0x08001340:    2003        .       MOVS     r0,#3
        0x08001342:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001346:    e074        t.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001348:    e7e7        ..      B        0x800131a ; HAL_RCC_OscConfig + 1438
        0x0800134a:    e021        !.      B        0x8001390 ; HAL_RCC_OscConfig + 1556
        0x0800134c:    2160        `!      MOVS     r1,#0x60
        0x0800134e:    f2c42147    ..G!    MOVT     r1,#0x4247
        0x08001352:    2000        .       MOVS     r0,#0
        0x08001354:    6008        .`      STR      r0,[r1,#0]
        0x08001356:    f7fffa73    ..s.    BL       HAL_GetTick ; 0x8000840
        0x0800135a:    9003        ..      STR      r0,[sp,#0xc]
        0x0800135c:    e7ff        ..      B        0x800135e ; HAL_RCC_OscConfig + 1506
        0x0800135e:    f6430000    C...    MOVW     r0,#0x3800
        0x08001362:    f2c40002    ....    MOVT     r0,#0x4002
        0x08001366:    6800        .h      LDR      r0,[r0,#0]
        0x08001368:    f0007100    ...q    AND      r1,r0,#0x2000000
        0x0800136c:    2000        .       MOVS     r0,#0
        0x0800136e:    ebb06f51    ..Qo    CMP      r0,r1,LSR #25
        0x08001372:    d00c        ..      BEQ      0x800138e ; HAL_RCC_OscConfig + 1554
        0x08001374:    e7ff        ..      B        0x8001376 ; HAL_RCC_OscConfig + 1530
        0x08001376:    f7fffa63    ..c.    BL       HAL_GetTick ; 0x8000840
        0x0800137a:    9903        ..      LDR      r1,[sp,#0xc]
        0x0800137c:    1a40        @.      SUBS     r0,r0,r1
        0x0800137e:    2803        .(      CMP      r0,#3
        0x08001380:    d304        ..      BCC      0x800138c ; HAL_RCC_OscConfig + 1552
        0x08001382:    e7ff        ..      B        0x8001384 ; HAL_RCC_OscConfig + 1544
        0x08001384:    2003        .       MOVS     r0,#3
        0x08001386:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x0800138a:    e052        R.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x0800138c:    e7e7        ..      B        0x800135e ; HAL_RCC_OscConfig + 1506
        0x0800138e:    e7ff        ..      B        0x8001390 ; HAL_RCC_OscConfig + 1556
        0x08001390:    e04a        J.      B        0x8001428 ; HAL_RCC_OscConfig + 1708
        0x08001392:    9804        ..      LDR      r0,[sp,#0x10]
        0x08001394:    6980        .i      LDR      r0,[r0,#0x18]
        0x08001396:    2801        .(      CMP      r0,#1
        0x08001398:    d104        ..      BNE      0x80013a4 ; HAL_RCC_OscConfig + 1576
        0x0800139a:    e7ff        ..      B        0x800139c ; HAL_RCC_OscConfig + 1568
        0x0800139c:    2001        .       MOVS     r0,#1
        0x0800139e:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x080013a2:    e046        F.      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x080013a4:    f6430004    C...    MOV      r0,#0x3804
        0x080013a8:    f2c40002    ....    MOVT     r0,#0x4002
        0x080013ac:    6800        .h      LDR      r0,[r0,#0]
        0x080013ae:    9002        ..      STR      r0,[sp,#8]
        0x080013b0:    9804        ..      LDR      r0,[sp,#0x10]
        0x080013b2:    6980        .i      LDR      r0,[r0,#0x18]
        0x080013b4:    2801        .(      CMP      r0,#1
        0x080013b6:    d031        1.      BEQ      0x800141c ; HAL_RCC_OscConfig + 1696
        0x080013b8:    e7ff        ..      B        0x80013ba ; HAL_RCC_OscConfig + 1598
        0x080013ba:    9802        ..      LDR      r0,[sp,#8]
        0x080013bc:    f4000080    ....    AND      r0,r0,#0x400000
        0x080013c0:    9904        ..      LDR      r1,[sp,#0x10]
        0x080013c2:    69c9        .i      LDR      r1,[r1,#0x1c]
        0x080013c4:    4288        .B      CMP      r0,r1
        0x080013c6:    d129        ).      BNE      0x800141c ; HAL_RCC_OscConfig + 1696
        0x080013c8:    e7ff        ..      B        0x80013ca ; HAL_RCC_OscConfig + 1614
        0x080013ca:    9802        ..      LDR      r0,[sp,#8]
        0x080013cc:    f000003f    ..?.    AND      r0,r0,#0x3f
        0x080013d0:    9904        ..      LDR      r1,[sp,#0x10]
        0x080013d2:    6a09        .j      LDR      r1,[r1,#0x20]
        0x080013d4:    4288        .B      CMP      r0,r1
        0x080013d6:    d121        !.      BNE      0x800141c ; HAL_RCC_OscConfig + 1696
        0x080013d8:    e7ff        ..      B        0x80013da ; HAL_RCC_OscConfig + 1630
        0x080013da:    9802        ..      LDR      r0,[sp,#8]
        0x080013dc:    f64771c0    G..q    MOV      r1,#0x7fc0
        0x080013e0:    4008        .@      ANDS     r0,r0,r1
        0x080013e2:    9904        ..      LDR      r1,[sp,#0x10]
        0x080013e4:    6a49        Ij      LDR      r1,[r1,#0x24]
        0x080013e6:    ebb01f81    ....    CMP      r0,r1,LSL #6
        0x080013ea:    d117        ..      BNE      0x800141c ; HAL_RCC_OscConfig + 1696
        0x080013ec:    e7ff        ..      B        0x80013ee ; HAL_RCC_OscConfig + 1650
        0x080013ee:    9802        ..      LDR      r0,[sp,#8]
        0x080013f0:    f4003040    ..@0    AND      r0,r0,#0x30000
        0x080013f4:    9904        ..      LDR      r1,[sp,#0x10]
        0x080013f6:    6a89        .j      LDR      r1,[r1,#0x28]
        0x080013f8:    03c9        ..      LSLS     r1,r1,#15
        0x080013fa:    f64f72ff    O..r    MOV      r2,#0xffff
        0x080013fe:    4391        .C      BICS     r1,r1,r2
        0x08001400:    f5a13180    ...1    SUB      r1,r1,#0x10000
        0x08001404:    4288        .B      CMP      r0,r1
        0x08001406:    d109        ..      BNE      0x800141c ; HAL_RCC_OscConfig + 1696
        0x08001408:    e7ff        ..      B        0x800140a ; HAL_RCC_OscConfig + 1678
        0x0800140a:    9802        ..      LDR      r0,[sp,#8]
        0x0800140c:    f0006070    ..p`    AND      r0,r0,#0xf000000
        0x08001410:    9904        ..      LDR      r1,[sp,#0x10]
        0x08001412:    6ac9        .j      LDR      r1,[r1,#0x2c]
        0x08001414:    ebb06f01    ...o    CMP      r0,r1,LSL #24
        0x08001418:    d004        ..      BEQ      0x8001424 ; HAL_RCC_OscConfig + 1704
        0x0800141a:    e7ff        ..      B        0x800141c ; HAL_RCC_OscConfig + 1696
        0x0800141c:    2001        .       MOVS     r0,#1
        0x0800141e:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001422:    e006        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001424:    e7ff        ..      B        0x8001426 ; HAL_RCC_OscConfig + 1706
        0x08001426:    e7ff        ..      B        0x8001428 ; HAL_RCC_OscConfig + 1708
        0x08001428:    e7ff        ..      B        0x800142a ; HAL_RCC_OscConfig + 1710
        0x0800142a:    2000        .       MOVS     r0,#0
        0x0800142c:    f88d0017    ....    STRB     r0,[sp,#0x17]
        0x08001430:    e7ff        ..      B        0x8001432 ; HAL_RCC_OscConfig + 1718
        0x08001432:    f89d0017    ....    LDRB     r0,[sp,#0x17]
        0x08001436:    b006        ..      ADD      sp,sp,#0x18
        0x08001438:    bd80        ..      POP      {r7,pc}
        0x0800143a:    0000        ..      MOVS     r0,r0
    HAL_SYSTICK_Config
        0x0800143c:    b580        ..      PUSH     {r7,lr}
        0x0800143e:    b082        ..      SUB      sp,sp,#8
        0x08001440:    9001        ..      STR      r0,[sp,#4]
        0x08001442:    9801        ..      LDR      r0,[sp,#4]
        0x08001444:    f000f89c    ....    BL       SysTick_Config ; 0x8001580
        0x08001448:    b002        ..      ADD      sp,sp,#8
        0x0800144a:    bd80        ..      POP      {r7,pc}
    HardFault_Handler
        0x0800144c:    e7ff        ..      B        0x800144e ; HardFault_Handler + 2
        0x0800144e:    e7fe        ..      B        0x800144e ; HardFault_Handler + 2
    MX_GPIO_Init
        0x08001450:    b580        ..      PUSH     {r7,lr}
        0x08001452:    b08a        ..      SUB      sp,sp,#0x28
        0x08001454:    2000        .       MOVS     r0,#0
        0x08001456:    9009        ..      STR      r0,[sp,#0x24]
        0x08001458:    9008        ..      STR      r0,[sp,#0x20]
        0x0800145a:    9007        ..      STR      r0,[sp,#0x1c]
        0x0800145c:    9006        ..      STR      r0,[sp,#0x18]
        0x0800145e:    9005        ..      STR      r0,[sp,#0x14]
        0x08001460:    e7ff        ..      B        0x8001462 ; MX_GPIO_Init + 18
        0x08001462:    2000        .       MOVS     r0,#0
        0x08001464:    9004        ..      STR      r0,[sp,#0x10]
        0x08001466:    f6430030    C.0.    MOV      r0,#0x3830
        0x0800146a:    f2c40002    ....    MOVT     r0,#0x4002
        0x0800146e:    6801        .h      LDR      r1,[r0,#0]
        0x08001470:    f0410180    A...    ORR      r1,r1,#0x80
        0x08001474:    6001        .`      STR      r1,[r0,#0]
        0x08001476:    6800        .h      LDR      r0,[r0,#0]
        0x08001478:    f0000080    ....    AND      r0,r0,#0x80
        0x0800147c:    9004        ..      STR      r0,[sp,#0x10]
        0x0800147e:    9804        ..      LDR      r0,[sp,#0x10]
        0x08001480:    e7ff        ..      B        0x8001482 ; MX_GPIO_Init + 50
        0x08001482:    e7ff        ..      B        0x8001484 ; MX_GPIO_Init + 52
        0x08001484:    2000        .       MOVS     r0,#0
        0x08001486:    9003        ..      STR      r0,[sp,#0xc]
        0x08001488:    f6430030    C.0.    MOV      r0,#0x3830
        0x0800148c:    f2c40002    ....    MOVT     r0,#0x4002
        0x08001490:    6801        .h      LDR      r1,[r0,#0]
        0x08001492:    f0410101    A...    ORR      r1,r1,#1
        0x08001496:    6001        .`      STR      r1,[r0,#0]
        0x08001498:    6800        .h      LDR      r0,[r0,#0]
        0x0800149a:    f0000001    ....    AND      r0,r0,#1
        0x0800149e:    9003        ..      STR      r0,[sp,#0xc]
        0x080014a0:    9803        ..      LDR      r0,[sp,#0xc]
        0x080014a2:    e7ff        ..      B        0x80014a4 ; MX_GPIO_Init + 84
        0x080014a4:    2000        .       MOVS     r0,#0
        0x080014a6:    f2c40002    ....    MOVT     r0,#0x4002
        0x080014aa:    9002        ..      STR      r0,[sp,#8]
        0x080014ac:    2120         !      MOVS     r1,#0x20
        0x080014ae:    9100        ..      STR      r1,[sp,#0]
        0x080014b0:    2201        ."      MOVS     r2,#1
        0x080014b2:    9201        ..      STR      r2,[sp,#4]
        0x080014b4:    f7fff9ac    ....    BL       HAL_GPIO_WritePin ; 0x8000810
        0x080014b8:    9a00        ..      LDR      r2,[sp,#0]
        0x080014ba:    9901        ..      LDR      r1,[sp,#4]
        0x080014bc:    9802        ..      LDR      r0,[sp,#8]
        0x080014be:    9205        ..      STR      r2,[sp,#0x14]
        0x080014c0:    9106        ..      STR      r1,[sp,#0x18]
        0x080014c2:    2100        .!      MOVS     r1,#0
        0x080014c4:    9107        ..      STR      r1,[sp,#0x1c]
        0x080014c6:    9108        ..      STR      r1,[sp,#0x20]
        0x080014c8:    a905        ..      ADD      r1,sp,#0x14
        0x080014ca:    f7feffef    ....    BL       HAL_GPIO_Init ; 0x80004ac
        0x080014ce:    b00a        ..      ADD      sp,sp,#0x28
        0x080014d0:    bd80        ..      POP      {r7,pc}
        0x080014d2:    0000        ..      MOVS     r0,r0
    MX_IWDG_Init
        0x080014d4:    b580        ..      PUSH     {r7,lr}
        0x080014d6:    f2400070    @.p.    MOVW     r0,#0x70
        0x080014da:    f2c20000    ....    MOVT     r0,#0x2000
        0x080014de:    f2430100    C...    MOVW     r1,#0x3000
        0x080014e2:    f2c40100    ....    MOVT     r1,#0x4000
        0x080014e6:    6001        .`      STR      r1,[r0,#0]
        0x080014e8:    2100        .!      MOVS     r1,#0
        0x080014ea:    6041        A`      STR      r1,[r0,#4]
        0x080014ec:    f64071ff    @..q    MOV      r1,#0xfff
        0x080014f0:    6081        .`      STR      r1,[r0,#8]
        0x080014f2:    f7fff9ab    ....    BL       HAL_IWDG_Init ; 0x800084c
        0x080014f6:    b118        ..      CBZ      r0,0x8001500 ; MX_IWDG_Init + 44
        0x080014f8:    e7ff        ..      B        0x80014fa ; MX_IWDG_Init + 38
        0x080014fa:    f7feffb1    ....    BL       Error_Handler ; 0x8000460
        0x080014fe:    e7ff        ..      B        0x8001500 ; MX_IWDG_Init + 44
        0x08001500:    bd80        ..      POP      {r7,pc}
        0x08001502:    0000        ..      MOVS     r0,r0
    MemManage_Handler
        0x08001504:    e7ff        ..      B        0x8001506 ; MemManage_Handler + 2
        0x08001506:    e7fe        ..      B        0x8001506 ; MemManage_Handler + 2
    NMI_Handler
        0x08001508:    e7ff        ..      B        0x800150a ; NMI_Handler + 2
        0x0800150a:    e7fe        ..      B        0x800150a ; NMI_Handler + 2
    NVIC_EncodePriority
        0x0800150c:    b088        ..      SUB      sp,sp,#0x20
        0x0800150e:    9007        ..      STR      r0,[sp,#0x1c]
        0x08001510:    9106        ..      STR      r1,[sp,#0x18]
        0x08001512:    9205        ..      STR      r2,[sp,#0x14]
        0x08001514:    9807        ..      LDR      r0,[sp,#0x1c]
        0x08001516:    f0000007    ....    AND      r0,r0,#7
        0x0800151a:    9004        ..      STR      r0,[sp,#0x10]
        0x0800151c:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800151e:    f1c00007    ....    RSB      r0,r0,#7
        0x08001522:    2805        .(      CMP      r0,#5
        0x08001524:    d303        ..      BCC      0x800152e ; NVIC_EncodePriority + 34
        0x08001526:    e7ff        ..      B        0x8001528 ; NVIC_EncodePriority + 28
        0x08001528:    2004        .       MOVS     r0,#4
        0x0800152a:    9001        ..      STR      r0,[sp,#4]
        0x0800152c:    e004        ..      B        0x8001538 ; NVIC_EncodePriority + 44
        0x0800152e:    9804        ..      LDR      r0,[sp,#0x10]
        0x08001530:    f1c00007    ....    RSB      r0,r0,#7
        0x08001534:    9001        ..      STR      r0,[sp,#4]
        0x08001536:    e7ff        ..      B        0x8001538 ; NVIC_EncodePriority + 44
        0x08001538:    9801        ..      LDR      r0,[sp,#4]
        0x0800153a:    9003        ..      STR      r0,[sp,#0xc]
        0x0800153c:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800153e:    3004        .0      ADDS     r0,#4
        0x08001540:    2806        .(      CMP      r0,#6
        0x08001542:    d803        ..      BHI      0x800154c ; NVIC_EncodePriority + 64
        0x08001544:    e7ff        ..      B        0x8001546 ; NVIC_EncodePriority + 58
        0x08001546:    2000        .       MOVS     r0,#0
        0x08001548:    9000        ..      STR      r0,[sp,#0]
        0x0800154a:    e003        ..      B        0x8001554 ; NVIC_EncodePriority + 72
        0x0800154c:    9804        ..      LDR      r0,[sp,#0x10]
        0x0800154e:    3803        .8      SUBS     r0,#3
        0x08001550:    9000        ..      STR      r0,[sp,#0]
        0x08001552:    e7ff        ..      B        0x8001554 ; NVIC_EncodePriority + 72
        0x08001554:    9800        ..      LDR      r0,[sp,#0]
        0x08001556:    9002        ..      STR      r0,[sp,#8]
        0x08001558:    9806        ..      LDR      r0,[sp,#0x18]
        0x0800155a:    9903        ..      LDR      r1,[sp,#0xc]
        0x0800155c:    2201        ."      MOVS     r2,#1
        0x0800155e:    fa02f101    ....    LSL      r1,r2,r1
        0x08001562:    3901        .9      SUBS     r1,#1
        0x08001564:    4008        .@      ANDS     r0,r0,r1
        0x08001566:    9b02        ..      LDR      r3,[sp,#8]
        0x08001568:    4098        .@      LSLS     r0,r0,r3
        0x0800156a:    9905        ..      LDR      r1,[sp,#0x14]
        0x0800156c:    409a        .@      LSLS     r2,r2,r3
        0x0800156e:    3a01        .:      SUBS     r2,#1
        0x08001570:    4011        .@      ANDS     r1,r1,r2
        0x08001572:    4308        .C      ORRS     r0,r0,r1
        0x08001574:    b008        ..      ADD      sp,sp,#0x20
        0x08001576:    4770        pG      BX       lr
    PendSV_Handler
        0x08001578:    4770        pG      BX       lr
        0x0800157a:    0000        ..      MOVS     r0,r0
    SVC_Handler
        0x0800157c:    4770        pG      BX       lr
        0x0800157e:    0000        ..      MOVS     r0,r0
    SysTick_Config
        0x08001580:    b580        ..      PUSH     {r7,lr}
        0x08001582:    b082        ..      SUB      sp,sp,#8
        0x08001584:    9000        ..      STR      r0,[sp,#0]
        0x08001586:    9800        ..      LDR      r0,[sp,#0]
        0x08001588:    3801        .8      SUBS     r0,#1
        0x0800158a:    f1b07f80    ....    CMP      r0,#0x1000000
        0x0800158e:    d303        ..      BCC      0x8001598 ; SysTick_Config + 24
        0x08001590:    e7ff        ..      B        0x8001592 ; SysTick_Config + 18
        0x08001592:    2001        .       MOVS     r0,#1
        0x08001594:    9001        ..      STR      r0,[sp,#4]
        0x08001596:    e019        ..      B        0x80015cc ; SysTick_Config + 76
        0x08001598:    9800        ..      LDR      r0,[sp,#0]
        0x0800159a:    3801        .8      SUBS     r0,#1
        0x0800159c:    f24e0114    N...    MOV      r1,#0xe014
        0x080015a0:    f2ce0100    ....    MOVT     r1,#0xe000
        0x080015a4:    6008        .`      STR      r0,[r1,#0]
        0x080015a6:    f04f30ff    O..0    MOV      r0,#0xffffffff
        0x080015aa:    210f        .!      MOVS     r1,#0xf
        0x080015ac:    f000f888    ....    BL       __NVIC_SetPriority ; 0x80016c0
        0x080015b0:    f24e0118    N...    MOV      r1,#0xe018
        0x080015b4:    f2ce0100    ....    MOVT     r1,#0xe000
        0x080015b8:    2000        .       MOVS     r0,#0
        0x080015ba:    6008        .`      STR      r0,[r1,#0]
        0x080015bc:    f24e0210    N...    MOV      r2,#0xe010
        0x080015c0:    f2ce0200    ....    MOVT     r2,#0xe000
        0x080015c4:    2107        .!      MOVS     r1,#7
        0x080015c6:    6011        .`      STR      r1,[r2,#0]
        0x080015c8:    9001        ..      STR      r0,[sp,#4]
        0x080015ca:    e7ff        ..      B        0x80015cc ; SysTick_Config + 76
        0x080015cc:    9801        ..      LDR      r0,[sp,#4]
        0x080015ce:    b002        ..      ADD      sp,sp,#8
        0x080015d0:    bd80        ..      POP      {r7,pc}
        0x080015d2:    0000        ..      MOVS     r0,r0
    SysTick_Handler
        0x080015d4:    b580        ..      PUSH     {r7,lr}
        0x080015d6:    f7fff981    ....    BL       HAL_IncTick ; 0x80008dc
        0x080015da:    bd80        ..      POP      {r7,pc}
    SystemClock_Config
        0x080015dc:    b580        ..      PUSH     {r7,lr}
        0x080015de:    b094        ..      SUB      sp,sp,#0x50
        0x080015e0:    a808        ..      ADD      r0,sp,#0x20
        0x080015e2:    2130        0!      MOVS     r1,#0x30
        0x080015e4:    f7fefed4    ....    BL       __aeabi_memclr4 ; 0x8000390
        0x080015e8:    2000        .       MOVS     r0,#0
        0x080015ea:    9007        ..      STR      r0,[sp,#0x1c]
        0x080015ec:    9006        ..      STR      r0,[sp,#0x18]
        0x080015ee:    9005        ..      STR      r0,[sp,#0x14]
        0x080015f0:    9004        ..      STR      r0,[sp,#0x10]
        0x080015f2:    9003        ..      STR      r0,[sp,#0xc]
        0x080015f4:    e7ff        ..      B        0x80015f6 ; SystemClock_Config + 26
        0x080015f6:    2000        .       MOVS     r0,#0
        0x080015f8:    9002        ..      STR      r0,[sp,#8]
        0x080015fa:    f6430040    C.@.    MOVW     r0,#0x3840
        0x080015fe:    f2c40002    ....    MOVT     r0,#0x4002
        0x08001602:    6801        .h      LDR      r1,[r0,#0]
        0x08001604:    f0415180    A..Q    ORR      r1,r1,#0x10000000
        0x08001608:    6001        .`      STR      r1,[r0,#0]
        0x0800160a:    6800        .h      LDR      r0,[r0,#0]
        0x0800160c:    f0005080    ...P    AND      r0,r0,#0x10000000
        0x08001610:    9002        ..      STR      r0,[sp,#8]
        0x08001612:    9802        ..      LDR      r0,[sp,#8]
        0x08001614:    e7ff        ..      B        0x8001616 ; SystemClock_Config + 58
        0x08001616:    e7ff        ..      B        0x8001618 ; SystemClock_Config + 60
        0x08001618:    2000        .       MOVS     r0,#0
        0x0800161a:    9001        ..      STR      r0,[sp,#4]
        0x0800161c:    f2470000    G...    MOVW     r0,#0x7000
        0x08001620:    f2c40000    ....    MOVT     r0,#0x4000
        0x08001624:    6801        .h      LDR      r1,[r0,#0]
        0x08001626:    2202        ."      MOVS     r2,#2
        0x08001628:    f362318f    b..1    BFI      r1,r2,#14,#2
        0x0800162c:    6001        .`      STR      r1,[r0,#0]
        0x0800162e:    6800        .h      LDR      r0,[r0,#0]
        0x08001630:    f4004040    ..@@    AND      r0,r0,#0xc000
        0x08001634:    9001        ..      STR      r0,[sp,#4]
        0x08001636:    9801        ..      LDR      r0,[sp,#4]
        0x08001638:    e7ff        ..      B        0x800163a ; SystemClock_Config + 94
        0x0800163a:    2009        .       MOVS     r0,#9
        0x0800163c:    9008        ..      STR      r0,[sp,#0x20]
        0x0800163e:    f44f3080    O..0    MOV      r0,#0x10000
        0x08001642:    9009        ..      STR      r0,[sp,#0x24]
        0x08001644:    2001        .       MOVS     r0,#1
        0x08001646:    900d        ..      STR      r0,[sp,#0x34]
        0x08001648:    2002        .       MOVS     r0,#2
        0x0800164a:    900e        ..      STR      r0,[sp,#0x38]
        0x0800164c:    f44f0180    O...    MOV      r1,#0x400000
        0x08001650:    910f        ..      STR      r1,[sp,#0x3c]
        0x08001652:    2119        .!      MOVS     r1,#0x19
        0x08001654:    9110        ..      STR      r1,[sp,#0x40]
        0x08001656:    21a8        .!      MOVS     r1,#0xa8
        0x08001658:    9111        ..      STR      r1,[sp,#0x44]
        0x0800165a:    9012        ..      STR      r0,[sp,#0x48]
        0x0800165c:    2004        .       MOVS     r0,#4
        0x0800165e:    9013        ..      STR      r0,[sp,#0x4c]
        0x08001660:    a808        ..      ADD      r0,sp,#0x20
        0x08001662:    f7fffb8b    ....    BL       HAL_RCC_OscConfig ; 0x8000d7c
        0x08001666:    b118        ..      CBZ      r0,0x8001670 ; SystemClock_Config + 148
        0x08001668:    e7ff        ..      B        0x800166a ; SystemClock_Config + 142
        0x0800166a:    f7fefef9    ....    BL       Error_Handler ; 0x8000460
        0x0800166e:    e7ff        ..      B        0x8001670 ; SystemClock_Config + 148
        0x08001670:    200f        .       MOVS     r0,#0xf
        0x08001672:    9003        ..      STR      r0,[sp,#0xc]
        0x08001674:    2102        .!      MOVS     r1,#2
        0x08001676:    9104        ..      STR      r1,[sp,#0x10]
        0x08001678:    2000        .       MOVS     r0,#0
        0x0800167a:    9005        ..      STR      r0,[sp,#0x14]
        0x0800167c:    f44f5280    O..R    MOV      r2,#0x1000
        0x08001680:    9206        ..      STR      r2,[sp,#0x18]
        0x08001682:    9007        ..      STR      r0,[sp,#0x1c]
        0x08001684:    a803        ..      ADD      r0,sp,#0xc
        0x08001686:    f7fff9d3    ....    BL       HAL_RCC_ClockConfig ; 0x8000a30
        0x0800168a:    b118        ..      CBZ      r0,0x8001694 ; SystemClock_Config + 184
        0x0800168c:    e7ff        ..      B        0x800168e ; SystemClock_Config + 178
        0x0800168e:    f7fefee7    ....    BL       Error_Handler ; 0x8000460
        0x08001692:    e7ff        ..      B        0x8001694 ; SystemClock_Config + 184
        0x08001694:    b014        ..      ADD      sp,sp,#0x50
        0x08001696:    bd80        ..      POP      {r7,pc}
    SystemInit
        0x08001698:    f64e5188    N..Q    MOV      r1,#0xed88
        0x0800169c:    f2ce0100    ....    MOVT     r1,#0xe000
        0x080016a0:    6808        .h      LDR      r0,[r1,#0]
        0x080016a2:    f4400070    @.p.    ORR      r0,r0,#0xf00000
        0x080016a6:    6008        .`      STR      r0,[r1,#0]
        0x080016a8:    4770        pG      BX       lr
        0x080016aa:    0000        ..      MOVS     r0,r0
    UsageFault_Handler
        0x080016ac:    e7ff        ..      B        0x80016ae ; UsageFault_Handler + 2
        0x080016ae:    e7fe        ..      B        0x80016ae ; UsageFault_Handler + 2
    __NVIC_GetPriorityGrouping
        0x080016b0:    f64e500c    N..P    MOV      r0,#0xed0c
        0x080016b4:    f2ce0000    ....    MOVT     r0,#0xe000
        0x080016b8:    6800        .h      LDR      r0,[r0,#0]
        0x080016ba:    f3c02002    ...     UBFX     r0,r0,#8,#3
        0x080016be:    4770        pG      BX       lr
    __NVIC_SetPriority
        0x080016c0:    b082        ..      SUB      sp,sp,#8
        0x080016c2:    f88d0007    ....    STRB     r0,[sp,#7]
        0x080016c6:    9100        ..      STR      r1,[sp,#0]
        0x080016c8:    f99d0007    ....    LDRSB    r0,[sp,#7]
        0x080016cc:    2800        .(      CMP      r0,#0
        0x080016ce:    d40a        ..      BMI      0x80016e6 ; __NVIC_SetPriority + 38
        0x080016d0:    e7ff        ..      B        0x80016d2 ; __NVIC_SetPriority + 18
        0x080016d2:    9800        ..      LDR      r0,[sp,#0]
        0x080016d4:    0100        ..      LSLS     r0,r0,#4
        0x080016d6:    f99d1007    ....    LDRSB    r1,[sp,#7]
        0x080016da:    f24e4200    N..B    MOVW     r2,#0xe400
        0x080016de:    f2ce0200    ....    MOVT     r2,#0xe000
        0x080016e2:    5488        .T      STRB     r0,[r1,r2]
        0x080016e4:    e00b        ..      B        0x80016fe ; __NVIC_SetPriority + 62
        0x080016e6:    9800        ..      LDR      r0,[sp,#0]
        0x080016e8:    0100        ..      LSLS     r0,r0,#4
        0x080016ea:    f89d1007    ....    LDRB     r1,[sp,#7]
        0x080016ee:    f001010f    ....    AND      r1,r1,#0xf
        0x080016f2:    f64e5214    N..R    MOV      r2,#0xed14
        0x080016f6:    f2ce0200    ....    MOVT     r2,#0xe000
        0x080016fa:    5488        .T      STRB     r0,[r1,r2]
        0x080016fc:    e7ff        ..      B        0x80016fe ; __NVIC_SetPriority + 62
        0x080016fe:    b002        ..      ADD      sp,sp,#8
        0x08001700:    4770        pG      BX       lr
        0x08001702:    0000        ..      MOVS     r0,r0
    __NVIC_SetPriorityGrouping
        0x08001704:    b083        ..      SUB      sp,sp,#0xc
        0x08001706:    9002        ..      STR      r0,[sp,#8]
        0x08001708:    9802        ..      LDR      r0,[sp,#8]
        0x0800170a:    f0000007    ....    AND      r0,r0,#7
        0x0800170e:    9000        ..      STR      r0,[sp,#0]
        0x08001710:    f64e510c    N..Q    MOV      r1,#0xed0c
        0x08001714:    f2ce0100    ....    MOVT     r1,#0xe000
        0x08001718:    6808        .h      LDR      r0,[r1,#0]
        0x0800171a:    9001        ..      STR      r0,[sp,#4]
        0x0800171c:    9801        ..      LDR      r0,[sp,#4]
        0x0800171e:    f64f02ff    O...    MOV      r2,#0xf8ff
        0x08001722:    4010        .@      ANDS     r0,r0,r2
        0x08001724:    9001        ..      STR      r0,[sp,#4]
        0x08001726:    9801        ..      LDR      r0,[sp,#4]
        0x08001728:    9a00        ..      LDR      r2,[sp,#0]
        0x0800172a:    ea402002    @..     ORR      r0,r0,r2,LSL #8
        0x0800172e:    2200        ."      MOVS     r2,#0
        0x08001730:    f2c052fa    ...R    MOVT     r2,#0x5fa
        0x08001734:    4310        .C      ORRS     r0,r0,r2
        0x08001736:    9001        ..      STR      r0,[sp,#4]
        0x08001738:    9801        ..      LDR      r0,[sp,#4]
        0x0800173a:    6008        .`      STR      r0,[r1,#0]
        0x0800173c:    b003        ..      ADD      sp,sp,#0xc
        0x0800173e:    4770        pG      BX       lr
    main
        0x08001740:    b580        ..      PUSH     {r7,lr}
        0x08001742:    b084        ..      SUB      sp,sp,#0x10
        0x08001744:    2000        .       MOVS     r0,#0
        0x08001746:    9003        ..      STR      r0,[sp,#0xc]
        0x08001748:    f7fff8d6    ....    BL       HAL_Init ; 0x80008f8
        0x0800174c:    f7ffff46    ..F.    BL       SystemClock_Config ; 0x80015dc
        0x08001750:    f7fffe7e    ..~.    BL       MX_GPIO_Init ; 0x8001450
        0x08001754:    f7fffebe    ....    BL       MX_IWDG_Init ; 0x80014d4
        0x08001758:    e7ff        ..      B        0x800175a ; main + 26
        0x0800175a:    2000        .       MOVS     r0,#0
        0x0800175c:    f2c40002    ....    MOVT     r0,#0x4002
        0x08001760:    9000        ..      STR      r0,[sp,#0]
        0x08001762:    2120         !      MOVS     r1,#0x20
        0x08001764:    9101        ..      STR      r1,[sp,#4]
        0x08001766:    2201        ."      MOVS     r2,#1
        0x08001768:    f7fff852    ..R.    BL       HAL_GPIO_WritePin ; 0x8000810
        0x0800176c:    2064        d       MOVS     r0,#0x64
        0x0800176e:    9002        ..      STR      r0,[sp,#8]
        0x08001770:    f7fefe7a    ..z.    BL       HAL_Delay ; 0x8000468
        0x08001774:    9800        ..      LDR      r0,[sp,#0]
        0x08001776:    9901        ..      LDR      r1,[sp,#4]
        0x08001778:    2200        ."      MOVS     r2,#0
        0x0800177a:    f7fff849    ..I.    BL       HAL_GPIO_WritePin ; 0x8000810
        0x0800177e:    9802        ..      LDR      r0,[sp,#8]
        0x08001780:    f7fefe72    ..r.    BL       HAL_Delay ; 0x8000468
        0x08001784:    e7e9        ..      B        0x800175a ; main + 26
    x$fpl$fpinit
    $v0
    _fp_init
        0x08001786:    eef10a10    ....    VMRS     r0,FPSCR
        0x0800178a:    f64f71ff    O..q    MOV      r1,#0xffff
        0x0800178e:    f2c031c0    ...1    MOVT     r1,#0x3c0
        0x08001792:    ea200001     ...    BIC      r0,r0,r1
        0x08001796:    f0407040    @.@p    ORR      r0,r0,#0x3000000
        0x0800179a:    eee10a10    ....    VMSR     FPSCR,r0
    __fplib_config_fpu_vfp
    __fplib_config_pureend_doubles
        0x0800179e:    4770        pG      BX       lr
    $d.realdata
    AHBPrescTable
        0x080017a0:    00000000    ....    DCD    0
        0x080017a4:    00000000    ....    DCD    0
        0x080017a8:    04030201    ....    DCD    67305985
        0x080017ac:    09080706    ....    DCD    151521030
    Region$$Table$$Base
        0x080017b0:    080017d0    ....    DCD    134223824
        0x080017b4:    20000000    ...     DCD    536870912
        0x080017b8:    00000010    ....    DCD    16
        0x080017bc:    080001f8    ....    DCD    134218232
        0x080017c0:    080017e0    ....    DCD    134223840
        0x080017c4:    20000010    ...     DCD    536870928
        0x080017c8:    00000670    p...    DCD    1648
        0x080017cc:    08000218    ....    DCD    134218264
    Region$$Table$$Limit

** Section #2 'RW_IRAM1' (SHT_PROGBITS) [SHF_ALLOC + SHF_WRITE]
    Size   : 16 bytes (alignment 8)
    Address: 0x20000000


** Section #3 'RW_IRAM1' (SHT_NOBITS) [SHF_ALLOC + SHF_WRITE]
    Size   : 1648 bytes (alignment 8)
    Address: 0x20000010


** Section #4 '.debug_abbrev' (SHT_PROGBITS)
    Size   : 2920 bytes


** Section #5 '.debug_frame' (SHT_PROGBITS)
    Size   : 3104 bytes


** Section #6 '.debug_info' (SHT_PROGBITS)
    Size   : 13740 bytes


** Section #7 '.debug_line' (SHT_PROGBITS)
    Size   : 10887 bytes


** Section #8 '.debug_ranges' (SHT_PROGBITS)
    Size   : 864 bytes


** Section #9 '.debug_str' (SHT_PROGBITS)
    Size   : 11274 bytes


** Section #10 '.symtab' (SHT_SYMTAB)
    Size   : 5408 bytes (alignment 4)
    String table #11 '.strtab'
    Last local symbol no. 164


** Section #11 '.strtab' (SHT_STRTAB)
    Size   : 5872 bytes


** Section #12 '.note' (SHT_NOTE)
    Size   : 28 bytes (alignment 4)


** Section #13 '.comment' (SHT_PROGBITS)
    Size   : 1520 bytes


** Section #14 '.shstrtab' (SHT_STRTAB)
    Size   : 136 bytes


