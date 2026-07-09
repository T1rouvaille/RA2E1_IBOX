[根目录](../CLAUDE.md) > **abs_bootloader**

# abs_bootloader -- 模块文档

## 变更记录 (Changelog)

| 日期 | 变更内容 | 操作 |
|------|----------|------|
| 2026-05-28 | 初始架构扫描与文档生成 (codegraph init) | 新建 |

---

## 模块职责

CMS32M5xxx (Cortex-M0) 平台的 ABS Bootloader 实现。负责：
- 通过 UART1 接收上位机命令
- 完成 MCU 内部 Flash 的读写和擦除操作
- 实现 EEPROM 的 Flash 模拟（双Bank冗余切换）
- 完成固件更新后跳转至 APP（位于 0x00000000）

---

## 入口与启动

### 启动流程

```
Reset_Handler (startup_cms32m5xxx.s)
  -> main() (2_User/main.c)
       -> BSP_MCU_Config()     -- 时钟、GPIO、UART、定时器初始化
       -> SelfOn_set()         -- 自锁电源保持
       -> while(!EXIT) 循环     -- 命令处理主循环
            -> getChar()       -- 从 UART1 读取命令字节
            -> switch(command) -- 命令分发
       -> 跳转至 APP           -- 设置 MSP，跳转到 APP Reset_Handler
```

### 主文件入口

| 文件 | 路径 | 职责 |
|------|------|------|
| 启动文件 | `1_Device/startup_cms32m5xxx.s` | 汇编启动代码，初始化栈，跳转到 main |
| 主程序 | `2_User/main.c` | Bootloader 主入口，命令分发循环，中断向量转发 |
| MCU配置 | `2_User/mcu_config.c` | 时钟配置 64MHz HSI，GPIO/UART/定时器外设初始化 |

### Flash 内存布局

```
0x00000000  +---------------------+  <-- APP Reset_Handler
            |     Application     |
            |     (28 KB)         |
0x00007000  +---------------------+  <-- Bootloader 起始
            |     Bootloader      |
            |     (4 KB)          |
0x00007FFF  +---------------------+  <-- Bootloader 结束
```

---

## 对外接口

### UART 协议命令处理 (`cm_handler.c`)

所有 ABS Bootloader 命令由 `main.c` 的主循环接收后通过 `switch` 分发到 `cm_handler.c` 中的处理函数：

| 函数 | 对应命令 | 功能 |
|------|----------|------|
| `cmd_handshake()` (内联在 main.c) | `'B'` (HANDSHAKE) | 回复 `'B'`，触发 EEPROM 读取 |
| `Cmd_Read_EEPROM()` | `'F'` (READ_EEPROM) | 发送 EEPROM 全部数据 (512字节 + 长度头 + ETX) |
| `Cmd_Load_EEPROM()` | `'E'` (LOAD_EEPROM) | 接收 EEPROM 数据，校验后写入 Flash Bank |
| `Read_Flash_Page()` | `'N'` (READ_FLASH) | 从指定地址读取一页(512字节)Flash 数据 |
| `Load_Flash_Page()` | `'M'` (LOAD_FLASH) | 写入一页(512字节)数据到 Flash，含擦除+回读校验 |
| `Set_Address()` | `'D'` (SET_ADDRESS) | 设置目标地址（16位偏移量） |
| BAUDRATE切换 (内联在 main.c) | `'*'` (CHANGE_BAUD) | 切换到高速波特率 (28800) |
| NACK处理 (内联在 main.c) | `0x01` (NACK) | 设置 `EXIT=1`，退出主循环并跳转 APP |

### 版本信息读取 (`Read_Version_Info`)

当 `Read_Flash_Page` 收到地址 0xFFFF 时，返回模块信息：
- Module ID（8字节，存储在 `0x0DF0` 地址）
- Part Number + Personalization（56字节，存储在 `0x0E00`）
- Code Version（2字节，存储在 `0x0E3C`）
- Code Verification（2字节，存储在 `0x7FFC`）
- Chip ID（16字节，存储在 `0x18000000`）
- 模块版本号（1字节，存储在 `0x0DF8`）

