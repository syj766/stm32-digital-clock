/**
 * @file    seg_display.c
 * @brief   4 位共阳数码管驱动（段选 8 + 位选 4，动态扫描）
 *
 * 引脚与极性全部来自 pinmap.h；init() 自行配置 GPIO，
 * 不依赖 CubeMX 生成的 MX_GPIO_Init（PA3/PB3 差异不影响本模块）。
 */
#include "seg_display.h"
#include "pinmap.h"
#include "stm32f1xx_hal.h"

#define SEG_SCAN_INTERVAL_MS   2u   /* 每位点亮 2ms，整帧 8ms（125Hz），无频闪 */

/* ---- 引脚表（顺序与段码 bit0..bit7 对应：A B C D E F G DP） ---- */
static GPIO_TypeDef *const kSegPort[8] = {
    SEG_A_PORT, SEG_B_PORT, SEG_C_PORT, SEG_D_PORT,
    SEG_E_PORT, SEG_F_PORT, SEG_G_PORT, SEG_DP_PORT,
};
static const uint16_t kSegPin[8] = {
    SEG_A_PIN, SEG_B_PIN, SEG_C_PIN, SEG_D_PIN,
    SEG_E_PIN, SEG_F_PIN, SEG_G_PIN, SEG_DP_PIN,
};
static GPIO_TypeDef *const kDigPort[SEG_DIGIT_COUNT] = {
    DIG1_SEL_PORT, DIG2_SEL_PORT, DIG3_SEL_PORT, DIG4_SEL_PORT,
};
static const uint16_t kDigPin[SEG_DIGIT_COUNT] = {
    DIG1_SEL_PIN, DIG2_SEL_PIN, DIG3_SEL_PIN, DIG4_SEL_PIN,
};

/* ---- 显示缓冲 ---- */
static char     s_char[SEG_DIGIT_COUNT] = { ' ', ' ', ' ', ' ' };
static uint8_t  s_dp[SEG_DIGIT_COUNT]   = { 0, 0, 0, 0 };
static uint8_t  s_pos;
static uint32_t s_last_scan;

/* 段码：bit0=A ... bit6=G，bit7=DP；1 = 该段点亮（写出时按低有效电平驱动） */
static uint8_t seg_font(char ch)
{
    switch (ch) {
    case '0': return 0x3Fu;
    case '1': return 0x06u;
    case '2': return 0x5Bu;
    case '3': return 0x4Fu;
    case '4': return 0x66u;
    case '5': return 0x6Du;
    case '6': return 0x7Du;
    case '7': return 0x07u;
    case '8': return 0x7Fu;
    case '9': return 0x6Fu;
    case '-': return 0x40u;
    default:  return 0x00u;   /* ' ' 及未知字符 → 熄灭 */
    }
}

static void seg_write_raw(uint8_t pattern)   /* pattern 中 1 = 亮 */
{
    for (uint8_t i = 0u; i < 8u; i++) {
        GPIO_PinState level = (pattern & (uint8_t)(1u << i)) ? SEG_ON_LEVEL : SEG_OFF_LEVEL;
        HAL_GPIO_WritePin(kSegPort[i], kSegPin[i], level);
    }
}

static void dig_all_off(void)
{
    for (uint8_t i = 0u; i < SEG_DIGIT_COUNT; i++) {
        HAL_GPIO_WritePin(kDigPort[i], kDigPin[i], DIG_OFF_LEVEL);
    }
}

void seg_display_init(void)
{
    GPIO_InitTypeDef io = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;

    io.Pin = SEG_DP_PIN;                                  /* GPIOA 上唯一一位 */
    HAL_GPIO_Init(SEG_DP_PORT, &io);
    io.Pin = SEG_A_PIN | SEG_B_PIN | SEG_C_PIN | SEG_D_PIN |
             SEG_E_PIN | SEG_F_PIN | SEG_G_PIN |
             DIG1_SEL_PIN | DIG2_SEL_PIN | DIG3_SEL_PIN | DIG4_SEL_PIN;
    HAL_GPIO_Init(GPIOB, &io);

    seg_write_raw(0x00u);
    dig_all_off();
    s_pos = 0u;
    s_last_scan = HAL_GetTick();
}

void seg_display_show_char(uint8_t pos, char ch)
{
    if (pos >= SEG_DIGIT_COUNT) {
        return;
    }
    s_char[pos] = ch;
}

void seg_display_show_dp(uint8_t pos, uint8_t on)
{
    if (pos >= SEG_DIGIT_COUNT) {
        return;
    }
    s_dp[pos] = on ? 1u : 0u;
}

void seg_display_set_hhmm(uint8_t hh, uint8_t mm)
{
    /* TODO(应用): 若需 <10 时带前导消隐（" 9:05"），在这里改缓冲策略 */
    s_char[0] = (char)('0' + (hh / 10u) % 10u);
    s_char[1] = (char)('0' + hh % 10u);
    s_char[2] = (char)('0' + (mm / 10u) % 10u);
    s_char[3] = (char)('0' + mm % 10u);
}

void seg_display_task(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_last_scan) < SEG_SCAN_INTERVAL_MS) {
        return;
    }
    s_last_scan = now;

    dig_all_off();
    s_pos = (uint8_t)((s_pos + 1u) % SEG_DIGIT_COUNT);
    seg_write_raw((uint8_t)(seg_font(s_char[s_pos]) |
                             (uint8_t)(s_dp[s_pos] ? 0x80u : 0x00u)));
    HAL_GPIO_WritePin(kDigPort[s_pos], kDigPin[s_pos], DIG_ON_LEVEL);
}