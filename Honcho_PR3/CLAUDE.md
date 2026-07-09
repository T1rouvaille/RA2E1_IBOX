[根目录](../CLAUDE.md) > **Honcho_PR3**

# Honcho_PR3 -- 模块文档

## 变更记录 (Changelog)

| 日期 | 变更内容 | 操作 |
|------|----------|------|
| 2026-06-22 | 增量初始化扫描与文档生成 (codegraph init -i) | 新建 |

---

## 模块职责

Honcho_PR3 是基于 **Zephyr RTOS** 的嵌入式应用程序，运行在 **nRF52840 (PAN1780模块)** 平台上。该模块是一个激光水平仪（Laser Line Unit, LLU）的固件，负责：

- 通过 BLE (Bluetooth Low Energy) 与上位机（手机 App/遥控器）进行无线通信和配对
- 控制三个激光器（Level/Plum1/Plum2）的 PWM 输出，支持脉冲调制和亮度调节
- 驱动垂直和偏航（Yaw）步进电机，实现激光线自调平
- 通过 I2C 读取加速度计（IIM42351）数据进行倾斜角度计算
- 通过 SPI 与 ATtiny 协处理器通信，处理按键输入
- 数据日志记录（Data logging），使用 NVS 将运行时数据写入内部 Flash
- 电池电压/温度监测和 SoC 指示
- 看门狗定时器保障系统可靠性

**项目名称**（CMake）：`Honcho_LLU`

---

## 入口与启动

### 启动流程

```
nRF52840 上电
  -> Early_init_LED() (board.c, SYS_INIT PRE_KERNEL_1) -- GPIO16/24/23 预置
  -> early_init() (main.c, SYS_INIT PRE_KERNEL_1) -- SOC引脚初始化
  -> main() (src/main.c)
       -> sys_init() (system_manager.c)
            -> control_init()    -- GPIO初始化 (所有使能/控制引脚)
            -> motor_init()      -- 电机PWM/方向引脚初始化
            -> laser_init()      -- 激光PWM/偏置引脚初始化
            -> led_init()        -- LED初始化
            -> led_off()         -- 关闭所有LED
            -> bucks_off()       -- 关闭所有Buck转换器
            -> enables_off()     -- 关闭ADC测量使能
            -> power POF配置    -- 低电压检测 (2.8V阈值)
            -> Bluetooth_initialize()   -- BLE协议栈初始化
            -> custom_adc_init()        -- ADC初始化
            -> freefall_HWinitconfig()  -- IIM42351加速度计初始化
            -> FLASHVAR_init_NVM()      -- Flash变量区初始化
            -> CALIB_init_NVM()         -- 校准数据区初始化
            -> DL_Init_NVM()            -- 数据日志NVS初始化
            -> DL_Comms_Init()          -- 数据日志UART通信初始化 (IBOX)
            -> Pendulumswitchint_init() -- Pendulum锁定开关中断初始化
            -> k_thread_start(KEYPAD_ATTINY_THREAD) -- 启动ATtiny按键线程
            -> task_delegator(0x0204)   -- 发送电源键模拟开机
       -> DL_Inc_U32(&DL_log.main_power_switch_in_cycles) -- 记录上电次数
       -> while(1) loop:
            -> wdt_feed(wdt, wdt_channel_id)  -- 喂狗
            -> k_sleep(K_SECONDS(5))          -- 5秒间隔
```

### 主文件入口

| 文件 | 路径 | 职责 |
|------|------|------|
| 主程序 | `src/main.c` | 应用入口，调用 sys_init，主循环喂狗 |
| 系统管理 | `src/drivers/system_manager.c` | sys_init/sys_on/sys_off 状态机，pendulum 中断处理，数据日志写线程 |
| 早期初始化 | `board.c` | PRE_KERNEL_1 阶段 GPIO 预置（防止上电瞬间 GPIO 状态不确定） |

### Flash 内存布局

