/**
 * @file    key.c
 * @brief   4 路独立按键扫描（消抖 + 短按/长按事件）
 */
#include "key.h"
#include "pinmap.h"
#include "stm32f1xx_hal.h"

#define KEY_DEBOUNCE_MS    20u
#define KEY_LONG_PRESS_MS  1000u

static GPIO_TypeDef *const kKeyPort[KEY_ID_COUNT] = {
    KEY1_PORT, KEY2_PORT, KEY3_PORT, KEY4_PORT,
};
static const uint16_t kKeyPin[KEY_ID_COUNT] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN,
};

typedef struct {
    uint8_t  stable;      /* 消抖后确认的按下状态 */
    uint8_t  last_raw;    /* 上次采样的原始电平 */
    uint8_t  long_fired;
    uint32_t t_change;    /* 原始电平最近一次跳变时刻 */
    uint32_t t_press;     /* 确认按下时刻 */
} key_state_t;

static key_state_t    s_keys[KEY_ID_COUNT];
static key_event_cb_t s_cb;

void key_init(void)
{
    GPIO_InitTypeDef io = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    io.Mode  = GPIO_MODE_INPUT;
    io.Pull  = GPIO_NOPULL;               /* 板上已有 10k 上拉，MCU 不再上拉 */
    io.Speed = GPIO_SPEED_FREQ_LOW;
    io.Pin   = KEY1_PIN | KEY2_PIN | KEY3_PIN;
    HAL_GPIO_Init(GPIOA, &io);
    io.Pin   = KEY4_PIN;
    HAL_GPIO_Init(GPIOB, &io);

    for (uint8_t i = 0u; i < KEY_ID_COUNT; i++) {
        s_keys[i].stable    = 0u;
        s_keys[i].last_raw  = 0u;
        s_keys[i].long_fired = 0u;
        s_keys[i].t_change  = HAL_GetTick();
        s_keys[i].t_press   = 0u;
    }
    s_cb = 0;
}

void key_set_callback(key_event_cb_t cb)
{
    s_cb = cb;
}

void key_task(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0u; i < KEY_ID_COUNT; i++) {
        key_state_t *k = &s_keys[i];
        uint8_t pressed =
            (HAL_GPIO_ReadPin(kKeyPort[i], kKeyPin[i]) == KEY_PRESSED_LEVEL) ? 1u : 0u;

        if (pressed != k->last_raw) {           /* 电平有抖动，重新计时 */
            k->last_raw = pressed;
            k->t_change = now;
            continue;
        }
        if ((now - k->t_change) < KEY_DEBOUNCE_MS) {
            continue;                            /* 电平稳定时间不够 */
        }

        if (pressed != k->stable) {              /* 状态翻转被确认 */
            k->stable = pressed;
            if (k->stable) {
                k->t_press    = now;
                k->long_fired = 0u;
            } else if (!k->long_fired && s_cb) {
                s_cb((key_id_t)i, KEY_EVT_SHORT);
            }
        }

        if (k->stable && !k->long_fired &&
            (now - k->t_press) >= KEY_LONG_PRESS_MS) {
            k->long_fired = 1u;
            if (s_cb) {
                s_cb((key_id_t)i, KEY_EVT_LONG);
            }
        }
    }
}