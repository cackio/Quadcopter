# STM32 Bootloader Project - Agent Coding Guidelines

This document provides guidelines for AI agents working on the STM32F401 Bootloader project in Linux (WSL) environment.

## Project Overview

**Target Hardware:** STM32F401RET6 (Cortex-M4, 84MHz, 512KB Flash, 96KB SRAM)
**Development Environment:** Linux/WSL
**Toolchain:** arm-none-eabi-gcc
**Debugger/Flasher:** ST-Link + GDB
**Architecture:** Two-stage bootloader (Bootloader + Application)
**Operating System:** Bare-metal (no RTOS)

## Build Commands

### Building Components

```bash
# Build all components (bootloader + application)
./build.sh all

# Build only bootloader
./build.sh bootloader

# Build only application
./build.sh application

# Clean all build artifacts
./build.sh clean

# Clean and rebuild all
./build.sh clean && ./build.sh all
```

### Flashing to Device

```bash
# Flash bootloader to STM32 (via ST-Link + GDB)
./tools/flash_bootloader.sh

# Flash application to STM32
./tools/flash_application.sh

# Flash both (bootloader first, then application)
./tools/flash_all.sh

# Debug bootloader with GDB
./tools/debug_bootloader.sh

# Debug application with GDB
./tools/debug_application.sh
```

### Verification

```bash
# Verify flash contents
arm-none-eabi-objdump -h build/bootloader.elf
arm-none-eabi-objdump -h build/application.elf

# Check binary sizes
ls -lh build/*.bin

# View memory map
arm-none-eabi-nm -n build/bootloader.elf | less
```

## Toolchain Requirements

**Required packages:**
```bash
# Install toolchain (Ubuntu/Debian)
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi openocd

# Verify installation
arm-none-eabi-gcc --version
openocd --version
```

**Minimum versions:**
- gcc-arm-none-eabi: 10.3-2021.10 or later
- newlib: 4.1.0 or later
- openocd: 0.11.0 or later

## Memory Layout

**STM32F401RET6 Memory Map:**
```
Flash Memory (512KB total):
  0x08000000 - 0x0800FFFF  Bootloader Region (64KB)
  0x08010000 - 0x0807FFFF  Application Region (448KB)

SRAM (96KB total):
  0x20000000 - 0x20017FFF  Main SRAM
```

**Vector Table Placement:**
- Bootloader: `SCB->VTOR = 0x08000000`
- Application: `SCB->VTOR = 0x08010000`

## Code Style Guidelines

### Naming Conventions

**Functions:** `snake_case`
```c
// Good
void system_init_clock(void);
void gpio_set_output(GPIO_TypeDef *port, uint8_t pin);

// Bad
void SystemInitClock(void);
void GPIO_SetOutput(GPIO_TypeDef *port, uint8_t pin);
```

**Variables:** `snake_case`
```c
// Good
uint32_t app_start_addr;
uint8_t boot_timeout_ms;

// Bad
uint32_t AppStartAddr;
uint8_t bootTimeoutMs;
```

**Constants/Macros:** `UPPER_SNAKE_CASE`
```c
// Good
#define FLASH_BASE_ADDR       0x08000000
#define BOOTLOADER_SIZE_KB    64
#define APP_START_OFFSET      0x10000

// Bad
#define flashBaseAddr         0x08000000
#define BootloaderSizeKB      64
```

**Types:** `PascalCase` with `_t` suffix for structs
```c
// Good
typedef struct {
    uint32_t stack_ptr;
    uint32_t reset_handler;
} VectorTable_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} Version_t;

// Bad
typedef struct {
    uint32_t stack_ptr;
    uint32_t reset_handler;
} vectorTable_t;
```

### Include Order

```c
// 1. C standard library headers
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// 2. CMSIS headers
#include "stm32f4xx.h"

// 3. Project headers (grouped by module)
#include "system_stm32f4xx.h"
#include "gpio.h"
#include "uart.h"
```

### File Organization

Each .c file should have a corresponding .h header file with the same basename.

```c
// bootloader/src/main.c
#include "main.h"
#include "system_init.h"
#include "bootloader_jump.h"

void main(void) {
    system_init();
    bootloader_jump_to_app();
}
```

### Error Handling

**Return codes:**
```c
#define SUCCESS         0
#define ERR_INVALID_ADDR    -1
#define ERR_NO_APP         -2
#define ERR_FLASH_FAIL     -3

int32_t bootloader_jump_to_app(void);
```

**Error checking:**
```c
// Good
int32_t ret = validate_application();
if (ret != SUCCESS) {
    log_error("Application validation failed: %d", ret);
    stay_in_bootloader();
}

// Bad
validate_application();  // Never ignore return codes
```

### Interrupt Handlers

All interrupt handlers must have the `IRQHandler` suffix and be marked with `__attribute__((interrupt))`.

```c
void __attribute__((interrupt)) SysTick_Handler(void) {
    system_tick_increment();
}
```

### Inline Assembly

Keep inline assembly minimal and well-commented.

```c
__attribute__((always_inline)) static inline void __disable_irq(void) {
    __asm volatile("cpsid i" ::: "memory");
}
```

