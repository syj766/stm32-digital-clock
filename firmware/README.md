# firmware · 焊接后硬件自检程序（bring-up self-test）

> ⚠️ 本目录是**上电自检程序**，用于焊接完成后逐项验证"焊得好不好、功能对不对"。
> 它**不是**数字钟的最终功能实现（走时 / 按键设置等业务逻辑待实现）。

## 它做什么

上板后按顺序自检，对应主 README §一.3「上电验收单」：

```
LED 流水×2 → 8.8.8.8. 全亮 → 逐位×2 → 逐段 → 蜂鸣(哔哔哔——哔) → Do Mi So Do↑
→ K1~K4（LED + 键号 + 响）→ 双响 → 12:00 走时、冒号闪、LED1 心跳、串口每秒报时
```

**健康判据**：跑一遍，某环节有响应 = 该环节焊接 / 接线正常；全无反应 = 查该环节电源与接线。

## 目录（只放"你写的 + 你改过的"）

```
firmware/
├── clock.ioc            # CubeMX 配置源头（引脚分配 / 时钟树 / Debug = Serial Wire）
├── CMakeLists.txt       # 构建入口（target_sources 已含 8 个自定义 .c）
└── Core/
    ├── Inc/             # 你写的 9 个头文件（app/hw_test/clock_time/uart_console/
    │                    #   buzzer/led/key/seg_display/pinmap）
    └── Src/             # 你写的 8 个 .c + 改动过的 main.c / stm32f1xx_it.c
```

## 为什么没有 Drivers/、启动文件、链接脚本？

这些是 **CubeMX 从 `clock.ioc` 自动生成的样板**（含 ST HAL 库），体积大且可完整重建，故不入库。
本目录刻意只保留"人写的部分"，复现时按下节重新生成样板即可。

## 复现 / 编译步骤

1. 用 STM32CubeMX 打开 `firmware/clock.ioc`
2. 确认 `SYS → Debug = Serial Wire`（释放 PB3 / PA15 / PB4 作段选，同时保留 SWD 烧录）
3. 生成 HAL 工程（Toolchain 选 **CMake**）
4. 用本目录覆盖生成工程的对应文件：`Core/Inc/`、`Core/Src/`、`CMakeLists.txt`
5. 构建：`cmake --preset Debug` → `cmake --build build/Debug`
6. 烧录 `clock.elf`，按上面的自检序列对照验收

> 逐条改动清单见主 README §一.2「操作清单」；三条铁律与易错点见 §一.1 / §一.4。