### 中断向量重映射

Bootloader 不使用任何外设中断。所有中断处理函数（NMI、HardFault、SVC、PendSV、SysTick 及各外设）都被重定向到 APP 区域的对应向量：

```c
void HardFault_Handler(void) {
    ((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE + 0x0C)))();
}
```

APP 基址 `FMC_APROM_BASE = 0x1000`（即 `0x00001000`，Flash 内存映射起始）。

---

## 关键依赖与配置

### 源码目录结构

```
abs_bootloader/
|-- 1_Device/               # CMS SDK 移植层
|   |-- cms32m5xxx.h        # MCU 寄存器定义
|   |-- core_cm0.h          # Cortex-M0 CMSIS
|   |-- startup_cms32m5xxx.s # 汇编启动文件
|   |-- system_cms32m5xxx.c  # SystemInit
|-- 2_User/                 # 应用层
|   |-- main.c              # 主入口 + 命令分发 + 中断向量转发
|   |-- main.h              # 命令定义宏 (COMMAND_*), ETX, Tx_ON/OFF
|   |-- cm_handler.c        # 命令处理函数实现
|   |-- cm_handler.h        # uReg32 联合体, 函数声明
|   |-- code_config.h       # 模块标识/配置宏 (ABS_BOOTLOADER_ID, 内存地址)
|   |-- mcu_config.c        # BSP 初始化 (时钟/GPIO/UART/定时器)
|   |-- mcu_config.h        # BSP 宏 (LED, SelfOn, 波特率, 超时)
|   |-- user_fmc.c          # Flash 操作 (读取/写入/擦除)
|   |-- user_fmc.h          # FMC Bank 地址宏, FMC 操作宏
|-- 3_driver/               # CMS SDK 驱动头文件
|   |-- uart.h              # UART 配置宏和函数声明
|   |-- gpio.h              # GPIO 配置宏
|   |-- system.h            # 系统时钟/电源/IO配置函数
|   |-- timer.h             # 定时器配置
|-- lint/                   # PC-Lint 静态分析配置
|   |-- LIN.BAT             # Lint 启动脚本
|   |-- co-gcc.lnt, std.lnt, au-sbdk.lnt 等
|-- Objects/                # 编译输出 (.hex, .axf, .sct)
|-- ABS_Bootloader.uvprojx  # Keil MDK 工程文件
```

### 编译目标（Keil MDK 工程配置）

项目有两个编译目标（Targets）：
1. **ABS_Bootloader_SingleWire** -- 单线 UART 通信模式（RX/TX 共用一根线）
2. **ABS_Bootloader_TwoWires** -- 双线 UART 通信模式（RX/TX 各一根线）

通过 `BL_HW_MODE` 宏控制：
- `0x53 ('S')` -- 单线模式（默认）
- `0x44 ('D')` -- 双线模式

### 编译优化

- 必须使用 `-Os`（优化体积）以适配仅 4KB 的 Bootloader 空间

---

## 数据模型

### EEPROM Flash 模拟（双Bank冗余）

EEPROM 通过 Flash 模拟实现，使用 **双Bank + 计数器** 机制保证掉电安全：

- **Bank0**：`FMC_BANK0_START_ADRESS = 0x1C000000`（128字）
- **Bank1**：`FMC_BANK1_START_ADRESS = 0x1C000200`（128字）
- 每个 Bank 的前3字节存储写入计数器，末4字节存储结束计数器
- 读取时选择 Valid 且较新的 Bank
- 写入时选择 Empty/Invalid 的 Bank，或轮换至另一个 Bank

**Bank状态判断 (`Get_Bank_Final_State`)：**
- 如果 `start_count == end_count`：
  - 均为 `0xFFFFFF` -> `BANK_EMPTY`
  - 否则 -> `BANK_VALID`
- 如果 `start_count != end_count` -> `BANK_INVALID`
- 两个 Bank 均 Valid 时，start_count 较大的为 `BANK_NEW`

**保护区域：**
- 首 16 字节（`PROTECTED_PAGE_SIZE_0`）：不允许通过 LOAD_EEPROM 写入
- 末 16 字节（`PROTECTED_PAGE_SIZE_1`）：不允许通过 LOAD_EEPROM 写入