```
nRF52840: 1MB Flash (0x00000000 - 0x000FFFFF)

0x00000000  +---------------------+  <-- mcuboot (Bootloader)
            |     MCUboot         |
            |     (48 KB)         |
0x0000C000  +---------------------+  <-- slot0_partition (image-0)
            |     Application     |
            |     (456 KB)        |
0x0007E000  +---------------------+
            |     Scratch         |
            |     (448 KB)        |
0x000EE000  +---------------------+  <-- datalog-partition (6KB)
            |     Data Log        |
0x000F4000  +---------------------+  <-- calib-partition (2KB)
            |     Calibration     |
0x000F6000  +---------------------+  <-- variable-partition (2KB)
            |     Flash Variables |
0x000F8000  +---------------------+  <-- storage-partition (32KB)
            |     NVS Storage     |
0x00100000  +---------------------+
```

---

## 对外接口

### BLE 通信接口 (`src/Bluetooth/Ble_central.c`)

Honcho LLU 作为 **BLE Central** 角色运行，扫描并连接远端设备：

| 函数 | 功能 |
|------|------|
| `Bluetooth_initialize()` | 初始化 BLE 协议栈、注册回调、配置扫描 |
| `BleComms_start()` | 开始扫描，60秒配对窗口 |
| `Stop_BleComm()` | 停止扫描，断开连接 |
| `start_scan()` | 启动活跃扫描，过滤 Manufacturer Data (SBD_ID1/SBD_ID2) |
| `ble_data_received()` | 接收远端 NUS 数据，解析按键命令，推入 FIFO |

#### BLE 协议栈结构

- **PHY**：仅使用 **Coded PHY (Long Range)**，明确禁用 1M PHY (`BT_LE_SCAN_OPT_CODED | BT_LE_SCAN_OPT_NO_1M`)
- **GATT Service**：NUS (Nordic UART Service) Client
- **配对**：基于 Manufacturer Data 中 Company ID (SBD_ID1/SBD_ID2) 和 PT_Index 匹配
- **安全**：L2 级别安全性 (`BT_SECURITY_L2`)，支持 Bonding
- **TX Power**：可动态设置 9 级 (-40dBm ~ +8dBm)

#### Manufacturer Data 格式 (12 bytes)

| 偏移 | 字段 | 说明 |
|------|------|------|
| 0-1 | Company ID | SBD_ID1, SBD_ID2 |
| 2 | PTI Seed | SBD_PTI_SEED |
| 3-8 | MAC Address | 6 字节 MAC |
| 9-10 | PT Index | SBD_PT_Index0, SBD_PT_Index1 |
| 11 | Status | bit7=配对请求标志 |

### UART 通信接口 (IBOX Bootloader)

UART1 用于与 Bootloader 通信（IBOX ABS 协议回读），配置：
- 波特率：38400
- 使用 `Uart_TX_28` (P0.28) / `Uart_RX_29` (P0.29)

通过 `src/data_logging/datalog_comms.c` 实现 UART 通信抽象：
- `DL_Comms_Init()` -- 初始化通信，50ms 内监听 Bootloader 轮询
- `IBOX()` -- 核心通信处理函数

### SPI 接口 (与 ATtiny 协处理器通信)

| 函数 | 功能 |
|------|------|
| `keypad_attiny()` | 按键处理线程，等待 ATtiny 中断后通过 SPI 读取按键数据 |
| `keypad_spi_handler()` | SPI 数据接收回调 |
| `task_delegator()` | 按键动作分发（电源键、亮度调节、电机控制等） |

---

## 关键依赖与配置

### 源码目录结构

```
Honcho_PR3/
|-- board.c                  # 早期 GPIO 初始化 (PRE_KERNEL_1)
|-- CMakeLists.txt           # Zephyr 构建系统
|-- prj.conf                 # Kconfig 应用配置
|-- sample.yaml              # Zephyr sample 元数据 (未自定义)
|-- README.rst               # Zephyr Hello World 模板 (未自定义)
|
|-- boards/arm/              # 板级支持
|   |-- honcho_pr3_llu_pan1780/   # PR3 变体板定义
|   |   |-- honcho_pr3_llu_pan1780.dts         # 设备树 (引脚映射)
|   |   |-- honcho_pr3_llu_pan1780-pinctrl.dtsi # Pin控制配置
|   |   |-- honcho_pr3_llu_pan1780_defconfig   # 默认Kconfig
|   |   |-- Kconfig.board / Kconfig.defconfig  # 板级Kconfig
|   |   |-- board.cmake                        # CMake板支持
|   |   |-- honcho_pr3_llu_pan1780.yaml        # 板元数据
|   |-- honcho_esl2_llu_pan1780/   # ESL2 变体板定义
|       |-- (同上结构)
|
|-- src/
|   |-- main.c               # 应用入口
|   |
|   |-- Bluetooth/           # BLE 模块
|   |   |-- Ble_central.c        # BLE Central 扫描/连接/配对 (~970行)
|   |   |-- include/Ble_central.h
|   |-- Bluetooth_Coded/     # Coded PHY BLE 变体
|   |   |-- Ble_central_coded.c
|   |   |-- include/Ble_central_coded.h
|   |
|   |-- battery/             # 电池管理
|   |   |-- battery.c / battery.h
|   |   |-- battery_batt.c   # 电池电压测量
|   |   |-- battery_th.c     # 电池温度测量
|   |
|   |-- button_decode/       # 按键解码
|   |   |-- button_decode.c
|   |   |-- include/button_decode.h
|   |
|   |-- calibration/         # 校准模块
|   |   |-- calibration.c
|   |   |-- include/calibration.h
|   |
|   |-- data_logging/        # 数据日志模块 (当前版本)
|   |   |-- datalog_comms.c      # UART通信 (IBOX回读)
|   |   |-- datalog_handler.c    # NVS日志存储
|   |   |-- include/datalog_comms.h
|   |   |-- include/datalog_handler.h
|   |   |-- include/personalization.h
|   |   |-- include/coding_std.h
|   |   |-- include/utils.h
|   |-- data_logging_old/    # 旧版数据日志 (保留参考)
|   |-- data_loging_1/       # 另一版数据日志 (保留参考)
|   |
|   |-- drivers/             # 外设驱动
|   |   |-- laser_control.c      # 激光 PWM 控制
|   |   |-- motor_control.c      # 步进电机 PWM 控制
|   |   |-- gpio_def.c           # GPIO 初始化/LED/Buck控制
|   |   |-- keypad_attiny.c      # ATtiny SPI 按键接口
|   |   |-- button_manager.c     # 按键管理
|   |   |-- acc_IIM42351.c       # IIM42351 加速度计 I2C 驱动
|   |   |-- custom_adc.c         # ADC 读取 (电池/激光电流等)
|   |   |-- watchdog_timer.c     # 看门狗定时器
|   |   |-- soc_laserblink.c     # SoC LED 闪烁
|   |   |-- system_manager.c     # 系统状态机 (~506行)
|   |   |-- include/             # 驱动头文件
|   |       |-- laser_control.h
|   |       |-- motor_control.h
|   |       |-- gpio_def.h
|   |       |-- keypad_attiny.h
|   |       |-- button_manager.h
|   |       |-- acc_IIM42351.h
|   |       |-- custom_adc.h
|   |       |-- watchdog_timer.h
|   |       |-- soc_laserblink.h
|   |       |-- system_manager.h
|   |
|   |-- motor_control/       # 电机控制
|   |   |-- motor_control.c
|   |   |-- include/motor_control.h
|   |
|   |-- peripherals/         # 外设抽象
|       |-- spi.c
|       |-- include/spi.h
|
|-- build_1/                 # 构建输出目录 1 (由 .gitignore 忽略)
|-- build_3/                 # 构建输出目录 3 (由 .gitignore 忽略)
|-- .vscode/                 # VS Code 配置
|-- Honcho_Macro_flowchart.pdf/vsdx  # 流程图文档
|-- Honch Macro flowcahrt.pdf
|-- Software_qualfication_document v*.docx/pdf  # 软件资质文档
|-- esl2-3 datalogging.xlsx        # 数据日志分析
|-- Power_ON_delay_quantification.JPG  # 上电延迟测试图片
```

