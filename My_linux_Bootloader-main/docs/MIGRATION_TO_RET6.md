# STM32F401RET6 配置总结

## 更新的文件

### 1. 链接脚本

**bootloader/ld/STM32F401RETx\_BOOTLOADER.ld:**

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 96K   /* 更新: 64K → 96K */
  FLASH (rx)   : ORIGIN = 0x08000000,   LENGTH = 64K   /* Bootloader区域不变 */
}
```

**application/ld/STM32F401RETx\_APPLICATION.ld:**

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 96K   /* 更新: 64K → 96K */
  FLASH (rx)   : ORIGIN = 0x08010000,   LENGTH = 448K  /* 更新: 192K → 448K */
}
```

### 2. 头文件

**bootloader/inc/main.h:**

```c
#define APP_START_ADDR           0x08010000U  /* 不变 */
#define APP_END_ADDR             0x0807FFFFU  /* 更新: 0x0803FFFF → 0x0807FFFF */

#define SRAM_BASE                0x20000000U  /* 不变 */
#define SRAM_END                 0x20017FFFU  /* 更新: 0x2000FFFF → 0x20017FFF */
```

<br />

## 内存布局

### Flash (512 KB 总计)

```
┌─────────────────────────────────────────┐
│  Bootloader (64 KB)                     │ 0x08000000
│  ├─ Vector Table                        │
│  ├─ Bootloader Code                     │
│  └─ Bootloader Data                     │
├─────────────────────────────────────────┤
│  Application (448 KB)                   │ 0x08010000
│  ├─ Vector Table                        │
│  ├─ Application Code                    │
│  └─ Application Data                    │
└─────────────────────────────────────────┘ 0x08080000
```

### SRAM (96 KB 总计)

