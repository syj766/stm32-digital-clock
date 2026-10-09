/**
 * @file    hw_test.c
 * @brief   硬件上电自检——除蜂鸣器外直推 pinmap.h GPIO（PB0 已归 TIM3 PWM，蜂鸣器走 buzzer 驱动）
 */
#include "hw_test.h"
#include "pinmap.h"
#include "buzzer.h"
#include "uart_console.h"
#include "stm32f1xx_hal.h"

#define T_LED_STEP_MS      150u
#define T_ALL_ON_MS        1500u
#define T_DIGIT_ON_MS      400u
#define T_SEG_ON_MS        250u
#define T_KEY_TIMEOUT_MS   10000u

/* ---- 引脚表（与 pinmap.h 一一对应；字符串仅用于串口播报） ---- */
static GPIO_TypeDef *const kSegPort[8] = {
    SEG_A_PORT, SEG_B_PORT, SEG_C_PORT, SEG_D_PORT,
    SEG_E_PORT, SEG_F_PORT, SEG_G_PORT, SEG_DP_PORT,
};
static const uint16_t kSegPin[8] = {
    SEG_A_PIN, SEG_B_PIN, SEG_C_PIN, SEG_D_PIN,
    SEG_E_PIN, SEG_F_PIN, SEG_G_PIN, SEG_DP_PIN,
};
static const char *const kSegName[8] = {
    "A=PB9", "B=PB8", "C=PB7", "D=PB6", "E=PB5", "F=PB4", "G=PB3", "DP=PA15",
};
static GPIO_TypeDef *const kDigPort[4] = {
    DIG1_SEL_PORT, DIG2_SEL_PORT, DIG3_SEL_PORT, DIG4_SEL_PORT,
};
static const uint16_t kDigPin[4] = {
    DIG1_SEL_PIN, DIG2_SEL_PIN, DIG3_SEL_PIN, DIG4_SEL_PIN,
};
static GPIO_TypeDef *const kLedPort[4] = {
    LED1_PORT, LED2_PORT, LED3_PORT, LED4_PORT,
};
static const uint16_t kLedPin[4] = {
    LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN,
};
static GPIO_TypeDef *const kKeyPort[4] = {
    KEY1_PORT, KEY2_PORT, KEY3_PORT, KEY4_PORT,
};
static const uint16_t kKeyPin[4] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN,
};
static const char *const kKeyName[4] = {
    "K1=PA8", "K2=PA11", "K3=PA12", "K4=PB11",
};

/* '1'~'4' 的段码（bit0=A..bit6=G，1=亮），按键测试时显示键号用 */
static const uint8_t kFont1to4[4] = { 0x06u, 0x5Bu, 0x4Fu, 0x66u };

static void set_segments(uint8_t on_mask)     /* bit0=A ... bit7=DP，1=亮 */
{
    for (uint8_t i = 0u; i < 8u; i++) {
        GPIO_PinState level = ((on_mask >> i) & 1u) ? SEG_ON_LEVEL : SEG_OFF_LEVEL;
        HAL_GPIO_WritePin(kSegPort[i], kSegPin[i], level);
    }
}

static void set_digit(uint8_t idx, uint8_t on)
{
    HAL_GPIO_WritePin(kDigPort[idx], kDigPin[idx], on ? DIG_ON_LEVEL : DIG_OFF_LEVEL);
}

static void display_all_off(void)
{
    set_segments(0x00u);
    for (uint8_t i = 0u; i < 4u; i++) {
        set_digit(i, 0u);
    }
}

/* PB0 已归 TIM3 PWM（AF 模式），GPIO 直写无效——蜂鸣器是唯一必须走驱动 API 的测试项。
 * 阻塞等待发声结束（自检是顺序流程，允许阻塞）。 */
static void beep(uint32_t ms)
{
    buzzer_beep_ms(ms);
    while (buzzer_playing()) {
        buzzer_task();
    }
}

/* 上行音阶：验证变调（音乐功能） */
static void play_scale(void)
{
    static const uint16_t scale[4] = { NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6 };

    for (uint8_t i = 0u; i < 4u; i++) {
        buzzer_tone(scale[i], 150u);
        while (buzzer_playing()) {
            buzzer_task();
        }
    }
}

