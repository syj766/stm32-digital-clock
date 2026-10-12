/**
 * @file    led.h
 * @brief   4 路状态 LED（灌电流驱动，拉低 = 亮）
 */
#ifndef LED_H
#define LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_4,
    LED_COUNT
} led_id_t;

void led_init(void);
void led_set(led_id_t id, uint8_t on);
void led_toggle(led_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */