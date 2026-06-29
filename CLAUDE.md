[根目录](../CLAUDE.md) > **ra2e1_boot**

# ra2e1_boot -- 模块文档

## 变更记录 (Changelog)

| 日期 | 变更内容 | 操作 |
|------|----------|------|
| 2026-06-26 | 完全对齐 Honcho_PR3 标准格式：变量重命名、SW版本、序列化区、移除 GAP | 重构 |
| 2026-06-22 | 增量扫描：更新版本号、Part Number、Personalization 到最新值 | 增量更新 |
| 2026-05-28 | 协议层完全重写为 Honcho_PR3 IBOX 格式 | 重构 |
| 2026-05-28 | 初始架构扫描与文档生成 (codegraph init) | 新建 |

---

## 模块职责

Renesas RA2E1 (R7FA2E1A92DFM, Cortex-M23) 平台的 ABS Bootloader 实现。负责：
- 通过 SCI-UART (UART0) 接收上位机命令
- 完成 MCU 内部 Code Flash 的读写和擦除操作（使用 Renesas FSP Flash LP 驱动）
- 实现 EEPROM 的 Flash 模拟（双Bank冗余切换）
- OTA 标志位检测与 CRC 校验，自动跳转至 APP
- 支持 XMODEM 协议批量固件下载
- 完成固件更新后跳转至 APP（位于 0x00004000）

---

## 入口与启动

### 启动流程

```
Reset_Handler (startup.c in FSP/BSP)
  -> SystemInit()
  -> R_BSP_WarmStart(BSP_WARM_START_RESET)  -- 启用 Data Flash 读取
  -> R_BSP_WarmStart(BSP_WARM_START_POST_C) -- 配置 Pin
  -> main() (ra_gen/main.c, 自动生成)
       -> hal_entry() (src/hal_entry.c)
            -> bl2_main() (src/bl2_main.c)
                 -> comms_open()            -- 打开 UART0 + 超时定时器
                 -> g_flash0.p_api->open()   -- 打开 Flash LP 驱动
                 -> menu()                   -- 命令处理主循环
                      -> 命令分发 (switch)
                      -> 超时时 try_boot()   -- 尝试跳转 APP
                 -> do_boot()               -- 跳转至 APP
```

### 主文件入口

| 文件 | 路径 | 职责 |
|------|------|------|
| 启动文件 | `ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/startup.c` | BSP启动代码（FSP SDK） |
| HAL入口 | `src/hal_entry.c` | R_BSP_WarmStart 配置，调用 bl2_main |
| 自动生成main | `ra_gen/main.c` | e2 studio自动生成，调用 hal_entry |
| Bootloader主程序 | `src/bl2_main.c` | 初始化通信/Flash，调用 menu，最终调用 do_boot |
| 命令处理 | `src/menu.c` | ABS协议命令分发与处理 |

### Flash 内存布局

```
RA2E1: R7FA2E1A92DFM (128 KB Code Flash / 16 KB RAM)

0x00000000  +---------------------+  <-- 向量表
            |     Bootloader      |
            |     (16 KB)         |  Bootloader 自身
0x00004000  +---------------------+  <-- APP_IMAGE_START_ADDRESS
            |     Application     |  APP 区域
            |     (112 KB)        |
0x0001FFFF  +---------------------+
0x00020000  +---------------------+  <-- APP_IMAGE_END_ADDRESS
            | OTA Flag (倒数4字节) |  0x0001FFFC
            | CRC (倒数8字节)     |  0x0001FFF8
            | EEPROM Bank0 (512B) |  0x0001FD00 - 0x0001FEFF
            | EEPROM Bank1 (512B) |  0x0001F800 - 0x0001F9FF
```

---

## 对外接口

### UART 协议命令处理 (`menu.c`)

协议行为已与 **Honcho_PR3 IBOX protocol** 完全对齐。所有命令由 `menu()` 函数接收后通过 `switch` 分发：

