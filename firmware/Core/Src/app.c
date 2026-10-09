/**
 * @file    app.c
 * @brief   应用层：模块装配 + 时钟主逻辑（骨架默认行为 + TODO 清单）
 *
 * 骨架上电默认行为（用于先验证硬件）：
 *   - 数码管显示 HH:MM，冒号（第 2 位小数点）每秒闪烁
 *   - 串口 115200 打印横幅；秒变化时可选打印
 *   - 任意按键：短按/长按蜂鸣 30ms 并回显（引脚级自检）
 * TODO(应用)：设置模式（调时）、闹钟、整点报时、按键功能分配、LED 指示策略
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

static void on_key(key_id_t id, key_evt_t evt)
{
    /* TODO(应用): 按键功能分配。
     * 建议分工：KEY1=设置/确认  KEY2=切换字段(时→分)  KEY3=加  KEY4=减；
     * 长按 KEY1 进入设置模式，设置模式下数码管对应字段闪烁。
     * 下面是骨架占位：蜂鸣一声 + 串口回显，便于先验证按键硬件。 */
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
    clock_time_init();        /* 默认 12:00:00；TODO(应用): 后续改为读 RTC/备份寄存器 */

    uart_printf("\r\n[clock] STM32 digital clock v1.0, sysclk=%u Hz\r\n",
                (unsigned)SystemCoreClock);
    uart_printf("[clock] console commands: TODO\r\n");
}

void app_task(void)
{
    seg_display_task();
    key_task();
    buzzer_task();
    uart_console_task();

    /* TODO(应用): 设置模式下，数码管改为显示被编辑字段并闪烁 */

    if (clock_time_tick_second()) {
        clock_hms_t t;
        clock_time_get(&t);

        seg_display_set_hhmm(t.hours, t.minutes);
        seg_display_show_dp(1u, (t.seconds & 1u) ? 0u : 1u);   /* 冒号每秒闪烁 */

        /* TODO(应用): 整点报时（buzzer_beep_ms）、闹钟判断 */

        /* 心跳：LED1 每秒翻转（TODO(应用): 正式版可改成信号指示） */
        led_toggle(LED_1);

        /* 串口每秒回显时间（调试用；TODO(应用): 正式版可去掉） */
        uart_printf("[time] %02u:%02u:%02u\r\n",
                    (unsigned)t.hours, (unsigned)t.minutes, (unsigned)t.seconds);
    }
}