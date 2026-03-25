# Makefile 基础语法详解

本文档旨在作为 [Makefile 教程](./MAKEFILE_TUTORIAL.md) 的补充，详细解释 Makefile 的核心语法和概念。

## 1. Makefile 的核心结构

Makefile 的核心由 **规则 (Rules)** 组成。一个规则通常长这样：

```makefile
target: prerequisites
    command
    command
    ...
```

- **target (目标)**: 你想生成的文件（如 `main.o`）或执行的动作（如 `clean`）。
- **prerequisites (依赖)**: 生成目标所需要的文件。如果依赖文件比目标文件更新，或者目标文件不存在，Make 就会执行命令。
- **command (命令)**: 构建目标的 Shell 命令。
  - **重要**: 命令前必须使用 **Tab 键** 缩进，不能使用空格！

## 2. 变量 (Variables)

变量用于简化 Makefile，提高可读性和可维护性。

### 2.1 赋值方式

- **`=`** **(递归展开赋值)**: 变量的值在**使用时**才确定。
  ```makefile
  FOO = $(BAR)
  BAR = 1
  # 此时 $(FOO) 为 1
  ```
- **`:=`** **(立即展开赋值)**: 变量的值在**定义时**就确定。**推荐使用**，因为它更高效且行为更可预测。
  ```makefile
  FOO := $(BAR)
  BAR := 1
  # 此时 $(FOO) 为空，因为定义 FOO 时 BAR 还没定义
  ```
- **`?=`** **(条件赋值)**: 如果变量未定义，则赋值；否则保持原值。
  ```makefile
  CC ?= gcc
  # 如果环境变量或命令行没指定 CC，则 CC 为 gcc
  ```
- **`+=`** **(追加赋值)**: 向变量追加值。
  ```makefile
  CFLAGS := -Wall
  CFLAGS += -O2
  # CFLAGS 为 "-Wall -O2"
  ```

## 3. 自动变量 (Automatic Variables)

在规则的命令中，可以使用自动变量来引用目标和依赖，这样可以写出通用的规则。

- **`$@`**: **目标**的文件名。
- **`$<`**: **第一个依赖**的文件名。
- **`$^`**: **所有依赖**的文件名（去重）。
- **`$+`**: **所有依赖**的文件名（不去重）。

**示例**:

```makefile
main.o: main.c main.h
    $(CC) -c $< -o $@
# 等价于: gcc -c main.c -o main.o
```

## 4. 模式规则 (Pattern Rules)

模式规则允许你为某种类型的文件定义通用的构建规则，使用 `%` 作为通配符。

**示例**: 将所有 `.c` 文件编译为 `.o` 文件

```makefile
%.o: %.c
    $(CC) -c $(CFLAGS) $< -o $@
```

如果 Make 需要生成 `utils.o`，它会自动寻找 `utils.c`，并套用这个规则。

## 5. 常用函数 (Functions)

Makefile 提供了许多内置函数，调用格式为 `$(function arguments)`。

- **`wildcard`**: 获取匹配模式的文件列表。
  ```makefile
  SRC_FILES := $(wildcard src/*.c)
  # 结果可能是: src/main.c src/utils.c
  ```
- **`patsubst`**: 模式替换。
  ```makefile
  OBJ_FILES := $(patsubst %.c,%.o,$(SRC_FILES))
  # 将 src/main.c 替换为 src/main.o
  ```
- **`addprefix`**: 添加前缀。
  ```makefile
  OBJS := $(addprefix build/, main.o utils.o)
  # 结果: build/main.o build/utils.o
  ```
- **`dir`**: 取目录部分。
- **`notdir`**: 取文件名部分。

## 6. 伪目标 (.PHONY)

如果一个目标不是一个真实的文件，而是一个动作（如 `clean`, `all`, `install`），应该声明为伪目标。

**为什么要声明 .PHONY?**
如果不声明，且目录下刚好有一个叫 `clean` 的文件，执行 `make clean` 时，Make 会认为目标已存在且是最新的，从而不执行命令。

```makefile
.PHONY: all clean

clean:
    rm -rf build
```

## 7. 控制指令

- **`include`**: 包含其他 Makefile。
  ```makefile
  include config.mk
  ```
- **`ifeq`** **/** **`ifneq`**: 条件判断。
  ```makefile
  ifeq ($(DEBUG), 1)
      CFLAGS += -g
  endif
  ```
- **`-`** **(命令前缀)**: 忽略错误。
  ```makefile
  clean:
      -rm -f *.o
  # 即使 rm 报错（例如文件不存在），Make 也会继续执行
  ```
- **`@`** **(命令前缀)**: 不回显命令本身。
  ```makefile
  echo-msg:
      @echo "Hello World"
  # 只输出 "Hello World"，不会输出 "echo "Hello World""
  ```

## 8. 综合示例

```makefile
# 定义编译器
CC := gcc
# 定义编译选项
CFLAGS := -Wall -O2
# 定义目标文件
TARGET := my_app

# 获取当前目录下所有 .c 文件
SRCS := $(wildcard *.c)
# 将 .c 替换为 .o
OBJS := $(patsubst %.c, %.o, $(SRCS))

# 伪目标
.PHONY: all clean

# 默认目标
all: $(TARGET)

# 链接规则
$(TARGET): $(OBJS)
    $(CC) $(OBJS) -o $@

# 编译规则 (模式规则)
%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

# 清理规则
clean:
    rm -f $(OBJS) $(TARGET)
```

