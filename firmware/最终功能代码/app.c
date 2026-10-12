/**
 * @file    app.c
 * @brief   应用层：模块装配 + 时钟主逻辑（骨架默认行为 + TODO 清单）
 *
 * 骨架上电默认行为（用于先验证硬件）：
 *   - 数码管显示 HH:MM，冒号（第 2 位小数点）每秒闪烁
 *   - 串口 115200 打印横幅；秒变化时可选打印
 *   - 任意按键：短按/长按蜂鸣 30ms 并回显（引脚级自检）
 * TODO(应用)：整点报时、串口调时命令、闹钟启用指示（LED 已占用为模式指示）
 */
#include "app.h"
#include "pinmap.h"
#include "led.h"
#include "buzzer.h"
#include "seg_display.h"
#include "key.h"
#include "uart_console.h"
#include "clock_time.h"
#include "stm32f1xx_hal.h"

/* ---- 音乐闹钟：超级马里奥主旋律 ---- */
typedef struct {
    uint16_t freq;
    uint32_t duration_ms;
    uint32_t gap_ms;
} music_note_t;

/* 音符时值 (约 250 bpm) */
#define M_GAP   20u
#define M_Q     240u  /* quarter = 四分 */
#define M_E     120u  /* eighth  = 八分 */
#define M_T      80u  /* triplet = 三连音 */
#define M_H     480u  /* half    = 二分 */
#define M_As4   466u  /* Bb4 = A#4，不在 buzzer.h 的 C 大调表内 */

static const music_note_t startup_melody[] = {
    /* ==== 句1: 经典开场 ==== */
    {NOTE_E5,  M_E, M_GAP}, {NOTE_E5,  M_E, M_GAP},
    {NOTE_REST, M_E, M_GAP},
    {NOTE_E5,  M_E, M_GAP},
    {NOTE_REST, M_E, M_GAP},
    {NOTE_C5,  M_E, M_GAP},
    {NOTE_E5,  M_E, M_GAP},
    {NOTE_G5,  M_Q, M_GAP},

    {NOTE_REST, M_Q, M_GAP},
    {NOTE_G4,  M_Q, M_GAP},
    {NOTE_REST, M_Q, M_GAP},

    /* ==== 句2: 地下世界感 ==== */
    {NOTE_C5,  M_Q, M_GAP},
    {NOTE_REST, M_Q, M_GAP},
    {NOTE_G4,  M_E, M_GAP},
    {NOTE_REST, M_E, M_GAP},
    {NOTE_E4,  M_Q, M_GAP},
    {NOTE_REST, M_Q, M_GAP},

    {NOTE_A4,  M_E, M_GAP},
    {NOTE_B4,  M_E, M_GAP},
    {M_As4,    M_E, M_GAP},
    {NOTE_A4,  M_E, M_GAP},

    /* ==== 句3: 爬升 + 收尾 ==== */
    {NOTE_G4,  M_T, M_GAP},
    {NOTE_E5,  M_T, M_GAP},
    {NOTE_G5,  M_T, M_GAP},
    {NOTE_A5,  M_Q, M_GAP},

    {NOTE_F5,  M_E, M_GAP},
    {NOTE_G5,  M_E, M_GAP},
    {NOTE_E5,  M_Q, M_GAP},

    {NOTE_C5,  M_E, M_GAP},
    {NOTE_D5,  M_E, M_GAP},
    {NOTE_B4,  M_H, M_GAP},

    /* ==== 循环间隔 ==== */
    {NOTE_REST, M_Q, M_GAP},
};

#define STARTUP_MELODY_COUNT \
    (sizeof(startup_melody) / sizeof(startup_melody[0]))

/* ---- 状态机 ---- */
typedef enum {
    APP_STATE_MELODY = 0,
    APP_STATE_CLOCK,
    APP_STATE_ADJUST,
    APP_STATE_STOPWATCH,
    APP_STATE_COUNTDOWN,
    APP_STATE_ALARM,
} app_state_t;