static void test_leds(void)
{
    uart_printf("[test 1/6] LED chase x2 (PA4-PA7)\n");
    for (uint8_t round = 0u; round < 2u; round++) {
        for (uint8_t i = 0u; i < 4u; i++) {
            HAL_GPIO_WritePin(kLedPort[i], kLedPin[i], LED_ON_LEVEL);
            HAL_Delay(T_LED_STEP_MS);
            HAL_GPIO_WritePin(kLedPort[i], kLedPin[i], LED_OFF_LEVEL);
        }
    }
}

static void test_display_all_on(void)
{
    uart_printf("[test 2/6] all digits show \"8.\" for 1.5s\n");
    set_segments(0xFFu);
    for (uint8_t i = 0u; i < 4u; i++) {
        set_digit(i, 1u);
    }
    HAL_Delay(T_ALL_ON_MS);
    display_all_off();
}

static void test_display_digits(void)
{
    uart_printf("[test 3/6] digit sweep DIG1..DIG4 x2\n");
    for (uint8_t round = 0u; round < 2u; round++) {
        for (uint8_t i = 0u; i < 4u; i++) {
            set_segments(0xFFu);
            set_digit(i, 1u);
            HAL_Delay(T_DIGIT_ON_MS);
            set_digit(i, 0u);
        }
    }
}

static void test_display_segments(void)
{
    uart_printf("[test 4/6] segment sweep A..DP on all digits\n");
    for (uint8_t i = 0u; i < 8u; i++) {
        uart_printf("  SEG_%s\n", kSegName[i]);
        set_segments((uint8_t)(1u << i));
        for (uint8_t d = 0u; d < 4u; d++) {
            set_digit(d, 1u);
        }
        HAL_Delay(T_SEG_ON_MS);
        for (uint8_t d = 0u; d < 4u; d++) {
            set_digit(d, 0u);
        }
    }
}

static void test_buzzer(void)
{
    uart_printf("[test 5/6] buzzer: 3 short + 1 long + scale (TIM3 PWM)\n");
    beep(80u);
    HAL_Delay(120u);
    beep(80u);
    HAL_Delay(120u);
    beep(80u);
    HAL_Delay(200u);
    beep(400u);
    HAL_Delay(200u);
    play_scale();
}

static void test_keys(void)
{
    uart_printf("[test 6/6] press K1..K4 one by one (%us timeout)\n",
                (unsigned)(T_KEY_TIMEOUT_MS / 1000u));
    uart_printf("  hint: on press, LED n lights + digit n shows n\n");

    uint8_t  done = 0u;
    uint32_t t0   = HAL_GetTick();

    while ((done != 0x0Fu) && ((HAL_GetTick() - t0) < T_KEY_TIMEOUT_MS)) {
        for (uint8_t i = 0u; i < 4u; i++) {
            if (((done >> i) & 1u) != 0u) {
                continue;
            }
            if (HAL_GPIO_ReadPin(kKeyPort[i], kKeyPin[i]) == KEY_PRESSED_LEVEL) {
                done |= (uint8_t)(1u << i);
                uart_printf("  KEY%d (%s) OK\n", (int)(i + 1u), kKeyName[i]);
                HAL_GPIO_WritePin(kLedPort[i], kLedPin[i], LED_ON_LEVEL);
                set_segments(kFont1to4[i]);
                set_digit(i, 1u);
                beep(50u);
                HAL_Delay(300u);                /* 展示一下再灭 */
                set_digit(i, 0u);
                HAL_GPIO_WritePin(kLedPort[i], kLedPin[i], LED_OFF_LEVEL);
            }
        }
    }
    if (done == 0x0Fu) {
        uart_printf("  keys 4/4 OK\n");
    } else {
        uart_printf("  keys %u/4 (timeout - check untested keys)\n",
                    (unsigned)__builtin_popcount(done));
    }
}

void hw_test_run(void)
{
    uart_console_init();                    /* 自持：单独拷出也能跑；重复初始化无害 */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    uart_printf("\n===== HW SELF TEST START =====\n");
    uart_printf("(watch the display & listen, ~10s)\n");

    test_leds();
    test_display_all_on();
    test_display_digits();
    test_display_segments();
    test_buzzer();
    test_keys();

    display_all_off();
    uart_printf("===== TEST DONE -> clock app =====\n");
    beep(80u);
    HAL_Delay(100u);
    beep(80u);
}