# STM32 数字钟

> ⚠️ 本目录（`firmware/`）是**焊接后硬件自检程序**，用于焊接完成后逐项验证"焊得好不好、功能对不对"。
> 它**不是**数字钟的最终功能实现（走时 / 按键设置等业务逻辑见文末 TODO）。
> 本目录只放"人写的部分"：`Drivers/`、启动文件、链接脚本等由 `clock.ioc` 用 CubeMX 重新生成，故不入库——下方"项目结构"为重新生成后的完整工程形态。

基于 STM32F103xx 的 4 位数码管电子钟，支持按键交互、蜂鸣器报时、LED 指示、串口调试。

## 硬件平台

| 组件 | 型号/说明 |
|---|---|
| MCU | STM32F103C8T6（或其他 STM32F103xB 系列） |
| 显示 | 4 位共阳数码管（段选 8 + 位选 4，动态扫描） |
| 按键 | 4 路独立按键（板上 10k 上拉，低电平有效） |
| LED | 4 路状态指示灯（灌电流驱动，低电平点亮） |
| 蜂鸣器 | 无源电磁式（S8050 低边 NPN 驱动，TIM3_CH3 PWM 方波发声） |
| 调试口 | USART1（PA9=TX / PA10=RX，115200-8N1） |

## 引脚映射

> 引脚经原理图截图、PCB 网表、.ioc 文件三重交叉验证，以 `Core/Inc/pinmap.h` 为唯一事实来源。

### 数码管段选（共阳：段脚拉低 = 点亮）

| 段 | 引脚 | 排针 |
|---|---|---|
| SEG_A | PB9 | 17 |
| SEG_B | PB8 | 16 |
| SEG_C | PB7 | 15 |
| SEG_D | PB6 | 14 |
| SEG_E | PB5 | 13 |
| SEG_F | PB4 | 12 |
| SEG_G | PB3 | 11 |
| SEG_DP | PA15 | 10 |

### 数码管位选（S8550 PNP 高边：拉低 = 选通）

| 位 | 引脚 | 排针 | 位置 |
|---|---|---|---|
| DIG1 | PB12 | 1 | 千位（最左） |
| DIG2 | PB13 | 2 | 百位 |
| DIG3 | PB14 | 3 | 十位 |
| DIG4 | PB15 | 4 | 个位（最右） |

### 按键、LED、蜂鸣器、串口

| 模块 | 引脚 | 排针 | 极性 |
|---|---|---|---|
| KEY1 | PA8 | 5 | 低 = 按下 |
| KEY2 | PA11 | 8 | 低 = 按下 |
| KEY3 | PA12 | 9 | 低 = 按下 |
| KEY4 | PB11 | 36 | 低 = 按下 |
| LED1 | PA4 | 29 | 低 = 亮 |
| LED2 | PA5 | 30 | 低 = 亮 |
| LED3 | PA6 | 31 | 低 = 亮 |
| LED4 | PA7 | 32 | 低 = 亮 |
| 蜂鸣器 | PB0 | 33 | TIM3_CH3 PWM（50% 方波 = 响 / 0% 占空 = 停） |
| USART1 TX | PA9 | — | 115200-8N1 |
| USART1 RX | PA10 | — | 115200-8N1 |

> **JTAG 注意**：PB3(JTDO)、PB4(NJTRST)、PA15(JTDI) 都是 JTAG 引脚且本设计全部占用。`clock.ioc` 当前 SYS→Debug 已设为 Serial Wire（SWD 只占 PA13/PA14，释放了全部 JTAG 引脚）。**切勿改回 Full JTAG**，否则段选/位选会失灵。

## 项目结构

```
clock/
├── clock.ioc                  # STM32CubeMX 项目文件
├── CMakeLists.txt             # 顶层 CMake 构建脚本
├── CMakePresets.json
├── STM32F103xx_FLASH.ld       # 链接脚本
├── startup_stm32f103xb.s      # 启动汇编
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32f1xx_hal_conf.h
│   │   ├── stm32f1xx_it.h
│   │   ├── pinmap.h           # 引脚映射（唯一事实来源）
│   │   ├── app.h              # 应用层装配
│   │   ├── seg_display.h      # 数码管动态扫描
│   │   ├── key.h              # 按键消抖 / 短按长按
│   │   ├── led.h              # LED 控制
│   │   ├── buzzer.h           # 蜂鸣器
│   │   ├── uart_console.h     # 串口 + 轻量 printf
│   │   ├── clock_time.h       # SysTick 软件走时
│   │   └── hw_test.h          # 上电硬件自检
│   └── Src/
│       ├── main.c             # 主程序入口
│       ├── stm32f1xx_it.c     # 中断处理
│       ├── stm32f1xx_hal_msp.c
│       ├── system_stm32f1xx.c
│       ├── syscalls.c
│       ├── sysmem.c
│       ├── app.c              # 应用层装配 + 主逻辑
│       ├── seg_display.c      # 数码管动态扫描
│       ├── key.c              # 按键消抖 / 短按长按
│       ├── led.c              # LED 控制
│       ├── buzzer.c           # 蜂鸣器
│       ├── uart_console.c     # 串口 + 轻量 printf
│       ├── clock_time.c       # SysTick 软件走时
│       └── hw_test.c          # 上电硬件自检
├── Drivers/                   # HAL 库 + CMSIS（CubeMX 生成，勿手动修改）
│   ├── STM32F1xx_HAL_Driver/
│   └── CMSIS/
└── cmake/                     # CMake 子模块
    ├── stm32cubemx/
    ├── gcc-arm-none-eabi.cmake
    └── starm-clang.cmake
```