| 函数 (static) | 对应命令 | 功能 |
|---------------|----------|------|
| `cmd_handshake()` | `'B'` (HANDSHAKE) | 回显 `'B'`，触发 EEPROM 读取 |
| `cmd_identify()` | `'I'` (IDENTIFY) | **Honcho新增**: 发送 `'I' + 0x04 + 0x45 + ETX` (4 bytes) |
| `cmd_read_eeprom()` | `'F'` (READ_EEPROM) | **不读任何字节**: addr=0xFFFF 时发送 FEE 数据, 否则仅发 ETX |
| `cmd_load_eeprom()` | `'E'` (LOAD_EEPROM) | Bootloader特有: 先回显 `'E'`，再收数据，写入Flash Bank (Honcho 未实现) |
| `cmd_read_flash()` | `'N'` (READ_FLASH) | **不读任何字节**: addr=0xFFFF→版本信息, addr=0xFFFE→校准, 其他→回显 `'N'` |
| `cmd_load_flash()` | `'M'` (LOAD_FLASH) | Bootloader特有: 写入Flash页面，含2KB块自动擦除 (Honcho 未实现) |
| `cmd_set_address()` | `'D'` (SET_ADDRESS) | **Honcho格式**: 先回显 `'D'` → 收2字节地址 → 发 ETX |
| `cmd_change_baud()` | `'*'` (CHANGE_BAUD) | **Honcho格式**: 回显 `'*'` → 切高速波特率 → 收3字节 → 发版本信息 → 切回低速 |
| `try_boot()` | `0x01` (NACK) / 超时 | OTA标志检测 + CRC校验，决定是否跳转APP |

**关键协议差异 (vs 旧 CMS32 格式):**
- `SET_ADDRESS`: 回显在**读取之前** (CMS32 是读取之后再响应)
- `READ_FLASH`: **不读任何字节**，根据已存地址直接响应 (CMS32 是先读2字节长度)
- `READ_EEPROM`: **不读任何字节**，仅 addr=0xFFFF 时响应 (CMS32 总是响应)
- `CHANGE_BAUD`: 回显后读3字节再发版本信息 (CMS32 只回显+切波特率)
- 新增 `IDENTIFY` 命令 (`'I'`)

### OTA 跳转逻辑 (`try_boot`)

Bootloader 在以下情况会尝试跳转至 APP：
1. **超时无命令**：`recv_byte()` 返回超时时自动调用
2. **收到 NACK 命令**（`0x01`）

跳转条件判断：
- 若 **OTA Flag** (`0x0001FFFC`) == `0x55555555`：直接跳转（OTA刚完成）
- 若 **OTA Flag** == `0xFFFFFFFF` 且 **Flash CRC** == **存储的CRC**：跳转（有有效固件）
- 否则：返回菜单，等待命令

### XMODEM 批量下载 (`xmodem.c`)

提供 `XmodemDownloadAndProgramFlash()` 函数，支持通过 XMODEM 协议批量下载固件数据到 Flash。

| 函数 | 功能 |
|------|------|
| `XmodemDownloadAndProgramFlash(FlashAddress)` | XMODEM协议接收数据并写入Flash |

### 通信抽象层 (`comms/comms.h` + `comms/uart/comms.c`)

提供 UART 通信的统一抽象接口：

| 函数 | 功能 |
|------|------|
| `comms_open()` | 打开 UART0 SCI + 超时定时器 (GPT0) |
| `comms_send(p_src, len)` | 阻塞式发送，等待 TX Complete 事件 |
| `comms_read(p_dest, &len, timeout_ms)` | 带超时的阻塞接收，使用 GPT0 定时器 |
| `comms_set_baud(rate)` | 运行时修改 SCI-UART 波特率 |

---

## 关键依赖与配置

### 源码目录结构

