# STM32F401RET6 Bootloader 快速参考

## 内存布局

| 区域          | 起始地址       | 大小     | 用途   |
| ----------- | ---------- | ------ | ---- |
| Bootloader  | 0x08000000 | 64 KB  | 引导程序 |
| Application | 0x08010000 | 448 KB | 应用程序 |
| SRAM        | 0x20000000 | 96 KB  | 内存   |

## LED 状态

| LED          | 含义              |
| ------------ | --------------- |
| 快速闪烁 (2Hz)   | Bootloader运行    |
| 慢速闪烁 (0.5Hz) | Application运行 ✅ |
| 常亮           | 错误              |
| 不亮           | 硬件问题            |

## 芯片规格

- **型号**: STM32F401RET6
- **Flash**: 512 KB
- **SRAM**: 96 KB
- **内核**: Cortex-M4F @ 84MHz

## 项目文件

- 链接脚本: `bootloader/ld/STM32F401RET6_BOOTLOADER.ld`
- 链接脚本: `application/ld/STM32F401RET6_APPLICATION.ld`
- 文档: `docs/CONFIG_COMPLETE.md`
- 验证: `verify_config.sh`, `verify_ld.sh`

## 故障排除

**编译失败**: `./build.sh clean && ./build.sh all`
**烧录失败**: 检查OpenOCD是否运行
**应用不启动**: 验证链接地址 `arm-none-eabi-objdump -h build/application.elf`

## 更多信息

- 完整文档: `README.md`
- 配置说明: `docs/STM32F401RET6_CONFIG.md`
- 架构详解: `docs/ARCHITECTURE.md`