## 快速开始：集成应用模块

以下步骤记录了应用层模块从 CubeMX 骨架集成进 `Core/` 的全过程（**本工程已完成全部 8 个模块集成并通过编译**）。复现工程或新增模块时按此顺序操作：

> **前置条件**：`clock.ioc` 中已启用 USART1（Connectivity → USART1 → Mode = Asynchronous），`HAL_UART_MODULE_ENABLED` 宏已开启。若重新 Generate Code 后编译报 UART 未定义，检查此项。

### 第 1 步：添加源文件

将以下文件放入 `Core/` 对应目录：

| 模块 | 头文件 → `Core/Inc/` | 源文件 → `Core/Src/` |
|---|---|---|
| pinmap | `pinmap.h` | —（纯头文件） |
| app | `app.h` | `app.c` |
| seg_display | `seg_display.h` | `seg_display.c` |
| key | `key.h` | `key.c` |
| led | `led.h` | `led.c` |
| buzzer | `buzzer.h` | `buzzer.c` |
| uart_console | `uart_console.h` | `uart_console.c` |
| clock_time | `clock_time.h` | `clock_time.c` |
| hw_test | `hw_test.h` | `hw_test.c` |

### 第 2 步：修改 CMakeLists.txt

在项目根目录的 `CMakeLists.txt` 中，替换已有的空 `target_sources` 段：

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/app.c
    Core/Src/seg_display.c
    Core/Src/key.c
    Core/Src/led.c
    Core/Src/buzzer.c
    Core/Src/uart_console.c
    Core/Src/clock_time.c
    Core/Src/hw_test.c
)
```

> include 路径无需额外添加：`Core/Inc` 已在 `stm32cubemx` 子目标的 INTERFACE include 中，链接时自动继承。

### 第 3 步：打 SysTick 补丁

修改 `Core/Src/stm32f1xx_it.c`，为软件走时提供 1ms 时基：

在文件顶部 `USER CODE BEGIN Includes` 处添加：
```c
#include "clock_time.h"
```

在 `SysTick_Handler` 函数的 `USER CODE BEGIN SysTick_IRQn 0` 处添加：
```c
clock_time_isr_1ms();
```

> 这两处都在 USER CODE 段内，CubeMX 重新生成代码不会覆盖。`clock_time_isr_1ms()` 只有计数与进位操作，与 `HAL_IncTick()` 互不依赖，先后顺序均可；真正的走时精度上限来自 HSI 晶振（±1%），后续可换 RTC + LSE 解决。

### 第 4 步：修改 main.c

在 `Core/Src/main.c` 的 USER CODE 区域添加以下内容：

**文件顶部 Includes：**
```c
/* USER CODE BEGIN Includes */
#include "app.h"
#include "hw_test.h"
/* USER CODE END Includes */
```

**初始化（USER CODE BEGIN 2）：**
```c
  /* USER CODE BEGIN 2 */
  app_init();
  hw_test_run();
  /* USER CODE END 2 */
```

**主循环（USER CODE BEGIN 3）：**
```c
    /* USER CODE BEGIN 3 */
    app_task();
  /* USER CODE END 3 */
```

### 上电默认行为

集成完成后，上电先运行约 10 秒硬件自检（LED 流水 → 数码管全亮 → 逐位扫描 → 逐段扫描 → 蜂鸣器三短一长 → 按键逐一测试），自检通过后进入时钟模式：

- 数码管显示 HH:MM，冒号每秒闪烁（软件从 12:00:00 开始走时）
- 串口 115200 打印启动横幅与每秒时间
- 任意按键短按/长按：蜂鸣 30ms + 串口回显键值
- LED1 每秒翻转（心跳指示）

## 构建

本工程使用 CMake + arm-none-eabi-gcc 工具链：

```bash
# 配置（在项目根目录）
cmake --preset Debug

# 编译
cmake --build build/Debug
```

或使用 VS Code / CLion 打开项目目录，配合 CMakePresets.json 自动配置。

## 已知问题

| 问题 | 影响 | 修复 |
|---|---|---|
| 蜂鸣器接 3.3V 音量偏小 | 板载 5V 有源蜂鸣器供电不足 | 已换无源蜂鸣器 + TIM3 PWM 驱动 |

## 应用逻辑 TODO

`app.c` 及各模块中标注了 `TODO(应用)` 的功能待实现：

- [ ] 设置模式（按键调时、字段闪烁编辑）
- [ ] 闹钟功能（设定 + 触发）
- [ ] 整点报时
- [ ] 按键功能分配（当前仅占位回显）
- [ ] 串口命令解析（调时、查询等）
- [ ] LED 指示策略（当前 LED1 仅心跳）

## 延伸阅读

- [从零搭建 HAL 工程（CubeMX → 移植 → 跑通）](https://github.com/syj766/personal-workflow/blob/main/workflows/02-board-software/01-cubemx-hal-scaffold.md)：本工程从零复刻的完整流程（图形化配置 → 移植 → 拼 USER CODE 段 → 编译烧录）。
- [PWM 初始化顺序笔记](docs/PWM初始化顺序笔记.md)：为什么 `main.c` 里 `app_init()` 必须在 `hw_test_run()` 之前。