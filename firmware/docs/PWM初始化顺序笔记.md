# PWM 初始化顺序笔记 —— 为什么 main.c 里 app_init() 必须在 hw_test_run() 前面

> 载体：STM32 数字钟工程，PB0 = TIM3_CH3 PWM 驱动无源蜂鸣器。
> 所有行号以 2026-10-11 工程现状为准。

## 背景知识：TIM3 是怎么驱动蜂鸣器的

PB0 引脚被 CubeMX 配置成了 **TIM3_CH3 的 PWM 输出引脚**（复用推挽模式）。这意味着：

- 你不能像控制 LED 那样 `HAL_GPIO_WritePin` 直接写高低电平
- PB0 的电平由 **TIM3 定时器硬件自动控制**，你只能改定时器的寄存器来间接控制

要让蜂鸣器响，需要**开关层 + 参数层**同时就位：

| 步骤 | 做了什么 | 谁来干 |
|------|------|------|
| ① 配寄存器 | TIM3 设成 PWM 模式，ARR=周期、CCR=占空比 | CubeMX `MX_TIM3_Init()`（main.c:156） |
| ② 开开关 | CEN（计数器跑）+ CC3E（通道通到引脚） | **全工程唯一一处**：`buzzer_init()` 里的 `HAL_TIM_PWM_Start()`（buzzer.c:54） |
| ③ 调参数 | 运行时改 ARR/CCR 变频率/占空 | `buzzer_tone()` → `buzzer_set_raw()` |

**① 和 ② 是不同的！** CubeMX 只做了 ①，② 是软件驱动手动做的。这是关键。

## 谁设参数、谁开开关（一张表看懂）

| 函数 | 设参数（ARR/CCR） | 开开关（CEN+CC3E） |
| -------------- | ---------------- | ------ |
| `app_init()` → `buzzer_init()` | ✅（设成 0，静音待命） | ✅ |
| `hw_test_run()` → `beep()` | ✅（设成发声值） | ❌ |

**app_init() 的 ✅ + ✅**（buzzer.c:49-56）：

```c
void buzzer_init(void)
{
    buzzer_set_raw(0u);                           /* :53 ✅ 设参数：CCR=0（静音） */
    (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); /* :54 ✅ 开开关：CEN+CC3E */
    s_playing = 0u;
}
```

**hw_test_run() 的 ✅ + ❌**（hw_test.c:77-83）：

```c
static void beep(uint32_t ms)
{
    buzzer_beep_ms(ms);       /* → buzzer_tone(2048, ms) → 只设 ARR/CCR */
    while (buzzer_playing()) {
        buzzer_task();        /* 只轮询超时，不开开关 */
    }
}
```

再看 `buzzer_set_raw()`（buzzer.c:29-47）：

```c
static void buzzer_set_raw(uint16_t freq)
{
    if (freq == 0u) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0u);  /* ✅ 只清 CCR，ARR 不动 */
        return;
    }
    /* ... */
    __HAL_TIM_SET_AUTORELOAD(&htim3, ...);   /* ✅ 设参数：ARR */
    __HAL_TIM_SET_COMPARE(&htim3, ...);      /* ✅ 设参数：CCR */
    /* ❌ 全函数没有 HAL_TIM_PWM_Start，不开开关 */
}
```

`HAL_TIM_PWM_Start` 在整个工程的用户代码里**只出现一次**，就在 `buzzer_init()` 第 54 行——搜一遍就知道上表"开开关"那一列为什么只有一个 ✅。

## 正确顺序：先 app_init()，再 hw_test_run()（main.c:98-99）

```
[1] MX_TIM3_Init()            ← 配好 PWM 模式寄存器，但计数器没转，PB0 没波形
[2] app_init() → buzzer_init()
      ├─ buzzer_set_raw(0)    ← CCR=0（油门归零待命）
      └─ HAL_TIM_PWM_Start()  ← ★ 点火：CEN+CC3E 置位，PB0 有 PWM 波形（占空 0，不响）
[3] hw_test_run() → test_buzzer()
      └─ beep(80) → buzzer_tone(2048, 80) → buzzer_set_raw(2048)
            ├─ ARR  = 8000000/2048 − 1 = 3905   ← 改周期
            └─ CCR  = 3906 × 50%      = 1953   ← 改占空 50%
      → 计数器已在转 → 新 ARR/CCR 立即被硬件消费 → 2048Hz 方波 → 响 ✅
```

点火之后改 ARR/CCR，下一个 PWM 周期硬件就自动用新值——这就是"点火一次、调油门无数次"。

## 调换顺序：先 hw_test_run()，再 app_init() —— 两段式静音

```
[1] MX_TIM3_Init()            ← 计数器没转
[2] hw_test_run() → test_buzzer()
      └─ buzzer_set_raw(2048)：ARR/CCR 都写对了
         但 CEN=0、CC3E=0 → 寄存器值没人消费 → 不响
         【原因一：点火前踩油门，白踩】
      → 80ms 后 buzzer_task() 到时静音，自检照常走完六项、串口零报错
        （自检是"安静地通过"，不是卡死——这比报错更难排查）
[3] app_init() → buzzer_init()
      ├─ buzzer_set_raw(0)    ← CCR 清 0（注意：只清 CCR，ARR 不动）
      └─ HAL_TIM_PWM_Start()  ← 点火了，但占空 0%
         【原因二：油门被拧回零】
      → 自检早跑完了，之后时钟模式也不发声
```

**两个原因独立生效**：只修原因二（init 不静音）自检照样静音；只修原因一（提前点火）app_init 一清 CCR 又哑了。所以正确顺序只有一种：**先点火（顺带油门归零待命）→ 再踩油门**。

## 一句话总结

**`HAL_TIM_PWM_Start()` = 发动机点火。**

车没点火时踩油门（改 ARR/CCR），发动机不会转；等你点火时，油门已经被 `buzzer_init()` 归零待命——自检那时早跑完了。

```
错误顺序: 踩油门 → 点火（油门归零） → 车不动
正确顺序: 点火 → 踩油门 → 车跑
```

## 附：通用判据

问一个问题：**这个模块的运行期函数，能不能离开 init() 单独干活？**

- `seg_display` / `led`：直推 GPIO，能 → 顺序宽容
- `buzzer`：运行期全靠 init 建立的开关层 → 顺序是硬约束

本质不是 TIM 特别麻烦，而是产出模型不同：

| 外设 | 产出模型 | 比方 |
|---|---|---|
| GPIO | 写一次变一次 | 手动开关 |
| UART | 发一帧走一帧 | 快递，一件一单 |
| TIM PWM | 启动后自主连续发电平 | 发电机：点火一次，调油门无数次 |

凡是"自主持续运行"的外设（ADC 连续转换、DAC、DMA 数据流），都有开关层/参数层分离——看到 HAL 里 `XXX_Start()` / `XXX_Stop()` 成对出现，基本就是这一族。
