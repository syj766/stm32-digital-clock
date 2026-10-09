/**
 * @file    pinmap.h
 * @brief   STM32 数字钟 —— 引脚映射定稿（唯一事实来源，不要在别处散落引脚宏）
 *
 * @verbatim
 * 证据链（2026-10-06 核实，非推测）：
 *   1. docs/schematic/03_5v_input.png          —— U1 核心板连线图（网络标签逐脚可读）
 *   2. stm32-digital-clock_v1.0.epro2 内 PCB 焊盘网络记录
 *      （U1 排针封装 uuid=9e60e809bd956221，40 个焊盘逐一比对，与截图一致）
 *   3. clock.ioc 引脚方向交叉验证：12 输出 / 4 输入 与本表完全吻合
 *
 * 已知差异（修复步骤见 patches/clock.ioc_PA3_to_PB3.md）：
 *   硬件 SEG_G 接 PB3（排针 11 脚），clock.ioc 却配置了 PA3、漏了 PB3。
 *   各模块 init() 自行按本文件配置引脚，不依赖 CubeMX 生成的 gpio 代码，
 *   因此无论 .ioc 是否重新生成，固件均按本表工作。
 * @endverbatim
 */
#ifndef PINMAP_H
#define PINMAP_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 数码管段选（8 位，共阳：段脚拉低 = 点亮，灌电流） ==================== */
#define SEG_A_PORT        GPIOB
#define SEG_A_PIN         GPIO_PIN_9     /* 排针 17 */
#define SEG_B_PORT        GPIOB
#define SEG_B_PIN         GPIO_PIN_8     /* 排针 16 */
#define SEG_C_PORT        GPIOB
#define SEG_C_PIN         GPIO_PIN_7     /* 排针 15 */
#define SEG_D_PORT        GPIOB
#define SEG_D_PIN         GPIO_PIN_6     /* 排针 14 */
#define SEG_E_PORT        GPIOB
#define SEG_E_PIN         GPIO_PIN_5     /* 排针 13 */
#define SEG_F_PORT        GPIOB
#define SEG_F_PIN         GPIO_PIN_4     /* 排针 12 */
#define SEG_G_PORT        GPIOB
#define SEG_G_PIN         GPIO_PIN_3     /* 排针 11 ⚠️ .ioc 现配的是 PA3，需修正 */
#define SEG_DP_PORT       GPIOA
#define SEG_DP_PIN        GPIO_PIN_15    /* 排针 10 */

/* ==================== 数码管位选（4 位，S8550 PNP 高边：拉低 = 选通） ==================== */
#define DIG1_SEL_PORT     GPIOB
#define DIG1_SEL_PIN      GPIO_PIN_12    /* 排针 1，千位/最左 */
#define DIG2_SEL_PORT     GPIOB
#define DIG2_SEL_PIN      GPIO_PIN_13    /* 排针 2 */
#define DIG3_SEL_PORT     GPIOB
#define DIG3_SEL_PIN      GPIO_PIN_14    /* 排针 3 */
#define DIG4_SEL_PORT     GPIOB
#define DIG4_SEL_PIN      GPIO_PIN_15    /* 排针 4，个位/最右 */

/* ==================== 用户按键（4 路，板上 10k 上拉 + 按下接地：低 = 按下） ==================== */
#define KEY1_PORT         GPIOA
#define KEY1_PIN          GPIO_PIN_8     /* 排针 5 */
#define KEY2_PORT         GPIOA
#define KEY2_PIN          GPIO_PIN_11    /* 排针 8 */
#define KEY3_PORT         GPIOA
#define KEY3_PIN          GPIO_PIN_12    /* 排针 9 */
#define KEY4_PORT         GPIOB
#define KEY4_PIN          GPIO_PIN_11    /* 排针 36 */

/* ==================== 状态 LED（4 路，灌电流驱动：拉低 = 亮） ==================== */
#define LED1_PORT         GPIOA
#define LED1_PIN          GPIO_PIN_4     /* 排针 29 */
#define LED2_PORT         GPIOA
#define LED2_PIN          GPIO_PIN_5     /* 排针 30 */
#define LED3_PORT         GPIOA
#define LED3_PIN          GPIO_PIN_6     /* 排针 31 */
#define LED4_PORT         GPIOA
#define LED4_PIN          GPIO_PIN_7     /* 排针 32 */

/* ==================== 蜂鸣器（S8050 低边 NPN：PB0 拉高 = 鸣响） ==================== */
#define BUZZER_PORT       GPIOB
#define BUZZER_PIN        GPIO_PIN_0     /* 排针 33，TIM3_CH3 复用脚；有源蜂鸣器 GPIO 直控 */

/* ==================== USART1 调试口（排针 J1：1=PA9 2=PA10 3=GND） ==================== */
#define USART1_TX_PORT    GPIOA
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PORT    GPIOA
#define USART1_RX_PIN     GPIO_PIN_10

/* ==================== 有效电平汇总（所有模块统一从这里取，不自行判断极性） ==================== */
#define SEG_ON_LEVEL       GPIO_PIN_RESET   /* 段选：低 = 亮 */
#define SEG_OFF_LEVEL      GPIO_PIN_SET
#define DIG_ON_LEVEL       GPIO_PIN_RESET   /* 位选：低 = 选通 */
#define DIG_OFF_LEVEL      GPIO_PIN_SET
#define LED_ON_LEVEL       GPIO_PIN_RESET   /* LED：低 = 亮 */
#define LED_OFF_LEVEL      GPIO_PIN_SET
#define KEY_PRESSED_LEVEL  GPIO_PIN_RESET   /* 按键：低 = 按下 */
#define BUZZER_ON_LEVEL    GPIO_PIN_SET     /* 蜂鸣器：高 = 响 */
#define BUZZER_OFF_LEVEL   GPIO_PIN_RESET

/*
 * ⚠️ JTAG 注意：PB3(JTDO) / PB4(NJTRST) / PA15(JTDI) 都是 JTAG 引脚，本设计同时占用了三个。
 * clock.ioc 的 SYS→Debug 已设为 Serial Wire（hal_msp.c 生成 __HAL_AFIO_REMAP_SWJ_NOJTAG()）：
 * JTAG 三脚已释放、SWD 调试口保留。切勿改回 Full JTAG，否则段选/位选会失灵；
 * 也别在自己代码里把 PA13/PA14 配成 GPIO，那等于自拆调试口。
 */

#ifdef __cplusplus
}
#endif

#endif /* PINMAP_H */