### 内存固定地址常量

| 地址 | 内容 | 大小 |
|------|------|------|
| `0x0DF0` | Module_ID (ABS_BOOTLOADER_ID) | 8 字节 |
| `0x0DF8` | Revision | 1 字节 |
| `0x0DFC` | BL_HW_Mode (通信模式) | 1 字节 |
| `0x0E00` | Flash Constants (Part Number + Personalization) | 56 字节 |
| `0x0E3C` | Version (Code Version) | 2 字节 |
| `0x7FFC` | Verify (Code Verification) | 2 字节 |
| `0x7FFF` | RAM Stack Pointer | 4 字节 |

---

## 测试与质量

### 测试目录

无自动化单元测试目录。

### 质量工具

| 工具 | 配置路径 | 说明 |
|------|----------|------|
| PC-Lint | `lint/LIN.BAT` | 静态分析，遵循 MISRA-C 2004 规则 |
| Keil MDK | `ABS_Bootloader.uvprojx` | 编译器内置警告检查 |

### 代码中的自检逻辑

- `Load_Flash_Page` 在写入后执行 `Read_Flash_Page` 回读并与 RAM 中的校验和进行比较（最多重试直至成功）
- `main()` 中有一个 Flash 写入测试循环（`while(1)` 位于 `while(!EXIT)` 之前），用于逐字节测试Flash

---

## 常见问题 (FAQ)

**Q：为什么代码中有一个在 `while(!EXIT)` 之前的 `while(1)` 循环？**
A：这是一个 Flash 写入连续测试逻辑（用 `0x00-0xFF` 填充 512 字节写入 `0x1000` 地址并回读校验），属于调试/验证代码。生产环境中应注意是否启用。

**Q：单线模式和双线模式的区别是什么？**
A：单线模式下，UART RX/TX 共用 P3.6 引脚，在发送前需要切换 GPIO 配置。双线模式下，TX 使用 P3.5，RX 使用 P3.6，无需切换。

**Q：Bootloader 如何跳转到 APP？**
A：1) 调用 `ResetReg()` 复位所有外设到默认状态；2) 设置 MSP 为 `*(uint32_t *)FMC_APROM_BASE`；3) 跳转到 `*(uint32_t *)(FMC_APROM_BASE + 4)` 即 APP 的 Reset_Handler。

---

## 相关文件清单

### 应用层源文件（需维护）

| 文件 | 行数 | 职责 |
|------|------|------|
| `2_User/main.c` | ~365 | 主循环、命令分发、中断向量转发 |
| `2_User/main.h` | ~44 | 命令定义、TX开关宏 |
| `2_User/cm_handler.c` | ~515 | Flash/EEPROM读写命令处理、地址设置、版本信息 |
| `2_User/cm_handler.h` | ~52 | uReg32联合体、函数声明 |
| `2_User/code_config.h` | ~87 | 模块标识、Flash地址布局、页面大小 |
| `2_User/mcu_config.c` | ~254 | BSP初始化、UART收发、延时函数 |
| `2_User/mcu_config.h` | ~77 | 外设宏、波特率、定时器超时 |
| `2_User/user_fmc.c` | ~216 | Flash控制器底层操作(读/写/擦除) |
| `2_User/user_fmc.h` | ~71 | FMC Bank地址、操作宏 |

### SDK/BSP文件（不应手动修改）

| 文件 | 来源 |
|------|------|
| `1_Device/cms32m5xxx.h` | 中微半导体 CMS SDK |
| `1_Device/core_cm0.h` | ARM CMSIS |
| `1_Device/startup_cms32m5xxx.s` | 中微半导体 CMS SDK |
| `1_Device/system_cms32m5xxx.c` | 中微半导体 CMS SDK |
| `3_driver/uart.h` | 中微半导体 CMS SDK |
| `3_driver/system.h` | 中微半导体 CMS SDK |
| `3_driver/gpio.h` | 中微半导体 CMS SDK |
| `3_driver/timer.h` | 中微半导体 CMS SDK |
