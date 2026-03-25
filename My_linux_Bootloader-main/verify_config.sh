#!/bin/bash

# STM32F401RET6 配置验证脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}STM32F401RET6 配置验证${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 检查工具链
echo -e "${YELLOW}[1/5] 检查工具链...${NC}"
if command -v arm-none-eabi-gcc &> /dev/null; then
    echo -e "${GREEN}✓ arm-none-eabi-gcc 已安装${NC}"
    arm-none-eabi-gcc --version | head -n1
else
    echo -e "${RED}✗ arm-none-eabi-gcc 未安装${NC}"
    echo "  安装命令: sudo apt-get install gcc-arm-none-eabi"
fi
echo ""

# 检查链接脚本
echo -e "${YELLOW}[2/5] 检查链接脚本配置...${NC}"

# 检查Bootloader链接脚本
if grep -q "LENGTH = 96K" bootloader/ld/STM32F401VCTx_BOOTLOADER.ld; then
    echo -e "${GREEN}✓ Bootloader SRAM: 96KB${NC}"
else
    echo -e "${RED}✗ Bootloader SRAM 配置错误${NC}"
fi

if grep -q "LENGTH = 64K" bootloader/ld/STM32F401VCTx_BOOTLOADER.ld; then
    echo -e "${GREEN}✓ Bootloader FLASH: 64KB${NC}"
else
    echo -e "${RED}✗ Bootloader FLASH 配置错误${NC}"
fi

# 检查Application链接脚本
if grep -q "LENGTH = 96K" application/ld/STM32F401VCTx_APPLICATION.ld; then
    echo -e "${GREEN}✓ Application SRAM: 96KB${NC}"
else
    echo -e "${RED}✗ Application SRAM 配置错误${NC}"
fi

if grep -q "LENGTH = 448K" application/ld/STM32F401VCTx_APPLICATION.ld; then
    echo -e "${GREEN}✓ Application FLASH: 448KB${NC}"
else
    echo -e "${RED}✗ Application FLASH 配置错误${NC}"
fi
echo ""

# 检查头文件
echo -e "${YELLOW}[3/5] 检查头文件配置...${NC}"

if grep -q "SRAM_END.*0x20017FFF" bootloader/inc/main.h; then
    echo -e "${GREEN}✓ SRAM_END: 0x20017FFF (96KB)${NC}"
else
    echo -e "${RED}✗ SRAM_END 配置错误${NC}"
fi

if grep -q "APP_END_ADDR.*0x0807FFFF" bootloader/inc/main.h; then
    echo -e "${GREEN}✓ APP_END_ADDR: 0x0807FFFF (512KB)${NC}"
else
    echo -e "${RED}✗ APP_END_ADDR 配置错误${NC}"
fi
echo ""

# 显示内存布局
echo -e "${YELLOW}[4/5] 内存布局...${NC}"
echo ""
echo "Flash (512KB):"
echo "  0x08000000 - 0x0800FFFF  Bootloader (64KB)"
echo "  0x08010000 - 0x0807FFFF  Application (448KB)"
echo ""
echo "SRAM (96KB):"
echo "  0x20000000 - 0x20017FFF  Main SRAM"
echo ""

# 检查二进制文件
echo -e "${YELLOW}[5/5] 检查编译输出...${NC}"

if [ -f "bootloader/build/bootloader.elf" ]; then
    echo -e "${GREEN}✓ Bootloader ELF 存在${NC}"
    arm-none-eabi-size bootloader/build/bootloader.elf
else
    echo -e "${YELLOW}⚠ Bootloader 未编译${NC}"
    echo "  运行: ./build.sh bootloader"
fi
echo ""

if [ -f "application/build/application.elf" ]; then
    echo -e "${GREEN}✓ Application ELF 存在${NC}"
    arm-none-eabi-size application/build/application.elf
else
    echo -e "${YELLOW}⚠ Application 未编译${NC}"
    echo "  运行: ./build.sh application"
fi
echo ""

# 检查应用程序的VMA地址
if [ -f "application/build/application.elf" ]; then
    echo -e "${YELLOW}验证应用程序链接地址...${NC}"
    VMA=$(arm-none-eabi-objdump -h application/build/application.elf | grep "\.isr_vector" | awk '{print $5}')
    if [ "$VMA" == "08010000" ]; then
        echo -e "${GREEN}✓ 应用程序正确链接到 0x08010000${NC}"
    else
        echo -e "${RED}✗ 应用程序链接地址错误: $VMA${NC}"
    fi
    echo ""
fi

# 总结
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}验证完成${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "下一步:"
echo -e "  1. 编译项目: ${GREEN}./build.sh all${NC}"
echo -e "  2. 启动OpenOCD: ${GREEN}openocd -f tools/openocd_stm32f4.cfg${NC}"
echo -e "  3. 烧录程序: ${GREEN}./tools/flash_all.sh${NC}"
echo ""
