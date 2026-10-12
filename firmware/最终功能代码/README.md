# 最终功能代码（五功能定稿版）

本目录 18 个文件（9 个 `.c` + 9 个 `.h`）是**项目最终版**。

## 怎么用

按扩展名分别覆盖到对应目录（本目录是平铺的，别一股脑塞进 `Src/`）：

- 9 个 `.h` → 覆盖 `firmware/Core/Inc/`
- 9 个 `.c` → 覆盖 `firmware/Core/Src/`

覆盖后即为功能完整的最终工程。

## 与 Core/ 的差异

| 目录 | 阶段 | app.c |
| --- | --- | --- |
| `firmware/Core/` | 硬件自检阶段 | 72 行，仅走时 |
| `firmware/最终功能代码/` | **最终版** | 519 行，五功能 |

最终版实现：走时 / 秒表 / 倒计时 / 闹钟 / 时间调整 五模式 + 超级马里奥启动旋律 + 模式 LED 指示。

## 模式 LED 指示

| 界面 | LED |
| --- | --- |
| 主界面 | 4 个全亮 |
| 秒表 | LED1 红 |
| 倒计时 | LED2 绿 |
| 闹钟设置 | LED3 蓝 |
| 时间调整 | LED4 黄 |
| 音乐响起（闹钟 / 倒计时到点） | 4 个闪烁 |

## 注意

框架文件不在仓库中，编译前需用 CubeMX 打开 `firmware/clock.ioc` 重新生成：
`main.h`、`stm32f1xx_hal_conf.h`、`stm32f1xx_it.h`、`stm32f1xx_hal_msp.c`、
`syscalls.c`、`sysmem.c`、`system_stm32f1xx.c`、启动文件、链接脚本、`Drivers/`。

`stm32f1xx_it.c` 不在这 18 个文件中（它是框架文件），其中 SysTick 调用 `clock_time_isr_1ms`
的改动已在 `firmware/Core/` 中，覆盖时它保持原样即可。