static app_state_t s_app_state = APP_STATE_MELODY;
static uint32_t    s_melody_idx;
static uint32_t    s_melody_tick;
static uint8_t     s_melody_once;

static void melody_start_note(void)
{
    const music_note_t *n = &startup_melody[s_melody_idx];
    buzzer_tone(n->freq, n->duration_ms);
    s_melody_tick = HAL_GetTick() + n->duration_ms + n->gap_ms;
}

/* ---- 调时模式暂存 ---- */
static uint8_t s_adj_hours;
static uint8_t s_adj_minutes;

/* ---- 秒表 ---- */
static uint32_t s_sw_elapsed_ms;
static uint32_t s_sw_last_tick;
static uint8_t  s_sw_running;

static void stopwatch_update_display(void)
{
    uint32_t total_sec = s_sw_elapsed_ms / 1000u;
    uint8_t min = (uint8_t)((total_sec / 60u) % 100u);
    uint8_t sec = (uint8_t)(total_sec % 60u);
    seg_display_set_hhmm(min, sec);
}

static void stopwatch_enter(void)
{
    s_sw_elapsed_ms = 0u;
    s_sw_running    = 1u;
    s_sw_last_tick  = HAL_GetTick();
    s_app_state     = APP_STATE_STOPWATCH;
    stopwatch_update_display();
    uart_printf("[stopwatch] enter & start\r\n");
}

static void stopwatch_leave(void)
{
    s_app_state = APP_STATE_CLOCK;
    uart_printf("[stopwatch] exit -> %ums\r\n", (unsigned)s_sw_elapsed_ms);
}

/* ---- 倒计时 ---- */
static uint32_t s_cd_remaining_ms;
static uint32_t s_cd_last_tick;
static uint8_t  s_cd_running;

static void countdown_update_display(void)
{
    uint32_t total_sec = s_cd_remaining_ms / 1000u;
    uint8_t min = (uint8_t)((total_sec / 60u) % 100u);
    uint8_t sec = (uint8_t)(total_sec % 60u);
    seg_display_set_hhmm(min, sec);
}

static void countdown_add_minute(void)
{
    uint32_t total_sec = s_cd_remaining_ms / 1000u;
    uint8_t min = (uint8_t)((total_sec / 60u) % 100u);
    uint8_t sec = (uint8_t)(total_sec % 60u);
    min = (min + 1u) % 100u;
    s_cd_remaining_ms = ((uint32_t)min * 60u + (uint32_t)sec) * 1000u;
    countdown_update_display();
}

static void countdown_add_second(void)
{
    uint32_t total_sec = s_cd_remaining_ms / 1000u;
    uint8_t min = (uint8_t)((total_sec / 60u) % 100u);
    uint8_t sec = (uint8_t)(total_sec % 60u);
    sec = (sec + 1u) % 60u;
    s_cd_remaining_ms = ((uint32_t)min * 60u + (uint32_t)sec) * 1000u;
    countdown_update_display();
}

static void countdown_enter(void)
{
    s_cd_remaining_ms = 0u;
    s_cd_running      = 0u;
    s_app_state       = APP_STATE_COUNTDOWN;
    countdown_update_display();
    uart_printf("[countdown] enter mode\r\n");
}

/* ---- 闹钟 ---- */
static uint8_t s_alarm_hours   = 0u;
static uint8_t s_alarm_minutes = 0u;
static uint8_t s_alarm_enabled = 0u;

static void alarm_update_display(void)
{
    seg_display_set_hhmm(s_alarm_hours, s_alarm_minutes);
    seg_display_show_dp(2u, s_alarm_enabled ? 1u : 0u);
}

static void alarm_enter(void)
{
    s_app_state = APP_STATE_ALARM;
    alarm_update_display();
    uart_printf("[alarm] enter mode, %02u:%02u %s\r\n",
                (unsigned)s_alarm_hours, (unsigned)s_alarm_minutes,
                s_alarm_enabled ? "ON" : "OFF");
}

