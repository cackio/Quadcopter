# Makefile 教程 - 以 Bootloader 为例

本文档通过剖析 `bootloader/Makefile`，详细讲解 Makefile 的编写与工作原理。

## 1. 什么是 Makefile？

Makefile 是一个定义了项目编译规则的文件。通过 `make` 命令，我们可以自动化编译过程，仅重新编译修改过的文件，从而节省时间。

## 2. Bootloader Makefile 逐行解析

我们将以 `bootloader/Makefile` 为例进行讲解。

### 2.1 目标名称 (Target)

```makefile
######################################
# target
######################################
TARGET = bootloader
```

- **`TARGET`**: 定义了最终生成文件的基础名称。
- 在这里，最终会生成 `bootloader.elf`, `bootloader.hex`, `bootloader.bin`。

### 2.2 构建变量 (Build Variables)

```makefile
######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og
```

- **`DEBUG = 1`**: 这是一个开关变量。如果设为 1，通常会添加调试信息（如 `-g`）。
- **`OPT = -Og`**: 定义优化级别。
  - `-O0`: 无优化。
  - `-Og`: 针对调试优化的级别（推荐开发时使用）。
  - `-O2`, `-O3`: 高级优化（用于发布版本，但可能导致调试困难）。

### 2.3 路径定义

```makefile
#######################################
# paths
#######################################
# Build path
BUILD_DIR = build
```

- **`BUILD_DIR`**: 定义了所有编译中间文件（`.o`, `.d`）和最终产物存放的目录。
- **好处**: 保持源代码目录整洁，`make clean` 时只需删除 `build` 目录即可。

### 2.4 源文件 (Source Files)

```makefile
######################################
# source
######################################
# C sources
C_SOURCES =  \
../bootloader/src/main.c \
../common/cmsis/device/system_stm32f4xx.c

# ASM sources
ASM_SOURCES =  \
../common/startup/startup_stm32f401xe.s
```

- **`C_SOURCES`**: 列出所有需要编译的 `.c` 文件。
- **`ASM_SOURCES`**: 列出所有需要编译的汇编文件（`.s`）。
- **`\`**: 续行符。Makefile 中一行太长时，可以用反斜杠换行。
- **注意**: 这里的路径是相对于 Makefile 所在目录的相对路径。

### 2.5 工具链定义 (Binaries)

```makefile
#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
```

- **`PREFIX`**: 交叉编译工具链的前缀。因为我们是在 PC (x86) 上为 ARM 编译，所以使用 `arm-none-eabi-`。
- **`CC`**: C 编译器 (gcc)。
- **`AS`**: 汇编器 (使用 gcc 处理汇编)。
- **`CP`**: `objcopy` 工具，用于将 `.elf` 格式转换为 `.bin` 或 `.hex`。
- **`SZ`**: `size` 工具，用于查看编译后的代码大小。

### 2.6 编译器标志 (Compiler Flags)

```makefile
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
```

- **`MCU`**: 汇总了所有硬件相关的编译选项。
  - `-mcpu=cortex-m4`: 指定处理器内核。
  - `-mthumb`: 使用 Thumb 指令集（ARM Cortex-M 必须）。
  - `-mfloat-abi=hard`: 使用硬件浮点单元 (FPU)。

```makefile
# C defines
C_DEFS =  \
-DSTM32F401xE

# C includes
C_INCLUDES =  \
-I../bootloader/inc \
-I../common/cmsis/core \
-I../common/cmsis/device
```

- **`C_DEFS`**: 定义宏。`-DSTM32F401xE` 相当于在代码中写了 `#define STM32F401xE`。
- **`C_INCLUDES`**: 指定头文件搜索路径 (`-I`)。

```makefile
# compile gcc flags
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
```

- **`CFLAGS`**: 最终传递给编译器的所有标志。
- **`-Wall`**: 开启所有警告 (Warning All)。
- **`-fdata-sections -ffunction-sections`**: 将每个变量和函数放在独立的段中。配合链接器的 `--gc-sections`，可以删除未使用的代码，减小体积。
- **依赖生成**: `-MMD -MP ...` 用于自动生成头文件依赖 (`.d` 文件)。如果头文件被修改，Make 会自动重新编译包含它的源文件。

### 2.7 链接器标志 (LDFLAGS)

```makefile
#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = ../bootloader/ld/STM32F401RET6_BOOTLOADER.ld

# libraries
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
```

- **`LDSCRIPT`**: 指定链接脚本 (`.ld` 文件)。链接脚本决定了代码和变量在 Flash 和 RAM 中的具体地址。
- **`-specs=nano.specs`**: 使用 newlib-nano 库，这是一个针对嵌入式优化的 C 标准库，体积更小。
- **`-Wl,--gc-sections`**: 告诉链接器删除未使用的段（配合前面的 `-ffunction-sections` 使用）。
- **`-Wl,-Map=...`**: 生成 .map 文件，这是一个非常有用的文件，列出了所有符号的地址和大小。

### 2.8 构建规则 (Build Rules)

这是 Makefile 的核心部分，定义了“如何生成目标”。

```makefile
# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
```

- **`all`**: 默认目标。当你只输入 `make` 时，它会执行这个目标。
- 它依赖于 `.elf`, `.hex`, `.bin` 文件。

```makefile
#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
```

- **`OBJECTS`**: 将源文件列表 (`.c`) 转换为对应的对象文件列表 (`.o`)，并加上构建目录前缀。
- **`vpath`**: 告诉 Make 在哪里寻找源文件。

```makefile
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
```

- **编译规则**: 如何将 `.c` 文件编译成 `.o` 文件。
- `$<`: 第一个依赖文件（即 `.c` 文件）。
- `$@`: 目标文件（即 `.o` 文件）。
- `| $(BUILD_DIR)`: 确保在编译前 `build` 目录已经存在。

```makefile
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@
```

- **链接规则**: 将所有 `.o` 文件链接成一个 `.elf` 文件。
- 最后运行 `size` 工具显示大小。

```makefile
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@
```

- **格式转换**: 将 `.elf` 转换为 `.hex` 和 `.bin`。

### 2.9 其他规则

```makefile
$(BUILD_DIR):
	mkdir $@

clean:
	-rm -fR $(BUILD_DIR)
```

- **`$(BUILD_DIR)`**: 创建构建目录的规则。
- **`clean`**: 清理规则。删除整个 `build` 目录。

```makefile
-include $(wildcard $(BUILD_DIR)/*.d)
```

- **包含依赖**: 将编译器生成的 `.d` 文件包含进来，实现头文件修改后的自动重编译。

## 3. 常用命令

在 `bootloader` 目录下：

- **`make`** 或 **`make all`**: 编译整个项目。
- **`make clean`**: 删除构建产物。

## 4. 总结

这个 Makefile 是一个典型的嵌入式 C 项目构建脚本。它涵盖了：

1. **交叉编译配置** (ARM GCC)。
2. **源文件管理** (C 和汇编)。
3. **编译选项优化** (大小优化、调试信息)。
4. **依赖管理** (自动处理头文件依赖)。
5. **多格式输出** (ELF, HEX, BIN)。