### 硬件资源使用

| 外设 | 实例 | 引脚 | 用途 |
|------|------|------|------|
| GPIO | GPIO0, GPIO1 | 多个 | LED、使能信号、中断输入 |
| SPI1 | `spi1` | P0.7(CS) | 与 ATtiny 协处理器通信 |
| I2C0 | `i2c0` | -- | IIM42351 加速度计通信 |
| ADC | `adc` channel 0,4,5 | AIN0/AIN4/AIN5 | 电池电压/电流监测 |
| PWM0 | `pwm0` | -- | Plum1 激光 PWM (200kHz/8.5kHz) |
| PWM1 | `pwm1` | -- | Plum2 激光 PWM |
| PWM2 | `pwm2` | -- | Level 激光 PWM |
| PWM3 | `pwm3` ch0/ch1 | -- | 垂直/偏航步进电机 |
| UART1 | `uart1` | P0.28(TX)/P0.29(RX) | Bootloader IBOX 通信 (38400bps) |
| WDT | `wdt0` | -- | 看门狗定时器 |
| RTT | SEGGER RTT | -- | 调试日志输出 |

### Kconfig 关键配置 (prj.conf)

- **BLE**: Central + Peripheral + Coded PHY + NUS Client
- **连接数**: 最多 2 个并发连接，1 个绑定设备
- **扫描过滤**: 基于 Manufacturer Data (SBD_ID1/SBD_ID2)
- **安全性**: SMP + Bonding + Settings 持久化
- **存储**: NVS (Non-Volatile Storage) + Flash Map
- **ADC, I2C, SPI, PWM, GPIO, Watchdog**: 全部启用
- **RTT Console**: 使用 SEGGER RTT 代替 UART 控制台
- **主堆栈**: 4096 字节
- **Heap 内存池**: 4096 字节
- **FPU**: 启用硬件浮点

---

## 数据模型

### 系统状态机 (system_manager.c)

系统有三种主要状态，由 `sys_init()` / `sys_on()` / `sys_off()` 管理：

```
上电 --> sys_init() --> sys_on() --> [正常运行]
                                      |
                          (按键关机/低电压)
                                      |
                                      v
                                  sys_off()
                                      |
                            (切断LDO供电 -> 彻底断电)
```

#### 全局状态标志

| 变量 | 类型 | 说明 |
|------|------|------|
| `shutdown` | `bool` | 关机标志 |
| `msg_got_poll` | `bool` | Bootloader 轮询检测标志 |
| `msg_disable_ibox_comms` | `bool` | 50ms 后禁用 IBOX 通信 |
| `system_reboot_flag` | `bool` | 系统是否已完成首次启动 |
| `system_sleep_flag` | `volatile bool` | 休眠模式标志 |
| `pendulum_state` | `volatile uint8_t` | Pendulum 锁定状态 (0=锁定, 1=解锁) |
| `sleep_reboot_request` | `volatile uint8_t` | 0=默认, 2=重启, 4=休眠 |
| `memory_corrupt` | `volatile uint8_t` | NVM 损坏标志位 (bit0=calib, bit1=flashvar, bit2=datalog) |

### 激光控制 (`laser_control.c`)

三个独立激光通道，每个支持两种 PWM 频率：

| 激光 | PWM 驱动 | 偏置使能引脚 | 说明 |
|------|----------|-------------|------|
| Plum1 | `pwm0` | P1.3 (LaserP1bias) | 垂直激光线1 |
| Plum2 | `pwm1` | P0.19 (LaserP2bias) | 垂直激光线2 |
| Level | `pwm2` | P1.4 (LaserLbias) | 水平激光线 |

