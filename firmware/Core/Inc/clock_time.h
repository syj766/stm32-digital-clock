/**
 * @file    clock_time.h
 * @brief   软件走时（SysTick 1ms 时基 → 时/分/秒）
 *
 * TODO(硬件演进): 硬件已具备 RTC 备电（CR2032 + 双二极管或门，VBAT 排针 21）。
 * 若要断电走时，在 CubeMX 激活 RTC（LSE 32.768k）后，把本模块实现替换为
 * RTC 读写，对外 API 保持不变即可，app.c 无需改动。
 */
#ifndef CLOCK_TIME_H
#define CLOCK_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} clock_hms_t;

void     clock_time_init(void);            /* 复位到 12:00:00 */
void     clock_time_isr_1ms(void);         /* ⚠️ 由 SysTick_Handler 调用，见 patches/ */
void     clock_time_set(uint8_t h, uint8_t m, uint8_t s);
void     clock_time_get(clock_hms_t *out);
uint8_t  clock_time_tick_second(void);     /* 每秒返回一次 1（读后自动清零），主循环轮询 */
uint32_t clock_time_ms(void);              /* 自由运行毫秒计数（回绕安全用法：now - t0） */

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_TIME_H */