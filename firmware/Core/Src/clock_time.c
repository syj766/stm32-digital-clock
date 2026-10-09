/**
 * @file    clock_time.c
 * @brief   软件走时（SysTick 1ms 时基 → 时/分/秒）
 */
#include "clock_time.h"

#define SECS_PER_DAY   86400u
#define DEFAULT_HOURS  12u

static volatile uint32_t s_ms;            /* 自由运行毫秒计数 */
static volatile uint16_t s_ms_in_sec;
static volatile uint32_t s_secs;          /* 当天 0 点起的秒数 */
static volatile uint8_t  s_sec_flag;      /* 秒事件标志（读后清零） */

void clock_time_init(void)
{
    s_ms        = 0u;
    s_ms_in_sec = 0u;
    s_secs      = (uint32_t)DEFAULT_HOURS * 3600u;
    s_sec_flag  = 0u;
}

/* SysTick 中断里调用（1ms 一次）。放在 HAL_IncTick() 之前，逻辑极短不影响 HAL 时基。 */
void clock_time_isr_1ms(void)
{
    s_ms++;
    s_ms_in_sec++;
    if (s_ms_in_sec >= 1000u) {
        s_ms_in_sec = 0u;
        s_secs++;
        if (s_secs >= SECS_PER_DAY) {
            s_secs = 0u;
        }
        s_sec_flag = 1u;
    }
}

void clock_time_set(uint8_t h, uint8_t m, uint8_t s)
{
    if ((h >= 24u) || (m >= 60u) || (s >= 60u)) {
        return;                            /* 非法值直接忽略 */
    }
    s_secs = (uint32_t)h * 3600u + (uint32_t)m * 60u + (uint32_t)s;
}

void clock_time_get(clock_hms_t *out)
{
    if (out == 0) {
        return;
    }
    uint32_t secs = s_secs;                /* 32 位读取原子 */
    out->hours   = (uint8_t)(secs / 3600u);
    out->minutes = (uint8_t)((secs / 60u) % 60u);
    out->seconds = (uint8_t)(secs % 60u);
}

uint8_t clock_time_tick_second(void)
{
    uint8_t flag = s_sec_flag;
    s_sec_flag = 0u;
    return flag;
}

uint32_t clock_time_ms(void)
{
    return s_ms;
}