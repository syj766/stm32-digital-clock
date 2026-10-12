/**
 * @file    buzzer.h
 * @brief   无源蜂鸣器驱动（TIM3_CH3 PWM，S8050 低边 NPN，变调/音乐）
 *
 * 硬件：无源电磁式（谐振 2048Hz，16Ω 线圈），与板上 9mm/5.0mm 封装不兼容，
 *       飞线安装："+" 接 3V3 焊盘、"−" 接 BUZ_C 焊盘（S8050 集电极侧）。
 * 驱动：PB0 = TIM3_CH3（CubeMX 已配：PSC=0 / ARR=3905 / Pulse=1953 ≈ 2048Hz·50%）。
 * 原理：发声 = 输出方波（50% 占空贴谐振最响，运行时改 ARR 即变调）；
 *       静音 = 占空 0%（对无源管，恒高直流是不响的——别把占空当 GPIO 开关用）。
 * 兼容：若日后换回有源管，把 buzzer.h 里 BUZZER_TYPE_PASSIVE 改 0
 *       （发声 = 100% 占空直流），其余代码不动。
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1 = 无源（50% 方波驱动）；0 = 有源（100% 占空 = 直流开关） */
#define BUZZER_TYPE_PASSIVE   1u

/* 上电默认频率 = 该管谐振点 */
#define BUZZER_DEFAULT_FREQ   2048u

/* 常用音名（Hz，C 大调），melody / TODO(应用) 直接引用；NOTE_REST = 休止 */
enum {
    NOTE_REST = 0,
    NOTE_C4 = 262,  NOTE_D4 = 294,  NOTE_E4 = 330,  NOTE_F4 = 349,
    NOTE_G4 = 392,  NOTE_A4 = 440,  NOTE_B4 = 494,
    NOTE_C5 = 523,  NOTE_D5 = 587,  NOTE_E5 = 659,  NOTE_F5 = 698,
    NOTE_G5 = 784,  NOTE_A5 = 880,  NOTE_B5 = 988,
    NOTE_C6 = 1047, NOTE_D6 = 1175, NOTE_E6 = 1319, NOTE_G6 = 1568,
    NOTE_C7 = 2093,
};

void buzzer_init(void);                          /* 启动 PWM 并保持静音 */

void buzzer_tone(uint16_t freq, uint32_t ms);    /* 非阻塞：按频率发声 ms 毫秒后自动停；freq=0 静音 */
void buzzer_beep_ms(uint32_t ms);                /* = buzzer_tone(默认频率, ms) */
void buzzer_on(uint8_t on);                      /* 手动持续鸣/停（不等时自动关，配 buzzer_off 用） */
void buzzer_off(void);
void buzzer_task(void);                          /* 主循环调用：到时自动静音 */
uint8_t buzzer_playing(void);                    /* 1=正在发声（串乐谱/防重入用） */

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_H */