/* ---- LED 模式指示：主界面 4 亮，各模式对应单灯亮，旋律中全灭 ---- */
static void led_apply(app_state_t st)
{
    uint8_t l1 = 0u, l2 = 0u, l3 = 0u, l4 = 0u;

    switch (st) {
    case APP_STATE_CLOCK:     l1 = 1u; l2 = 1u; l3 = 1u; l4 = 1u; break;
    case APP_STATE_STOPWATCH: l1 = 1u; break;
    case APP_STATE_COUNTDOWN: l2 = 1u; break;
    case APP_STATE_ALARM:     l3 = 1u; break;
    case APP_STATE_ADJUST:    l4 = 1u; break;
    case APP_STATE_MELODY:
        if (s_melody_once) {
            uint8_t phase = (uint8_t)((HAL_GetTick() / 250u) & 1u);
            l1 = phase; l2 = phase; l3 = phase; l4 = phase;
        }
        break;
    default:                  break;
    }
    led_set(LED_1, l1);
    led_set(LED_2, l2);
    led_set(LED_3, l3);
    led_set(LED_4, l4);
}

/* ---- 按键回调 ---- */
static void on_key(key_id_t id, key_evt_t evt)
{
    /* -- 旋律中：任意键停止 -- */
    if (s_app_state == APP_STATE_MELODY) {
        buzzer_off();
        s_app_state = APP_STATE_CLOCK;
        uart_printf("[melody] stopped by K%u\r\n", (unsigned)id + 1u);
        return;
    }

    /* -- 调时模式 -- */
    if (s_app_state == APP_STATE_ADJUST) {
        /* K4 长按: 保存并退出 */
        if (id == KEY_ID_4 && evt == KEY_EVT_LONG) {
            buzzer_beep_ms(30u);
            clock_time_set(s_adj_hours, s_adj_minutes, 0u);
            s_app_state = APP_STATE_CLOCK;
            uart_printf("[adjust] saved %02u:%02u\r\n",
                        (unsigned)s_adj_hours, (unsigned)s_adj_minutes);
            return;
        }
        /* K1 短按: 小时 ++ */
        if (id == KEY_ID_1 && evt == KEY_EVT_SHORT) {
            s_adj_hours = (s_adj_hours + 1u) % 24u;
            seg_display_set_hhmm(s_adj_hours, s_adj_minutes);
            return;
        }
        /* K3 短按: 分钟 ++ */
        if (id == KEY_ID_3 && evt == KEY_EVT_SHORT) {
            s_adj_minutes = (s_adj_minutes + 1u) % 60u;
            seg_display_set_hhmm(s_adj_hours, s_adj_minutes);
            return;
        }
        /* K2 及其他: 不响应 */
        return;
    }

    /* -- 主界面：K1=秒表  K2=倒计时  K3=闹钟  K4=时间调整 -- */
    if (s_app_state == APP_STATE_CLOCK) {
        if (id == KEY_ID_4 && evt == KEY_EVT_SHORT) {
            clock_hms_t t;
            clock_time_get(&t);
            s_adj_hours   = t.hours;
            s_adj_minutes = t.minutes;
            s_app_state   = APP_STATE_ADJUST;
            seg_display_set_hhmm(s_adj_hours, s_adj_minutes);
            uart_printf("[adjust] enter mode\r\n");
            return;
        }
        if (id == KEY_ID_1 && evt == KEY_EVT_SHORT) {
            stopwatch_enter();
            return;
        }
        if (id == KEY_ID_2 && evt == KEY_EVT_SHORT) {
            countdown_enter();
            return;
        }
        if (id == KEY_ID_3 && evt == KEY_EVT_SHORT) {
            alarm_enter();
            return;
        }
        return;
    }

    /* -- 秒表模式 -- */
    if (s_app_state == APP_STATE_STOPWATCH) {
        /* K1 长按: 清零并重新计时 */
        if (id == KEY_ID_1 && evt == KEY_EVT_LONG) {
            s_sw_elapsed_ms = 0u;
            s_sw_running    = 1u;
            s_sw_last_tick  = HAL_GetTick();
            stopwatch_update_display();
            uart_printf("[stopwatch] reset & restart\r\n");
            return;
        }
        /* K2: 暂停 */
        if (id == KEY_ID_2 && evt == KEY_EVT_SHORT) {
            s_sw_running = 0u;
            uart_printf("[stopwatch] pause at %ums\r\n",
                        (unsigned)s_sw_elapsed_ms);
            return;
        }
        /* K3: 继续 */
        if (id == KEY_ID_3 && evt == KEY_EVT_SHORT) {
            if (!s_sw_running) {
                s_sw_running   = 1u;
                s_sw_last_tick = HAL_GetTick();
                uart_printf("[stopwatch] resume\r\n");
            }
            return;
        }
        /* K4 长按: 退出 */
        if (id == KEY_ID_4 && evt == KEY_EVT_LONG) {
            buzzer_beep_ms(30u);
            stopwatch_leave();
            return;
        }
        return;
    }

    /* -- 倒计时模式 -- */
    if (s_app_state == APP_STATE_COUNTDOWN) {
        /* K1 短按: 分钟 +1（仅在未启动时可调） */
        if (id == KEY_ID_1 && evt == KEY_EVT_SHORT && !s_cd_running) {
            countdown_add_minute();
            return;
        }
        /* K2 短按: 启动 / 暂停 / 继续 */
        if (id == KEY_ID_2 && evt == KEY_EVT_SHORT) {
            if (s_cd_remaining_ms == 0u) return;
            if (!s_cd_running) {
                s_cd_running   = 1u;
                s_cd_last_tick = HAL_GetTick();
                uart_printf("[countdown] start\r\n");
            } else {
                s_cd_running = 0u;
                uart_printf("[countdown] pause at %ums\r\n",
                            (unsigned)s_cd_remaining_ms);
            }
            return;
        }
        /* K3 短按: 秒 +1（仅在未启动时可调） */
        if (id == KEY_ID_3 && evt == KEY_EVT_SHORT && !s_cd_running) {
            countdown_add_second();
            return;
        }
        /* K4 长按: 退出 */
        if (id == KEY_ID_4 && evt == KEY_EVT_LONG) {
            buzzer_beep_ms(30u);
            s_cd_running = 0u;
            s_app_state  = APP_STATE_CLOCK;
            uart_printf("[countdown] exit\r\n");
            return;
        }
        return;
    }

    /* -- 闹钟设置模式 -- */
    if (s_app_state == APP_STATE_ALARM) {
        /* K1 短按: 小时 +1 */
        if (id == KEY_ID_1 && evt == KEY_EVT_SHORT) {
            s_alarm_hours = (s_alarm_hours + 1u) % 24u;
            alarm_update_display();
            return;
        }
        /* K2 短按: 开启/关闭闹钟 */
        if (id == KEY_ID_2 && evt == KEY_EVT_SHORT) {
            s_alarm_enabled = s_alarm_enabled ? 0u : 1u;
            alarm_update_display();
            uart_printf("[alarm] %s\r\n", s_alarm_enabled ? "ON" : "OFF");
            return;
        }
        /* K3 短按: 分钟 +1 */
        if (id == KEY_ID_3 && evt == KEY_EVT_SHORT) {
            s_alarm_minutes = (s_alarm_minutes + 1u) % 60u;
            alarm_update_display();
            return;
        }
        /* K4 长按: 保存并退出 */
        if (id == KEY_ID_4 && evt == KEY_EVT_LONG) {
            buzzer_beep_ms(30u);
            s_app_state = APP_STATE_CLOCK;
            uart_printf("[alarm] saved, %02u:%02u %s\r\n",
                        (unsigned)s_alarm_hours, (unsigned)s_alarm_minutes,
                        s_alarm_enabled ? "ON" : "OFF");
            return;
        }
        return;
    }

    /* 其余按键: 占位 */
    buzzer_beep_ms(30u);
    uart_printf("[key] K%u %s\r\n", (unsigned)id + 1u,
                (evt == KEY_EVT_LONG) ? "LONG" : "SHORT");
}

