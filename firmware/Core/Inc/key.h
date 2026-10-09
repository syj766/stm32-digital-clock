/**
 * @file    key.h
 * @brief   4 路独立按键扫描（板上 10k 上拉 + 100nF，低电平 = 按下）
 */
#ifndef KEY_H
#define KEY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEY_ID_1 = 0,
    KEY_ID_2,
    KEY_ID_3,
    KEY_ID_4,
    KEY_ID_COUNT
} key_id_t;

typedef enum {
    KEY_EVT_SHORT = 0,   /* 稳定按下后，在长按阈值前释放 → 短按 */
    KEY_EVT_LONG        /* 持续按住达到长按阈值时触发一次，释放不再补短按 */
} key_evt_t;

typedef void (*key_event_cb_t)(key_id_t id, key_evt_t evt);

void key_init(void);
void key_set_callback(key_event_cb_t cb);   /* 传 NULL 关闭回调 */
void key_task(void);                        /* 主循环调用，内部按 HAL_GetTick 消抖 */

#ifdef __cplusplus
}
#endif

#endif /* KEY_H */