**PWM 调制策略**：
- 正常模式：200kHz @ 可调占空比 (1%/50%/65%)
- 探测器模式：每 500ms 切换至 8.5kHz @ 50% 占空比，持续 4ms（供激光探测器识别）

激光状态寄存器 `laser_status_reg`: `| Resvd | Resvd | Resvd | Resvd | Resvd | Level | Plum1 | Plum2 |`

### 电机控制 (`motor_control.c`)

| 电机 | PWM 驱动 | 方向引脚 | 使能引脚 |
|------|----------|----------|----------|
| 垂直 (Vertical) | `pwm3` ch0 | P1.10 (motordir) | P1.12 (motorverten) |
| 偏航 (Yaw) | `pwm3` ch1 | P1.10 (motordir, 共用) | P1.11 (motoryawen) |

- 支持单步和斜坡两种运动模式
- 带有反向间隙补偿（Backlash Compensation）
- 安全超时保护

### 数据日志 (Data Logging)

使用 Zephyr NVS (Non-Volatile Storage) 将运行时数据写入 Flash 的 `datalog-partition` (0xEE000, 6KB)。

**日志结构** (`DL_log`, 144 bytes):
包含总运行时间、各激光运行时间、电机运行时间、BLE 连接时间、上电循环计数、Pendulum 解锁次数、蓝牙配对次数等 36 个 uint32_t 字段。

**写入策略**：
- 每秒更新运行时计数器（`datalogtime_timer`）
- 每 300 秒触发一次 Flash 写入（`datalogwrite_timer`）
- 写入前通过 `k_sched_lock()` 禁止调度以保证原子性

### 校准数据 (`calibration.c`)

存储在 Flash `calib-partition` (0xF4000, 2KB)，使用 NVS 进行校准参数读写。

### Flash 变量 (`FLASHVAR`)

存储在 Flash `variable-partition` (0xF6000, 2KB)，用于持久化激光状态（如上次关机前的激光开关状态）。

---

## 测试与质量

### 测试目录

无自动化单元测试目录。

### 质量工具

| 工具 | 说明 |
|------|------|
| Zephyr 构建系统 | 编译器警告 (`-Wall -Wextra`) |
| SEGGER RTT | 调试日志输出，通过 `printk()` |
| Watchdog | 硬件看门狗 (WDT)，5 秒喂狗周期 |
| Assert | `CONFIG_ASSERT=y` 启用运行时断言 |
| NVM 完整性 | `memory_corrupt` 标志跟踪各存储分区初始化状态 |

### 硬件验证

- 上电延迟量化分析文档 (`Power_ON_delay_quantification.*`)
- 数据日志分析 Excel (`esl2-3 datalogging.xlsx`)
- 软件资质文档 (`Software_qualfication_document v*.docx/pdf`)

---

## 常见问题 (FAQ)

**Q：Honcho_PR3 与 abs_bootloader/ra2e1_boot 的关系是什么？**
A：Honcho_PR3 是激光水平仪的**主固件**。abs_bootloader 和 ra2e1_boot 是运行在**不同 MCU 平台上的 Bootloader**，通过 UART 与上位机通信完成固件更新。Honcho_PR3 在启动时会短暂监听 UART1 (50ms) 以响应 Bootloader 的 IBOX 通信轮询。

**Q：IBOX 通信是什么？**
A：IBOX 是 Honcho_PR3 中实现的数据日志 UART 回读协议。当 Bootloader 通过 UART 发送轮询命令时，Honcho_PR3 的 `datalog_comms.c` 模块会在 50ms 窗口内响应，允许 Bootloader 读取数据日志。

**Q：为什么有 `data_logging`、`data_logging_old`、`data_loging_1` 三个目录？**
A：`data_logging/` 是当前使用的版本。`data_logging_old/` 和 `data_loging_1/` 是旧版本的保留参考代码，未被 CMakeLists.txt 引用。