```
ra2e1_boot/
|-- src/                     # 应用层源代码
|   |-- bl2_main.c           # Bootloader主程序（启动、do_boot跳转）
|   |-- hal_entry.c          # HAL入口点，R_BSP_WarmStart
|   |-- menu.c               # ABS协议命令分发与处理 (~711行)
|   |-- menu.h               # menu() 声明
|   |-- header.h             # 全局配置宏 (地址/命令/类型)
|   |-- crc16.c / crc16.h    # CRC16 CCITT 实现 (用于OTA校验)
|   |-- xmodem.c / xmodem.h  # XMODEM 批量下载协议
|   |-- comms/               # 通信抽象层
|       |-- comms.h          # 通信接口 (open/send/read/set_baud)
|       |-- uart/comms.c     # SCI-UART 通信实现 (~119行)
|-- ra_gen/                  # e2 studio 自动生成
|   |-- hal_data.c / hal_data.h   # HAL 实例 (g_flash0, g_uart0, g_timer0)
|   |-- main.c               # 自动生成的 main()
|   |-- vector_data.c        # 中断向量表
|   |-- common_data.c        # 通用数据
|   |-- pin_data.c           # Pin 配置
|   |-- bsp_clock_cfg.h      # 时钟配置
|-- ra_cfg/                  # FSP 配置
|   |-- fsp_cfg/bsp/board_cfg.h       # 板级配置
|   |-- fsp_cfg/r_flash_lp_cfg.h      # Flash LP 驱动配置
|   |-- fsp_cfg/r_sci_uart_cfg.h      # SCI UART 配置
|   |-- fsp_cfg/r_gpt_cfg.h           # GPT 定时器配置
|   |-- fsp_cfg/r_ioport_cfg.h        # IO 端口配置
|-- ra/                      # Renesas FSP SDK + CMSIS-6 (第三方库)
|   |-- arm/CMSIS_6/         # ARM CMSIS-6 (Cortex-M23 等)
|   |-- fsp/                 # Renesas FSP 5.9.0
|       |-- inc/api/         # FSP API 头文件
|       |-- inc/instances/   # 驱动实例头文件
|       |-- src/r_flash_lp/  # Flash LP 驱动实现
|       |-- src/r_sci_uart/  # SCI UART 驱动实现
|       |-- src/r_gpt/       # GPT 定时器驱动实现
|       |-- src/r_ioport/    # IO 端口驱动实现
|       |-- src/bsp/         # BSP (startup, system, clocks, IRQ等)
|-- script/fsp.ld            # FSP 链接脚本 (模板)
|-- Debug/memory_regions.ld  # 内存区域定义 (自动生成)
|-- build/compile_commands.json  # 编译数据库
|-- configuration.xml        # e2 studio 工程配置
```

### 硬件资源使用（由 FSP 配置）

| 外设 | 实例 | 用途 |
|------|------|------|
| SCI-UART | `g_uart0` | UART 通信 (Channel 0)，用于 ABS 协议 |
| Flash LP | `g_flash0` | Code Flash 编程/擦除 |
| GPT | `g_timer0` | 超时定时器 (用于 UART 读取超时) |
| IOPORT | `g_ioport_ctrl` | GPIO Pin 配置 |

### FSP HAL 实例接口

```c
g_flash0.p_api->open()    // 打开 Flash 驱动
g_flash0.p_api->write()   // 写入 Flash (自动处理编程序列)
g_flash0.p_api->erase()   // 擦除 Flash 块

g_uart0.p_api->open()     // 打开 UART
g_uart0.p_api->read()     // 异步读取
g_uart0.p_api->write()    // 异步写入
g_uart0.p_api->baudSet()  // 设置波特率

g_timer0.p_api->open()    // 打开定时器
g_timer0.p_api->start()   // 启动定时器
g_timer0.p_api->stop()    // 停止定时器
g_timer0.p_api->periodSet() // 设置周期
```

---

## 数据模型

### EEPROM Flash 模拟（双Bank冗余）

与 `abs_bootloader` 使用相同的双Bank策略，但存储位置不同：

- **Bank0**：`FMC_BANK0_START_ADRESS = APP_IMAGE_END_ADDRESS - 2048 - 512` (= `0x0001FD00`)
- **Bank1**：`FMC_BANK1_START_ADRESS = APP_IMAGE_END_ADDRESS - 2048` (= `0x0001F800`)
- 每个 Bank 占用 512 字节（`FEE_SIZE`）

Bank 状态判断逻辑与 `abs_bootloader` 完全一致。

