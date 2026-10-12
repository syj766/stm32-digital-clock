# RTC 掉电续走（时间持久化）

> 目标：断电 / 复位后再上电，数字钟接着上次的时间走，而不是回到零点。
> 涉及两个文件：`main.c`（RTC 初始化期的"抢救"与"回写"）、`clock_time.c`（上电读表 / 设表写回）。

## 一、为什么要"抢救"

CubeMX 生成的 `MX_RTC_Init()` 里有一段**无条件**的初始化：

```c
sTime.Hours = 0; sTime.Minutes = 0; sTime.Seconds = 0;
HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD);   /* 每次上电都清成 00:00:00 */
```

也就是说，**每次上电它都会把 RTC 计数器清零**。要用 RTC 续走，就必须在这段**之前**把当前时间读出来，之后再写回去。

## 二、实现（三处，全在 USER CODE 段，重新生成不丢）

```text
main.c ── MX_RTC_Init()
  ├─ Check_RTC_BKUP 段：先 GetTime/GetDate 抢救到 sSavedTime，并读 BKP->DR1 判断是否"已初始化过"
  ├─ CubeMX 生成区：  SetTime(00:00:00)   ← 不加干涉，任由它清
  └─ RTC_Init 2 段：  有效 → 写回抢救值（抵消清零）；首次 → BKP->DR1 = 0x5948 烙标记

clock_time.c
  ├─ init()：上电读 RTC 进 s_secs（读失败兜底 12:00:00）
  └─ set() ：写完 s_secs 后同步 HAL_RTC_SetTime / SetDate 写回 RTC
```

- **标记**：`BKP->DR1 == 0x5948`（备份域寄存器，由 VBAT 供电、掉电保持）用作"RTC 已初始化过"的依据。
- **顺序**：抢救代码必须放在 `HAL_RTC_Init()` **之后**（此时备份域已由 `HAL_RTC_MspInit` 解锁），且在生成器 `SetTime` **之前**。
- **F1 细节**：读时间必须 `HAL_RTC_GetTime()` 在 `HAL_RTC_GetDate()` **之前**（否则影子寄存器不更新）。

## 三、CubeMX 配置（RCC / RTC）

```text
RCC：High Speed Clock (HSE) = Disable
     Low Speed Clock (LSE)  = Disable
RTC：Activate Clock Source ✅
     Activate Calendar     ✅
     RTC OUT = Disable、Tamper ☐
```

## 四、注意事项（重要）

1. **RTC 时钟源是 LSI（内部 RC），不是晶振。**
   本板未装 32.768kHz 晶振，LSE 不可用，故 CubeMX 里 RTC 走 **LSI**。
   → 掉电续走**成立**，但 LSI 是 F103 里精度最差的时钟源，**长期会漂**。
2. **首次上电显示 00:00:00，不是 12:00。**
   `clock_time.c` 的 12:00 兜底只在 **RTC 读取失败**时触发；首次上电 RTC 被生成器设为 0 且**读取成功**，所以显示 00:00:00。设表之后，断电再上电才会接着设定值走。
3. **运行中的走时基准是 HSI 8MHz（同样是 RC）。**
   软件计数靠 SysTick，主频未用 PLL / 晶振，精度同样受 RC 限制；且 HSI 与 LSI 频率不同步，长时间后软件计数与 RTC 会缓慢分叉——属无晶振方案的固有取舍，非缺陷。

## 五、验证

1. 上电 → 显示应为 **00:00:00**（首次）并在走时；
2. 设表到某时刻 → 观察数字钟；
3. 断电（或按复位）再上电 → 时间应**接着上次走**，而不是回零；
4. 若拔掉 CR2032 再上电 → 会退回首次上电行为（标记丢失）。
