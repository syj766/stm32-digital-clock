/**
 * @file    hw_test.h
 * @brief   硬件上电自检（bring-up test）——验证引脚/极性/驱动能力，与课设逻辑无关
 *
 * 测试内容（全程串口 115200 播报，总计约 9~19 秒）：
 *   1. LED1~4 流水灯 ×2        （验 PA4~PA7，低电平亮）
 *   2. 数码管全亮 "8."×4 1.5s  （12 个脚全拉，坏脚一眼看出）
 *   3. 逐位扫描 DIG1~4 ×2      （验位选/驱动管）
 *   4. 逐段扫描 SEG_A~DP       （逐脚验证，串口打印每段对应的引脚号）
 *   5. 蜂鸣器 3短1长+上行音阶  （验 TIM3 PWM 变调/音乐功能）
 *   6. 按键 K1~K4 逐个按       （10 秒超时；按下时对应 LED 亮 + 数码管显示该键号）
 *
 * 设计原则：除蜂鸣器外均直推 pinmap.h GPIO，不经过驱动模块；
 * PB0 已归 TIM3 PWM（AF 模式），蜂鸣器测试必须走 buzzer 驱动 API。
 * 这样故障可分层定位：测试通过但时钟显示异常 → 驱动代码问题；
 * 测试本身就不过 → 硬件/焊接问题。
 *
 * 用法：main() 里 CubeMX 的 MX_USART1_UART_Init() 之后调用（挂 USER CODE 2 即满足），
 *      测完自动返回进入正常应用。hw_test_run() 内部会调用 uart_console_init() 复位收包缓冲。
 */
#ifndef HW_TEST_H
#define HW_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

void hw_test_run(void);

#ifdef __cplusplus
}
#endif

#endif /* HW_TEST_H */