与 `abs_bootloader` 不同之处：
- RA2E1 的 EEPROM 模拟位于 **Code Flash 最后 2KB 块内**
- 使用 `g_flash0.p_api->write()` API 而非直接操作 FMC 寄存器
- 写入前通过 `ThreadsAndInterrupts(DISABLE)` 禁用中断以保证原子性

### OTA 标志位与 CRC

| 地址 (相对APP_IMAGE_END_ADDRESS) | 绝对地址 | 内容 | 大小 |
|------|------|------|------|
| `-4` | `0x0001FFFC` | OTA Flag (0x55555555 = 有效OTA) | 4 字节 |
| `-8` | `0x0001FFF8` | CRC 校验值 | 2 字节 (实际为4字节，取低16位) |

OTA Flag 和 CRC 存储在 APP 区域的末尾，由上位机在固件写入完成后设置。

### 版本信息响应格式 (Honcho DL_Send_Read_FLASH_Msg, 88 bytes)

```
Offset | Size | Content
-------|------|--------
0      | 1    | 'N' (READ_FLASH_BYTE)
1      | 1    | 0x00 (len MSB)
2      | 1    | 84 (len LSB = 84 byte payload)
3-9    | 7    | Module ID        "N265280"     (g_module_id)
10-16  | 7    | Code Module ID   "0A83733"     (g_module_id_second)
17     | 1    | ' ' (空格分隔符)
18-25  | 8    | Part Number      "NA837330"    (g_tool_part_number)
26-61  | 36   | Personalization  "MSL2 2APR Hawkeye Laser M1 20VHE 123345 "
62-65  | 4    | Code Version     "v0.0"        (g_code_version)
66     | 1    | SW_VERSION_MAJOR (0x13)
67     | 1    | SW_VERSION_MINOR (0x32)
68     | 1    | SW_VERSION_MAJOR 重复 (0x13)
69     | 1    | SW_VERSION_MINOR 重复 (0x32)
70-73  | 4    | UCID[0..3]                     (→ DEVICEID[0])
74-77  | 4    | UCID[4..7]                     (→ DEVICEID[1])
78-81  | 4    | UCID[8..11]                    (→ DEVICEADDR[0])
82-85  | 4    | UCID[12..15]                   (→ DEVICEADDR[1])
86     | 1    | MODULE_REVISION (0x04)
87     | 1    | ETX (0x03)
```

### 全局状态变量 (`menu.c`)

| 变量 | 类型 | 初始值 | 说明 |
|------|------|--------|------|
| `g_target_addr` | `uint32_t` | `APP_IMAGE_START_ADDRESS` | 当前 Flash 读写目标地址 |
| `g_target_offset` | `uint32_t` | 0 | 当前地址偏移量 (用于版本/校准请求判断) |
| `g_addr_high` | `uint8_t` | 0 | SET_ADDRESS 收到的地址高字节 (Honcho协议) |
| `g_addr_low` | `uint8_t` | 0 | SET_ADDRESS 收到的地址低字节 (Honcho协议) |
| `g_FEE_Buffer` | `uint8_t[512]` | 0 | EEPROM 缓存 |
| `g_read_eeprom` | `uint8_t` | 0 | EEPROM读取触发标志 |
| `RX_TIMEOUT_MS` | `#define` | 245ms | UART接收超时时间 |

---

## 测试与质量

### 测试目录

无自动化单元测试目录。

### 运行时调试

- Bootloader 启动时通过 UART 发送状态信息：`"1-comms open ok"`, `"2-flash open ok"`, `"*** ABS Bootloader v2.1 ***"` 等
- `try_boot()` 函数通过 UART 输出调试信息（OTA Flag 值、Flash CRC、存储 CRC）
- 使用自定义 hex 打印函数（`put_hex32`, `put_hex16`, `put_hex8`）代替 `snprintf` 以减少代码体积

### 质量工具

| 工具 | 说明 |
|------|------|
| GCC ARM Embedded | 编译器警告 (`-Wall -Wextra`) |
| e2 studio | IDE 内置静态分析 |

---

## 常见问题 (FAQ)

**Q：协议格式与 CMS32 版本有何不同？**
A：RA2E1 协议已完全重写为 Honcho_PR3 IBOX 格式。关键区别: SET_ADDRESS 先回显再读字节; READ_FLASH 不读字节直接响应; READ_EEPROM 仅 addr=0xFFFF 时响应; 新增 IDENTIFY('I') 命令。

**Q：RA2E1 的 Flash 擦除块大小是多少？**
A：RA2E1 Code Flash 最小擦除单元（Block）为 **2KB** (`FLASH_BLOCK_SIZE`)。`erase_block_if_needed()` 在进入新 Block 时自动擦除。

**Q：为什么 Bootloader 使用 0x00000000 - 0x00003FFF？**
A：这段空间为 Bootloader 自身代码，由 FSP 链接脚本分配。APP 从 `0x00004000` 开始。

**Q：`ThreadsAndInterrupts()` 的作用是什么？**
A：在进行 Flash 写/擦除操作前，禁用 SysTick 和全局中断（保存 PRIMASK），防止 Flash 操作被打断导致编程失败。操作完成后恢复。

**Q：`do_boot()` 如何跳转到 APP？**
A：1) 读取 APP 起始地址的向量表（MSP 和 Reset_Handler）；2) 设置 `SCB->VTOR` 指向新的向量表；3) 设置 MSP；4) 跳转到 Reset_Handler。

**Q：XMODEM 和 ABS 协议的关系是什么？**
A：ABS 协议用于固件烧录前的握手和配置（设置地址、擦除等），XMODEM 用于大批量固件数据的传输。`xmodem.c` 中的 `XmodemDownloadAndProgramFlash()` 可被用于批量下载场景。

---

## 相关文件清单

### 应用层源文件（需维护）

| 文件 | 行数 | 职责 |
|------|------|------|
| `src/bl2_main.c` | ~132 | 初始化通信/Flash，调用menu/do_boot |
| `src/hal_entry.c` | ~106 | BSP Warm Start 配置 (Flash/Pin) |
| `src/menu.c` | ~711 | ABS协议所有命令处理 + EEPROM Bank管理 + OTA检测 |
| `src/menu.h` | ~16 | menu() 声明 |
| `src/header.h` | ~115 | 全局配置宏，类型定义，函数声明 |
| `src/crc16.c` | ~94 | CRC16 CCITT (calcrc, 用于OTA校验) |
| `src/crc16.h` | ~35 | calcrc() 声明 |
| `src/xmodem.c` | ~50+ | XMODEM 协议下载与Flash编程 |
| `src/xmodem.h` | -- | XMODEM 类型和返回值定义 |
| `src/comms/comms.h` | ~34 | 通信接口声明 |
| `src/comms/uart/comms.c` | ~119 | SCI-UART 通信实现 |

### e2 studio 自动生成文件（不应手动修改）

| 文件 | 说明 |
|------|------|
| `ra_gen/hal_data.c` / `.h` | HAL 实例定义 |
| `ra_gen/main.c` | 自动生成的 main() |
| `ra_gen/vector_data.c` / `.h` | 中断向量表 |
| `ra_gen/common_data.c` / `.h` | 通用数据 |
| `ra_gen/pin_data.c` | Pin 配置数据 |
| `ra_gen/bsp_clock_cfg.h` | 时钟配置 |
| `ra_cfg/fsp_cfg/*` | FSP 驱动配置 |
| `Debug/memory_regions.ld` | 内存区域定义 |

### FSP SDK/BSP 库文件（第三方，不应手动修改）

| 目录 | 说明 |
|------|------|
| `ra/arm/CMSIS_6/` | ARM CMSIS-6 标准库 |
| `ra/fsp/inc/` | FSP API 头文件 |
| `ra/fsp/src/r_flash_lp/` | Flash LP 驱动 |
| `ra/fsp/src/r_sci_uart/` | SCI UART 驱动 |
| `ra/fsp/src/r_gpt/` | GPT 定时器驱动 |
| `ra/fsp/src/r_ioport/` | IO 端口驱动 |
| `ra/fsp/src/bsp/` | BSP 启动和底层支持 |
