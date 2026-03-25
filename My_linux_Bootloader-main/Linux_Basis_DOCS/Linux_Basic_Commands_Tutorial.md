# Linux 基础命令教程

## 目录

- [前言](#前言)
- [文件操作命令](#文件操作命令)
- [文件查看命令](#文件查看命令)
- [文本处理命令](#文本处理命令)
- [实用技巧](#实用技巧)
- [练习示例](#练习示例)

***

## 前言

### 什么是 Linux?

Linux 是一种开源的类 **Unix 操作系统内核**，因其稳定性、安全性和灵活性而广泛应用于服务器、嵌入式设备和个人计算机。

### 为什么学习 Linux 命令?

- **高效**: 命令行可以快速完成复杂的操作
- **强大**: 提供图形界面无法实现的功能
- **远程管理**: 服务器管理必备技能
- **自动化**: 便于编写脚本实现自动化任务
- **STM32 开发**: 嵌入式开发环境配置和构建需要使用命令行

### 终端基础

- WSL: 在 Windows 终端中打开 Ubuntu

**命令格式:**

```bash
命令 [选项] [参数]
```

***

## 文件操作命令

### 1. `ls` - 列出文件和目录

**功能**: 显示当前目录下的文件和子目录

**基本语法:**

```bash
ls [选项] [目录]
```

**常用选项:**

- `-l`: 长格式显示 (详细信息)
- `-a`: 显示所有文件 (包括隐藏文件)
- `-h`: 人类可读格式 (显示文件大小)
- `-R`: 递归显示子目录

**示例:**

```bash
# 列出当前目录文件
ls

# 长格式显示
ls -l

# 显示所有文件 (包括隐藏文件)
ls -a

# 组合选项 (长格式 + 人类可读 + 所有文件)
ls -lah

# 列出指定目录
ls /home

# 递归显示目录树
ls -R
```

**输出说明 (ls -l):**

```
-rw-r--r-- 1 user group 1234 Mar 10 15:30 file.txt
↑         ↑  ↑    ↑      ↑    ↑          ↑
权限     链接数 所有者 组   大小 修改时间  文件名
```

***

### 2. `cd` - 切换目录

**功能**: 改变当前工作目录

**基本语法:**

```bash
cd [目录]
```

**常用路径:**

- `~` (或无参数): 用户主目录
- `-`: 上一个目录
- `..`: 上级目录
- `.`: 当前目录
- `/`: 根目录

**示例:**

```bash
# 切换到主目录
cd ~
# 或
cd

# 切换到上级目录
cd ..

# 切换到上一次所在目录
cd -

# 切换到指定目录 (绝对路径)
cd /usr/local/bin

# 切换到指定目录 (相对路径)
cd Documents/work

# 返回两级目录
cd ../..
```

***

### 3. `pwd` - 显示当前目录

**功能**: 打印当前工作目录的完整路径

**基本语法:**

```bash
pwd [选项]
```

**常用选项:**

- `-P`: 显示物理路径 (不跟随符号链接)

**示例:**

```bash
# 显示当前目录
pwd
# 输出: /home/user/Documents/project

# 显示物理路径
pwd -P
```

***

### 4. `mkdir` - 创建目录

**功能**: 创建新目录

**基本语法:**

```bash
mkdir [选项] 目录名
```

**常用选项:**

- `-p`: 递归创建父目录 (如果父目录不存在)
- `-v`: 显示创建过程
- `-m`: 设置权限 (如 `mkdir -m 755 dir`)

**示例:**

```bash
# 创建单个目录
mkdir my_folder

# 创建多个目录
mkdir dir1 dir2 dir3

# 递归创建目录树
mkdir -p parent/child/grandchild

# 创建目录并显示信息
mkdir -v new_folder
# 输出: mkdir: created directory 'new_folder'

# 创建目录并设置权限
mkdir -m 755 public_folder
```

***

### 5. `rmdir` - 删除空目录

**功能**: 删除空目录 (目录必须为空)

**基本语法:**

```bash
rmdir [选项] 目录名
```

**常用选项:**

- `-p`: 递归删除空父目录
- `-v`: 显示删除过程

**示例:**

```bash
# 删除空目录
rmdir empty_folder

# 递归删除空目录树
rmdir -p parent/child/grandchild

# 删除多个空目录
rmdir dir1 dir2 dir3
```

**注意**: `rmdir` 只能删除空目录,非空目录需使用 `rm -r`

***

### 6. `touch` - 创建空文件

**功能**: 创建空文件或更新文件时间戳

**基本语法:**

```bash
touch [选项] 文件名
```

**常用选项:**

- `-c`: 不创建文件 (仅更新时间戳)
- `-d`: 设置指定时间
- `-a`: 只更新访问时间
- `-m`: 只更新修改时间

**示例:**

```bash
# 创建单个空文件
touch file.txt

# 创建多个文件
touch file1.txt file2.txt file3.txt

# 创建带路径的文件
mkdir -p files && touch files/document.txt

# 创建文件并设置日期
touch -d "2024-01-01" old_file.txt
```

***

### 7. `cp` - 复制文件/目录

**功能**: 复制文件或目录

**基本语法:**

```bash
cp [选项] 源文件 目标文件
cp [选项] 源文件... 目标目录
```

**常用选项:**

- `-r` / `-R`: 递归复制目录
- `-i`: 交互式 (覆盖前询问)
- `-v`: 显示复制过程
- `-p`: 保留文件属性 (权限、时间戳等)
- `-a`: 归档模式 (保留所有属性)

**示例:**

```bash
# 复制文件
cp file.txt file_copy.txt

# 复制文件到目录
cp file.txt Documents/

# 复制多个文件到目录
cp file1.txt file2.txt file3.txt Documents/

# 交互式复制 (覆盖前询问)
cp -i file.txt existing_file.txt
# cp: overwrite 'existing_file.txt'? y

# 递归复制目录
cp -r folder1 folder2

# 复制并显示进度
cp -rv folder1 folder2

# 复制并保留所有属性
cp -pa file.txt file_backup.txt
```

***

### 8. `mv` - 移动/重命名文件

**功能**: 移动文件或目录,或重命名文件

**基本语法:**

```bash
mv [选项] 源文件 目标文件
mv [选项] 源文件... 目标目录
```

**常用选项:**

- `-i`: 交互式 (覆盖前询问)
- `-v`: 显示移动过程
- `-n`: 不覆盖已存在文件
- `-u`: 仅当源文件较新时移动

**示例:**

```bash
# 重命名文件
mv old_name.txt new_name.txt

# 移动文件到目录
mv file.txt Documents/

# 移动多个文件
mv file1.txt file2.txt Documents/

# 交互式移动 (覆盖前询问)
mv -i file.txt existing_file.txt

# 移动目录
mv old_folder new_location/

# 移动并显示过程
mv -v file.txt Documents/
```

***

### 9. `rm` - 删除文件/目录

**功能**: 删除文件或目录

**基本语法:**

```bash
rm [选项] 文件...
```

**常用选项:**

- `-r` / `-R`: 递归删除目录
- `-i`: 交互式 (删除前询问)
- `-f`: 强制删除 (不询问)
- `-v`: 显示删除过程

**示例:**

```bash
# 删除文件
rm file.txt

# 删除多个文件
rm file1.txt file2.txt file3.txt

# 交互式删除 (每个文件都询问)
rm -i *.txt

# 递归删除目录
rm -r folder

# 强制递归删除 (谨慎使用!)
rm -rf folder

# 删除并显示过程
rm -rv folder
```

**⚠️ 危险命令警告:**

```bash
# 绝对不要运行这些命令!
rm -rf /          # 删除根目录 (系统毁灭)
rm -rf ~          # 删除主目录所有内容
rm -rf .*         # 删除所有隐藏文件和目录
```

**安全提示:**

- 删除前先用 `ls` 确认内容
- 使用 `rm -i` 交互式删除
- 重要操作前先备份

***

### 10. `file` - 查看文件类型

**功能**: 确定文件类型

**基本语法:**

```bash
file [选项] 文件...
```

**示例:**

```bash
# 查看文件类型
file document.txt
# 输出: document.txt: ASCII text

# 查看多个文件
file *.txt

# 查看二进制文件
file program
# 输出: program: ELF 64-bit LSB executable, x86-64

# 查看目录
file /home
# 输出: /home: directory
```

***

## 文件查看命令

### 1. `cat` - 显示文件内容

**功能**: 连接并显示文件内容

**基本语法:**

```bash
cat [选项] 文件...
```

**常用选项:**

- `-n`: 显示行号
- `-b`: 对非空行编号
- `-s`: 压缩多行空行为一行
- `-E`: 显示行尾符号 ($)

**示例:**

```bash
# 显示文件内容
cat file.txt

# 显示多个文件
cat file1.txt file2.txt

# 显示行号
cat -n file.txt

# 合并文件
cat file1.txt file2.txt > combined.txt

# 显示文件并添加行号 (仅非空行)
cat -b file.txt
```

***

### 2. `less` / `more` - 分页查看

**功能**: 分页显示文件内容 (适合查看大文件)

#### `less` (推荐)

**基本语法:**

```bash
less 文件名
```

**操作快捷键:**

- `空格` / `Page Down`: 下一页
- `b` / `Page Up`: 上一页
- `Enter`: 下一行
- `k`: 上一行
- `/字符串`: 向下搜索
- `?字符串`: 向上搜索
- `n`: 下一个搜索结果
- `N`: 上一个搜索结果
- `g`: 跳到文件开头
- `G`: 跳到文件结尾
- `q`: 退出

**示例:**

```bash
# 查看大文件
less large_file.txt

# 查看并高亮搜索
less /var/log/syslog
# 然后输入 /error 搜索 "error"

# 查看多个文件
less file1.txt file2.txt
# 使用 :n 下一个文件, :p 上一个文件
```

#### `more` (简单版)

**基本语法:**

```bash
more 文件名
```

**操作快捷键:**

- `空格`: 下一页
- `Enter`: 下一行
- `b`: 上一页
- `q`: 退出

**示例:**

```bash
more file.txt
```

***

### 3. `head` - 查看文件开头

**功能**: 显示文件的前几行

**基本语法:**

```bash
head [选项] 文件
```

**常用选项:**

- `-n 数字`: 显示前 n 行 (默认 10 行)
- `-c 字节数`: 显示前 n 个字节

**示例:**

```bash
# 显示前 10 行 (默认)
head file.txt

# 显示前 20 行
head -n 20 file.txt
# 或简写
head -20 file.txt

# 显示前 100 字节
head -c 100 file.txt

# 查看多个文件开头
head -n 5 file1.txt file2.txt
```

***

### 4. `tail` - 查看文件结尾

**功能**: 显示文件的最后几行

**基本语法:**

```bash
tail [选项] 文件
```

**常用选项:**

- `-n 数字`: 显示最后 n 行 (默认 10 行)
- `-f`: 实时跟踪文件更新 (监控日志)
- `-c 字节数`: 显示最后 n 个字节

**示例:**

```bash
# 显示最后 10 行 (默认)
tail file.txt

# 显示最后 20 行
tail -n 20 file.txt
# 或简写
tail -20 file.txt

# 实时监控日志文件 (Ctrl+C 退出)
tail -f /var/log/syslog

# 显示最后 100 字节
tail -c 100 file.txt

# 查看多个文件结尾
tail -n 5 file1.txt file2.txt
```

**实用场景:**

```bash
# 监控应用日志
tail -f application.log

# 查看最近的错误
tail -f error.log | grep ERROR
```

***

## 文本处理命令

### 1. `grep` - 文本搜索

**功能**: 在文件中搜索指定文本模式

**基本语法:**

```bash
grep [选项] "模式" 文件
grep [选项] "模式"  # 从标准输入搜索
```

**常用选项:**

- `-i`: 忽略大小写
- `-r` / `-R`: 递归搜索目录
- `-n`: 显示行号
- `-v`: 反向匹配 (显示不包含模式的行)
- `-c`: 只显示匹配行数
- `-l`: 只显示包含匹配的文件名
- `-E`: 使用扩展正则表达式
- `-o`: 只输出匹配部分

**示例:**

```bash
# 在文件中搜索文本
grep "hello" file.txt

# 忽略大小写搜索
grep -i "error" log.txt

# 递归搜索目录
grep -r "TODO" ./src

# 显示行号
grep -n "function" code.c

# 反向匹配 (不包含注释的行)
grep -v "^#" config.txt

# 统计匹配行数
grep -c "ERROR" log.txt

# 只显示文件名 (哪些文件包含搜索内容)
grep -l "include" *.c

# 扩展正则表达式
grep -E "error|warning|fail" log.txt

# 显示匹配部分
grep -o "[0-9]+" file.txt

# 精确匹配单词
grep -w "main" code.c
```

**常用搜索模式:**

```bash
# 搜索数字
grep "[0-9]" file.txt

# 搜索以特定字符串开头的行
grep "^#include" code.c

# 搜索以特定字符串结尾的行
grep ";" code.c

# 搜索空行
grep "^$" file.txt

# 搜索 IP 地址
grep -E "[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}" log.txt
```

***

### 2. `find` - 查找文件

**功能**: 在目录树中查找文件

**基本语法:**

```bash
find [路径] [选项]
```

**常用条件:**

- `-name`: 按文件名查找
- `-type`: 按类型查找 (f=文件, d=目录)
- `-size`: 按大小查找
- `-mtime`: 按修改时间查找
- `-perm`: 按权限查找
- `-user`: 按所有者查找

**常用动作:**

- `-print`: 打印结果 (默认)
- `-delete`: 删除找到的文件
- `-exec`: 对结果执行命令

**示例:**

```bash
# 在当前目录查找文件
find . -name "file.txt"

# 忽略大小写查找
find . -iname "*.txt"

# 查找所有 .c 文件
find . -name "*.c"

# 查找目录
find . -type d -name "src"

# 只查找文件
find . -type f

# 查找大于 10MB 的文件
find . -size +10M

# 查找小于 1KB 的文件
find . -size -1k

# 查找 7 天前修改的文件
find . -mtime +7

# 查找 7 天内修改的文件
find . -mtime -7

# 查找并删除 .tmp 文件
find . -name "*.tmp" -delete

# 查找并执行命令
find . -name "*.c" -exec cat {} \;

# 查找并复制文件
find . -name "*.txt" -exec cp {} backup/ \;

# 组合条件 (查找 .c 文件且大小大于 1KB)
find . -name "*.c" -size +1k

# 或条件 (查找 .c 或 .h 文件)
find . \( -name "*.c" -o -name "*.h" \)
```

**实用场景:**

```bash
# 查找所有空文件
find . -type f -empty

# 查找所有空目录
find . -type d -empty

# 查找权限为 777 的文件
find . -perm 777

# 查找属于特定用户的文件
find . -user username

# 查找并统计代码行数
find . -name "*.c" -exec wc -l {} + | tail -1
```

***

### 3. `wc` - 统计行数、字数

**功能**: 统计文件的行数、字数、字节数

**基本语法:**

```bash
wc [选项] 文件...
```

**常用选项:**

- `-l`: 只统计行数
- `-w`: 只统计字数
- `-c`: 只统计字节数
- `-m`: 只统计字符数

**示例:**

```bash
# 显示所有统计信息
wc file.txt
# 输出:  10  50 250 file.txt
#       行  字  字节

# 只统计行数
wc -l file.txt

# 只统计字数
wc -w file.txt

# 统计多个文件
wc file1.txt file2.txt

# 统计所有 .c 文件的行数
wc -l *.c

# 统计代码总行数
find . -name "*.c" | xargs wc -l | tail -1
```

***

### 4. `sort` - 排序

**功能**: 对文本行进行排序

**基本语法:**

```bash
sort [选项] 文件
```

**常用选项:**

- `-n`: 按数值排序
- `-r`: 反向排序 (降序)
- `-u`: 去除重复行
- `-k`: 按指定字段排序
- `-t`: 指定字段分隔符

**示例:**

```bash
# 默认按字母排序
sort names.txt

# 按数值排序
sort -n numbers.txt

# 反向排序
sort -r numbers.txt

# 去除重复行并排序
sort -u file.txt

# 按第 2 个字段排序
sort -k 2 data.txt

# 按冒号分隔的第 3 个字段排序
sort -t: -k 3 /etc/passwd

# 按数值反向排序
sort -nr numbers.txt

# 组合使用
sort -u names.txt > sorted_unique.txt
```

***

### 5. `uniq` - 去重

**功能:** 报告或忽略重复的行

**基本语法:**

```bash
uniq [选项] 文件
```

**常用选项:**

- `-c`: 显示重复次数
- `-d`: 只显示重复的行
- `-u`: 只显示唯一的行
- `-i`: 忽略大小写

**示例:**

```bash
# 删除相邻重复行
uniq file.txt

# 显示重复次数
uniq -c file.txt
# 输出:
#   3 hello
#   1 world
#   2 linux

# 只显示重复的行
uniq -d file.txt

# 只显示唯一的行
uniq -u file.txt

# 忽略大小写
uniq -i file.txt

# 统计每个值出现的次数
sort file.txt | uniq -c

# 先排序再去重 (去除所有重复)
sort file.txt | uniq
```

**注意**: `uniq` 只能去除相邻的重复行,通常需要先 `sort`

***

## 实用技巧

### 1. Tab 键自动补全

**功能**: 自动补全命令、文件名、路径

**使用方法:**

```bash
# 输入命令开头,按 Tab
cd Doc[Tab]  # 补全为 cd Documents/

# 如果有多个匹配,按两次 Tab 显示所有选项
ls D[Tab][Tab]  # 显示 Documents/ Downloads/
```

***

### 2. 命令历史记录

**功能**: 查看和重用之前执行的命令

**快捷键:**

- `↑` / `↓`: 上一条/下一条命令
- `Ctrl + r`: 搜索历史命令
- `!n`: 执行历史中第 n 条命令
- `!!`: 执行上一条命令
- `!字符串`: 执行最近一条以该字符串开头的命令

**命令:**

```bash
# 显示历史记录
history

# 执行历史中的第 100 条命令
!100

# 执行上一条命令
!!

# 执行最近一条包含 "grep" 的命令
!grep

# 清空历史记录
history -c
```

***

### 3. 通配符

**功能**: 匹配多个文件

**常用通配符:**

- `*`: 匹配任意字符 (0 个或多个)
- `?`: 匹配单个字符
- `[...]`: 匹配括号中任意一个字符
- `[a-z]`: 匹配 a-z 范围
- `[0-9]`: 匹配数字

**示例:**

```bash
# 匹配所有 .txt 文件
ls *.txt

# 匹配所有文件
ls *

# 匹配单个字符
ls file?.txt  # 匹配 file1.txt, filea.txt 等

# 匹配范围
ls file[1-3].txt  # 匹配 file1.txt, file2.txt, file3.txt

# 匹配多个字符
ls [ch]*.txt  # 匹配 c*.txt 和 h*.txt

# 排除匹配
ls *[!0-9].txt  # 匹配不以数字结尾的 .txt 文件
```

***

### 4. 管道和重定向

#### 管道 `|`

**功能**: 将一个命令的输出作为另一个命令的输入

**示例:**

```bash
# 将 ls 的输出传递给 grep
ls -l | grep ".txt"

# 将 find 的结果传递给 wc
find . -name "*.c" | wc -l

# 多个管道
cat file.txt | grep "error" | wc -l

# 查看进程并搜索
ps aux | grep nginx

# 查看长文件并搜索
less /var/log/syslog | grep ERROR
```

#### 重定向 `>`, `>>`, `<`

**功能**: 重定向输入/输出

**示例:**

```bash
# 将输出写入文件 (覆盖)
echo "Hello" > file.txt

# 将输出追加到文件
echo "World" >> file.txt

# 将错误输出到文件
command 2> error.log

# 同时输出标准输出和错误
command > output.log 2>&1

# 从文件读取输入
grep "pattern" < input.txt

# 同时重定向输入和输出
grep "pattern" < input.txt > output.txt
```

***

## 练习示例

### 练习 1: 文件和目录管理

创建以下目录结构:

```
practice/
├── documents/
│   ├── notes.txt
│   └── readme.md
├── code/
│   ├── hello.c
│   └── world.c
└── backup/
```

**命令:**

```bash
# 创建目录结构
mkdir -p practice/{documents,code,backup}

# 创建文件
touch practice/documents/notes.txt
touch practice/documents/readme.md
touch practice/code/hello.c
touch practice/code/world.c

# 验证结构
tree practice
# 或
find practice -print
```

***

### 练习 2: 文件内容操作

**步骤:**

1. 向文件写入内容
2. 查看文件内容
3. 搜索特定文本
4. 统计信息

**命令:**

```bash
# 写入内容
echo "Hello Linux" > practice/documents/notes.txt
echo "This is a practice file" >> practice/documents/notes.txt
echo "Linux is powerful" >> practice/documents/notes.txt

# 查看内容
cat practice/documents/notes.txt

# 搜索包含 "Linux" 的行
grep "Linux" practice/documents/notes.txt

# 统计行数
wc -l practice/documents/notes.txt

# 按字母排序行内容
sort practice/documents/notes.txt
```

***

### 练习 3: 查找和统计

**任务:**

1. 查找所有 .c 文件
2. 统计所有 .c 文件的行数
3. 查找包含特定文本的文件

**命令:**

```bash
# 查找所有 .c 文件
find practice -name "*.c"

# 统计 .c 文件数量
find practice -name "*.c" | wc -l

# 在所有文件中搜索 "hello"
grep -r "hello" practice/

# 查找并显示行号
grep -rn "hello" practice/
```

***

### 练习 4: 备份和清理

**任务:**

1. 复制文件到备份目录
2. 创建压缩备份 (需要安装 tar)
3. 清理临时文件

**命令:**

```bash
# 复制所有文档到备份
cp -r practice/documents/* practice/backup/

# 验证备份
ls -la practice/backup/

# 创建 tar 归档 (可选)
tar -czf backup.tar.gz practice/

# 清理 (谨慎!)
# rm -i practice/code/*.c  # 交互式删除
```

***

## 常见问题

### Q1: 命令输入错误如何修改?

**A:** 使用快捷键:

- `Ctrl + a`: 跳到行首
- `Ctrl + e`: 跳到行尾
- `Ctrl + u`: 删除到行首
- `Ctrl + k`: 删除到行尾
- `Ctrl + w`: 删除前一个单词

***

### Q2: 如何查看命令帮助?

**A:** 使用以下方法:

```bash
# 内置命令帮助
help cd

# 程序手册页
man ls

# 简要帮助
ls --help

# 显示命令路径
which ls

# 显示命令类型
type ls
```

***

### Q3: 如何停止正在运行的命令?

**A:** 使用快捷键:

- `Ctrl + c`: 中断当前命令
- `Ctrl + d`: 退出当前 shell
- `Ctrl + z`: 暂停当前命令 (后台运行)

***

### Q4: 如何清理终端屏幕?

**A:** 使用命令:

```bash
clear
# 或快捷键
Ctrl + l
```

***

### Q5: 命令执行权限被拒绝?

**A:** 使用 `sudo` 提升权限:

```bash
# 需要 root 权限的命令
sudo apt update
sudo reboot

# 添加执行权限
chmod +x script.sh
./script.sh
```

***

## 参考资源

### 在线资源

- **Linux 命令手册**: <https://linux.die.net/>
- **GNU Coreutils**: <https://www.gnu.org/software/coreutils/manual/>
- **Ubuntu 文档**: <https://ubuntu.com/server/docs>

### 相关书籍

- 《Linux 命令行与 shell 脚本编程大全》
- 《鸟哥的 Linux 私房菜》
- 《Linux 性能优化实战》

### STM32 相关

- **项目构建**: 使用 `./build.sh` 命令
- **烧录工具**: `tools/` 目录中的脚本
- **调试命令**: GDB 和 OpenOCD 相关命令

***

## 总结

### 关键命令速查

| 命令          | 功能      |
| ----------- | ------- |
| `ls`        | 列出文件    |
| `cd`        | 切换目录    |
| `pwd`       | 显示当前目录  |
| `mkdir`     | 创建目录    |
| `rm`        | 删除文件    |
| `cp`        | 复制文件    |
| `mv`        | 移动文件    |
| `cat`       | 显示文件内容  |
| `grep`      | 搜索文本    |
| `find`      | 查找文件    |
| `wc`        | 统计行数    |
| `sort`      | 排序      |
| `head/tail` | 查看文件头/尾 |

### 学习建议

1. **循序渐进**: 从基本命令开始,逐步学习复杂命令
2. **动手实践**: 多练习,熟能生巧
3. **查看帮助**: 遇到问题使用 `man` 或 `--help`
4. **记录笔记**: 记录常用命令和技巧
5. **实际应用**: 在 STM32 开发中运用这些命令

### 下一步学习

- Shell 脚本编程
- 系统管理命令
- 网络命令
- 进程管理
- Vim/Emacs 编辑器

***

**文档版本**: 1.0\
**最后更新**: 2024-03-11\
**适用环境**: Linux/WSL Ubuntu/Debian\
**维护者**: STM32 Bootloader 项目团队
