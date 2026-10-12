/**
 * @file    led.c
 * @brief   4 路状态 LED（灌电流驱动，拉低 = 亮）
 */
#include "led.h"
#include "pinmap.h"
#include "stm32f1xx_hal.h"

static GPIO_TypeDef *const kLedPort[LED_COUNT] = {
    LED1_PORT, LED2_PORT, LED3_PORT, LED4_PORT,
};
static const uint16_t kLedPin[LED_COUNT] = {
    LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN,
};

void led_init(void)
{
    GPIO_InitTypeDef io = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE();

    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    io.Pin   = LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN;
    HAL_GPIO_Init(GPIOA, &io);

    for (uint8_t i = 0u; i < LED_COUNT; i++) {
        HAL_GPIO_WritePin(kLedPort[i], kLedPin[i], LED_OFF_LEVEL);
    }
}

void led_set(led_id_t id, uint8_t on)
{
    if (id >= LED_COUNT) {
        return;
    }
    HAL_GPIO_WritePin(kLedPort[id], kLedPin[id], on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

void led_toggle(led_id_t id)
{
    if (id >= LED_COUNT) {
        return;
    }
    HAL_GPIO_TogglePin(kLedPort[id], kLedPin[id]);
}