void app_init(void)
{
    led_init();
    buzzer_init();
    seg_display_init();
    key_init();
    key_set_callback(on_key);
    uart_console_init();
    clock_time_init();        /* 上电读 RTC 续表（首次上电 00:00:00） */

    uart_printf("\r\n[clock] STM32 digital clock v1.0, sysclk=%u Hz\r\n",
                (unsigned)SystemCoreClock);
    uart_printf("[clock] console commands: TODO\r\n");
}

static uint8_t s_melody_started = 0u;

void app_task(void)
{
    seg_display_task();
    key_task();
    buzzer_task();
    uart_console_task();
    led_apply(s_app_state);   /* 模式指示灯：状态无关，每圈幂等刷新 */

    /* 首次进入主循环时启动旋律 */
    if (!s_melody_started) {
        s_melody_started = 1u;
        melody_start_note();
        uart_printf("[melody] startup melody begin, %u notes\r\n",
                    (unsigned)STARTUP_MELODY_COUNT);
    }

    /* ---- MELODY: 推进音符 ---- */
    if (s_app_state == APP_STATE_MELODY) {
        uint32_t now = HAL_GetTick();
        if ((int32_t)(now - s_melody_tick) >= 0) {
            s_melody_idx = (s_melody_idx + 1u) % STARTUP_MELODY_COUNT;
            if (s_melody_idx == 0u && s_melody_once) {
                s_melody_once = 0u;
                s_app_state   = APP_STATE_CLOCK;
                return;
            }
            melody_start_note();
        }
        return;
    }

    /* ---- ADJUST: 调时界面（显示由按键回调实时刷新）---- */
    if (s_app_state == APP_STATE_ADJUST) {
        return;
    }

    /* ---- STOPWATCH: 秒表 ---- */
    if (s_app_state == APP_STATE_STOPWATCH) {
        if (s_sw_running) {
            uint32_t now = HAL_GetTick();
            s_sw_elapsed_ms += now - s_sw_last_tick;
            s_sw_last_tick   = now;
            stopwatch_update_display();
        }
        return;
    }

    /* ---- COUNTDOWN: 倒计时 ---- */
    if (s_app_state == APP_STATE_COUNTDOWN) {
        if (s_cd_running) {
            uint32_t now = HAL_GetTick();
            uint32_t delta = now - s_cd_last_tick;
            s_cd_last_tick = now;
            if (delta >= s_cd_remaining_ms) {
                s_cd_remaining_ms = 0u;
                s_cd_running      = 0u;
                countdown_update_display();
                s_app_state   = APP_STATE_MELODY;
                s_melody_idx  = 0u;
                s_melody_once = 1u;
                melody_start_note();
                uart_printf("[countdown] done, playing melody!\r\n");
            } else {
                s_cd_remaining_ms -= delta;
                countdown_update_display();
            }
        }
        return;
    }

    /* ---- ALARM: 设置界面保持显示，不进走时刷新 ---- */
    if (s_app_state == APP_STATE_ALARM) {
        return;
    }

    /* ---- CLOCK: 正常走时 ---- */
    if (clock_time_tick_second()) {
        clock_hms_t t;
        clock_time_get(&t);

        seg_display_set_hhmm(t.hours, t.minutes);
        seg_display_show_dp(1u, (t.seconds & 1u) ? 0u : 1u);

        uart_printf("[time] %02u:%02u:%02u\r\n",
                    (unsigned)t.hours, (unsigned)t.minutes, (unsigned)t.seconds);

        /* 闹钟触发检查 */
        if (s_alarm_enabled
            && t.hours == s_alarm_hours
            && t.minutes == s_alarm_minutes
            && t.seconds == 0u) {
            s_app_state   = APP_STATE_MELODY;
            s_melody_idx  = 0u;
            s_melody_once = 1u;
            melody_start_note();
            uart_printf("[alarm] ring! %02u:%02u\r\n",
                        (unsigned)s_alarm_hours, (unsigned)s_alarm_minutes);
        }
    }
}