### Memory-Mapped I/O

Always use CMSIS-provided peripheral pointers. Never hardcode addresses except in linker scripts.

```c
// Good
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
GPIOA->MODER |= (1 << 10);  // PA5 as output

// Bad
*(volatile uint32_t *)(0x40023830) |= (1 << 0);  // Unclear what this does
```

## Critical Constraints

### Bootloader-Specific Rules

1. **NEVER write to bootloader flash region (0x08000000-0x0800FFFF) from application code**
2. **ALWAYS disable interrupts before remapping vector table:**
   ```c
   __disable_irq();
   SCB->VTOR = APP_START_ADDR;
   __enable_irq();
   ```

3. **ALWAYS validate application before jumping:**
   - Check if stack pointer points to SRAM range (0x20000000-0x2000FFFF)
   - Check if reset handler points to flash range (0x08010000-0x0803FFFF)
   - Use checksum verification for production builds

4. **NEVER use dynamic memory allocation (malloc/free)**
5. **NEVER use blocking delays in critical boot sequences**

### Application-Specific Rules

1. **Application must be linked with VTOR = 0x08010000**
2. **Application must not modify vectors 0-3 (initial SP, Reset, NMI, HardFault)**
3. **Application stack size should be at least 1KB**

### Vector Table Rules

```c
// Correct placement in linker script
.isr_vector :
{
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
} > FLASH
```

```c
// Correct placement in source code
__attribute__((section(".isr_vector")))
const VectorTable_t g_vector_table = {
    .stack_ptr = (uint32_t)&_estack,
    .reset_handler = (uint32_t)&Reset_Handler,
    // ... other vectors
};
```

## Testing and Validation

### Self-Test Requirements

Every code change must pass these checks:

1. **Compilation:** `./build.sh clean && ./build.sh all`
2. **Size check:** Bootloader < 64KB, Application < 192KB
3. **Vector table validation:** Use `arm-none-eabi-objdump -s -j .isr_vector`
4. **Link map validation:** Check `_estack` placement

### Flash Verification

```bash
# After flashing, verify with GDB
arm-none-eabi-gdb -batch -ex "target extended-remote /dev/ttyACM0" \
    -ex "mon verify_image build/bootloader.elf"
```

### LED Status Codes

- **Fast blink (2Hz):** Bootloader running, waiting for timeout
- **Slow blink (0.5Hz):** Application running successfully
- **Solid on:** Error in bootloader (failed to validate app)
- **3 short blinks:** Application validation failed

## Debugging

### Common Issues

**Problem:** Application doesn't start after bootloader jumps
**Solution:** Check VTOR register value and verify application's linker script has correct ORIGIN

**Problem:** HardFault immediately after jump
**Solution:** Verify application's initial stack pointer is valid (points to SRAM)

**Problem:** Can't flash via ST-Link
**Solution:** Check OpenOCD config file and verify ST-Link driver installation

### GDB Commands

```gdb
# Connect to ST-Link
target extended-remote :3333

# Load and verify
load build/bootloader.elf
verify_image build/bootloader.elf

# Set breakpoint and run
break main
continue

# Inspect registers
info registers
print $pc
print $sp

# View memory
x/10x 0x08000000  # View bootloader vector table
x/10x 0x08010000  # View application vector table
```

## Before Making Changes

1. **Read existing code** - Follow established patterns
2. **Check linker scripts** - Ensure sections match
3. **Verify memory placement** - Use `nm` and `objdump`
4. **Test on hardware** - Simulator behavior may differ
5. **Update documentation** - Keep AGENTS.md and docs/ in sync

## Project-Specific Patterns

### Bootloader Jump Sequence

```c
typedef void (*pFunction)(void);

void bootloader_jump_to_app(void) {
    uint32_t app_stack = *(uint32_t *)(APP_START_ADDR);
    uint32_t app_entry = *(uint32_t *)(APP_START_ADDR + 4);

    if (app_stack < SRAM_BASE || app_stack > SRAM_END) {
        return;  // Invalid stack pointer
    }

    if (app_entry < APP_START_ADDR || app_entry > FLASH_END) {
        return;  // Invalid reset handler
    }

    __disable_irq();
    SCB->VTOR = APP_START_ADDR;
    __set_MSP(app_stack);

    pFunction app_reset = (pFunction)app_entry;
    app_reset();
}
```

### Clock Configuration

```c
// Default to 84MHz using HSI (internal 16MHz RC)
void system_clock_config(void) {
    // Enable HSI
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Configure PLL: HSI * 4 / 2 = 32MHz -> * 5.25 = 168MHz / 2 = 84MHz
    // Actually simpler: HSI (16MHz) * 21 / 4 / 1 = 84MHz (using PLLP)
    // See STM32F401 reference manual for PLL formula
}
```

## Additional Resources

- STM32F401 Reference Manual: RM0368
- STM32F4 CMSIS: [GitHub](https://github.com/STMicroelectronics/cmsis_device_f4)
- ARM Cortex-M4 Programming Manual: PM0214
- GNU LD documentation: [Linker Scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)

---

**Last updated:** 2025-03-10
**Maintainer:** Development Team
