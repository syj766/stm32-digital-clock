/**
 * @file    buzzer.c
 * @brief   无源蜂鸣器驱动（TIM3_CH3 PWM，变调/音乐）
 *
 * 频率-周期换算：PWM 频率 = BUZZER_TIMER_CLOCK_HZ ÷ (ARR+1)，
 * 占空 = CCR ÷ (ARR+1)。buzzer_tone() 运行时按音符频率重算 ARR/CCR。
 * 说明：PB0 在 MX_TIM3_Init 之后、buzzer_init 之前处于"PWM 未启动"态，
 *       输出恒高——对无源管无影响（直流不发声），故无需处理。
 */
#include "buzzer.h"
#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim3;   /* CubeMX 生成，定义于 main.c */

/* 定时器输入时钟：本工程 HSI 8MHz、APB1 不分频 → TIM3 = 8MHz。
 * ⚠️ 若日后在 CubeMX 改主频/分频，这里必须同步修改。 */
#define BUZZER_TIMER_CLOCK_HZ   8000000u

#if BUZZER_TYPE_PASSIVE
#define BUZZER_ON_DUTY_PERCENT   50u   /* 无源：50% 方波最响 */
#else
#define BUZZER_ON_DUTY_PERCENT  100u   /* 有源：恒高直流 */
#endif

static uint32_t s_off_at;    /* 自动静音时刻（HAL_GetTick 域） */
static uint8_t  s_playing;

/* freq=0 → 占空 0（静音）；freq>0 → 按频率重算 ARR/CCR 并连续发声 */
static void buzzer_set_raw(uint16_t freq)
{
    if (freq == 0u) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0u);
        return;
    }

    uint32_t total = BUZZER_TIMER_CLOCK_HZ / freq;      /* = ARR+1 */
    if (total > 65536u) {
        total = 65536u;                                  /* PSC=0 时最低约 122Hz */
    }
    if (total < 2u) {
        total = 2u;                                      /* 上限约 4MHz */
    }

    __HAL_TIM_SET_AUTORELOAD(&htim3, (uint16_t)(total - 1u));
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3,
                          (uint16_t)((total * BUZZER_ON_DUTY_PERCENT) / 100u));
}

void buzzer_init(void)
{
    /* PB0 复用与 PWM 模式已由 CubeMX 的 MX_TIM3_Init() 配好；
     * 这里把占空清零（静音）后启动 PWM 输出。 */
    buzzer_set_raw(0u);
    (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    s_playing = 0u;
}

void buzzer_tone(uint16_t freq, uint32_t ms)
{
    if ((freq == 0u) || (ms == 0u)) {
        buzzer_off();
        return;
    }
    buzzer_set_raw(freq);
    s_off_at = HAL_GetTick() + ms;
    s_playing = 1u;
}

void buzzer_beep_ms(uint32_t ms)
{
    buzzer_tone(BUZZER_DEFAULT_FREQ, ms);
}

void buzzer_on(uint8_t on)
{
    if (on) {
        buzzer_set_raw(BUZZER_DEFAULT_FREQ);   /* 手动持续鸣，须 buzzer_off() 停 */
        s_playing = 0u;
    } else {
        buzzer_off();
    }
}

void buzzer_off(void)
{
    buzzer_set_raw(0u);
    s_playing = 0u;
}

void buzzer_task(void)
{
    if (s_playing && ((HAL_GetTick() - s_off_at) < 0x80000000u)) {
        /* HAL_GetTick 已越过 deadline（回绕安全：差值 < 2^31 视为超时） */
        buzzer_off();
    }
}

uint8_t buzzer_playing(void)
{
    return s_playing;
}