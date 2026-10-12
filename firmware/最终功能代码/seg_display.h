/**
 * @file    seg_display.h
 * @brief   4 位共阳数码管驱动（段选 8 + 位选 4，动态扫描）
 */
#ifndef SEG_DISPLAY_H
#define SEG_DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEG_DIGIT_COUNT   4u   /* DIG1..DIG4，pos 0..3 = 左..右 */

void seg_display_init(void);

/* 写显示缓冲（不立即刷硬件，由 seg_display_task() 轮流扫描输出） */
void seg_display_show_char(uint8_t pos, char ch);        /* '0'-'9' ' ' '-'，其余按空白 */
void seg_display_show_dp(uint8_t pos, uint8_t on);       /* 小数点，pos=1 常用作时间冒号 */
void seg_display_set_hhmm(uint8_t hh, uint8_t mm);       /* 便捷：显示 HH:MM（补零） */

/* 主循环调用：内部按 2ms 间隔轮流点亮一位（占空 1/4） */
void seg_display_task(void);

#ifdef __cplusplus
}
#endif

#endif /* SEG_DISPLAY_H */