**Q：Coded PHY 与 1M PHY 有什么区别？**
A：Coded PHY (Long Range) 提供更远的通信距离（通过前向纠错编码），但速率较低。Honcho_PR3 明确配置为仅使用 Coded PHY，不使用 1M PHY，以最大化通信范围。

**Q：如何构建此项目？**
A：使用 Zephyr 构建系统（需要安装 nRF Connect SDK）：
```
west build -b honcho_pr3_llu_pan1780
```
或
```
west build -b honcho_esl2_llu_pan1780
```

---

## 相关文件清单

### 应用层源文件（需维护）

| 文件 | 行数 | 职责 |
|------|------|------|
| `src/main.c` | ~99 | 应用入口，主循环喂狗 |
| `src/drivers/system_manager.c` | ~506 | 系统初始化/运行/关机状态机，pendulum中断，数据日志写线程 |
| `src/Bluetooth/Ble_central.c` | ~970 | BLE Central 扫描/连接/配对/NUS |
| `src/Bluetooth_Coded/Ble_central_coded.c` | -- | Coded PHY BLE 变体 |
| `src/drivers/laser_control.c` | ~80+ | 激光 PWM 控制（双频调制） |
| `src/drivers/motor_control.c` | -- | 步进电机 PWM 控制 |
| `src/drivers/gpio_def.c` | ~193 | GPIO 初始化/LED/Buck管理 |
| `src/drivers/keypad_attiny.c` | -- | ATtiny SPI 按键接口 |
| `src/drivers/acc_IIM42351.c` | -- | IIM42351 加速度计驱动 |
| `src/drivers/custom_adc.c` | -- | ADC 读取（电池/激光电流） |
| `src/drivers/watchdog_timer.c` | ~86 | WDT 初始化/去初始化 |
| `src/drivers/button_manager.c` | -- | 按键管理 |
| `src/drivers/soc_laserblink.c` | -- | SoC LED 闪烁 |
| `src/motor_control/motor_control.c` | -- | 电机控制实现 |
| `src/data_logging/datalog_comms.c` | -- | IBOX UART 通信 |
| `src/data_logging/datalog_handler.c` | -- | NVS 日志存储 |
| `src/battery/battery.c` / `battery_batt.c` / `battery_th.c` | -- | 电池电压/温度管理 |
| `src/calibration/calibration.c` | -- | 校准数据 NVS 管理 |
| `src/peripherals/spi.c` | -- | SPI 外设抽象 |
| `src/button_decode/button_decode.c` | -- | 按键解码 |
| `board.c` | ~25 | 早期 GPIO 预置 |

### 板级配置文件

| 文件 | 说明 |
|------|------|
| `boards/arm/honcho_pr3_llu_pan1780/*.dts` | PR3 变体设备树 |
| `boards/arm/honcho_esl2_llu_pan1780/*.dts` | ESL2 变体设备树 |
| `prj.conf` | Kconfig 应用配置 |
| `CMakeLists.txt` | Zephyr CMake 构建定义 |

### 构建输出（由 .gitignore 忽略）

| 目录 | 说明 |
|------|------|
| `build_1/` | 构建输出目录 |
| `build_3/` | 构建输出目录 |

### 文档/参考资料

| 文件 | 说明 |
|------|------|
| `Honcho_Macro_flowchart.pdf` | 宏观流程图 |
| `Honcho_Macro_flowchart.vsdx` | 流程图 Visio 源文件 |
| `Honch Macro flowcahrt.pdf` | 流程图 |
| `Honcho_Flowchart_detailed.vsdx` | 详细流程图 |
| `Honcho MS3 EE Project Review.pptx` | 项目评审演示 |
| `Software_qualfication_document v*.docx/pdf` | 软件资质文档 (多个版本) |
| `esl2-3 datalogging.xlsx` | 数据日志分析 |
| `Power_ON_delay_quantification.*` | 上电延迟量化 |
