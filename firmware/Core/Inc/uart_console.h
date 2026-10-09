/**
 * @file    uart_console.h
 * @brief   USART1 调试串口（PA9=TX / PA10=RX，115200-8N1）
 *
 * USART1 由 CubeMX 激活（Asynchronous，115200-8N1，2026-10-06 完成），
 * 句柄 huart1 由 CubeMX 生成、定义在 main.c，main() 先执行
 * MX_USART1_UART_Init() 再进入应用，本模块直接使用该句柄。
 */
#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_console_init(void);
void uart_printf(const char *fmt, ...);   /* 支持 %c %s %d %u %x %%，%0Nd 补零 */

/* 收到的原始字节（主循环里 uart_console_task() 持续搬运到环形缓冲） */
uint8_t uart_console_pop(uint8_t *byte);  /* 1=取到一个字节，0=空 */
void uart_console_task(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_CONSOLE_H */