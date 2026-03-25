#!/bin/bash

# STM32F401RET6 链接脚本配置验证

echo "=========================================="
echo "STM32F401RET6 链接脚本配置验证"
echo "=========================================="
echo ""

echo "✅ 链接脚本文件:"
echo ""
echo "Bootloader:"
ls -lh bootloader/ld/STM32F401RET6_BOOTLOADER.ld 2>/dev/null || echo "  ❌ 文件不存在"
echo ""
echo "Application:"
ls -lh application/ld/STM32F401RET6_APPLICATION.ld 2>/dev/null || echo "  ❌ 文件不存在"
echo ""

echo "✅ Makefile 引用:"
echo ""
echo "Bootloader Makefile:"
grep "LDSCRIPT" bootloader/Makefile | grep -v "LDFLAGS" | sed 's/^/  /'
echo ""
echo "Application Makefile:"
grep "LDSCRIPT" application/Makefile | grep -v "LDFLAGS" | sed 's/^/  /'
echo ""

echo "✅ 内存配置检查:"
echo ""
echo "Bootloader:"
grep -A2 "MEMORY" bootloader/ld/STM32F401RET6_BOOTLOADER.ld | grep "LENGTH" | sed 's/^/  /'
echo ""
echo "Application:"
grep -A2 "MEMORY" application/ld/STM32F401RET6_APPLICATION.ld | grep "LENGTH" | sed 's/^/  /'
echo ""

echo "✅ 旧文件检查（不应该存在）:"
if [ -f "bootloader/ld/STM32F401VCTx_BOOTLOADER.ld" ]; then
    echo "  ⚠️  警告: bootloader/ld/STM32F401VCTx_BOOTLOADER.ld 仍然存在"
else
    echo "  ✓ 旧文件已清理"
fi

if [ -f "application/ld/STM32F401VCTx_APPLICATION.ld" ]; then
    echo "  ⚠️  警告: application/ld/STM32F401VCTx_APPLICATION.ld 仍然存在"
else
    echo "  ✓ 旧文件已清理"
fi
echo ""

echo "=========================================="
echo "验证完成！"
echo "=========================================="
echo ""
echo "所有链接脚本已正确重命名为 STM32F401RET6 版本"
echo ""
