/**
 * @file    uart_console.c
 * @brief   USART1 调试串口（轻量 printf + 轮询收包）
 *
 * 前提：CubeMX 已激活 USART1（Asynchronous，115200-8N1）——2026-10-06 已激活。
 * main() 里 MX_USART1_UART_Init() 先于 USER CODE 2（app_init）执行，
 * 句柄 huart1 由 CubeMX 生成、定义在 main.c，本模块直接引用。
 * （自带初始化是"外设未激活"时期的过渡方案，已随 CubeMX 激活移除。）
 */
#include "uart_console.h"
#include "stm32f1xx_hal.h"
#include <stdarg.h>

extern UART_HandleTypeDef huart1;   /* CubeMX 生成，定义于 main.c */

#define UART_RX_BUF_SIZE   32u

static volatile uint8_t s_rx[UART_RX_BUF_SIZE];
static volatile uint8_t s_rx_head;
static volatile uint8_t s_rx_tail;

void uart_console_init(void)
{
    /* UART 外设本身由 CubeMX 生成的 MX_USART1_UART_Init() 负责初始化，
     * 本函数只复位接收环形缓冲（在 app_init 里调用，保证状态干净）。 */
    s_rx_head = 0u;
    s_rx_tail = 0u;
}

/* ---- 轻量 printf：%c %s %d %u %x %%，支持 %0Nd（N≤8）补零 ---- */
static void uart_put(const char *s, uint32_t n)
{
    (void)HAL_UART_Transmit(&huart1, (const uint8_t *)s, (uint16_t)n, 100u);
}

static uint32_t emit_padded(char *digits, uint32_t ndig, uint8_t neg,
                            uint8_t width, uint8_t zero_pad)
{
    char tmp[16];
    uint32_t n = 0u;

    if (neg) {
        tmp[n++] = '-';
    }
    if (zero_pad && (width > (ndig + neg))) {
        for (uint32_t i = 0u; i < (width - ndig - neg); i++) {
            tmp[n++] = '0';
        }
    }
    while (ndig > 0u) {
        tmp[n++] = digits[--ndig];
    }
    uart_put(tmp, n);
    return n;
}

void uart_printf(const char *fmt, ...)
{
    char digits[10];
    va_list ap;

    va_start(ap, fmt);
    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            uart_put(p, 1u);
            continue;
        }
        p++;

        uint8_t zero_pad = 0u, width = 0u;
        if (*p == '0') {
            zero_pad = 1u;
            p++;
        }
        while ((*p >= '0') && (*p <= '9')) {
            width = (uint8_t)(width * 10u + (uint8_t)(*p - '0'));
            p++;
        }

        switch (*p) {
        case 'd': {
            int sv = va_arg(ap, int);
            uint32_t v = (sv < 0) ? (uint32_t)(-sv) : (uint32_t)sv;
            uint32_t nd = 0u;
            do {
                digits[nd++] = (char)('0' + (v % 10u));
                v /= 10u;
            } while (v != 0u);
            (void)emit_padded(digits, nd, (sv < 0) ? 1u : 0u, width, zero_pad);
            break;
        }
        case 'u': {
            uint32_t v = va_arg(ap, uint32_t);
            uint32_t nd = 0u;
            do {
                digits[nd++] = (char)('0' + (v % 10u));
                v /= 10u;
            } while (v != 0u);
            (void)emit_padded(digits, nd, 0u, width, zero_pad);
            break;
        }
        case 'x': {
            uint32_t v = va_arg(ap, uint32_t);
            uint32_t nd = 0u;
            do {
                uint32_t d = v & 0xFu;
                digits[nd++] = (char)((d < 10u) ? ('0' + d) : ('a' + d - 10u));
                v >>= 4u;
            } while (v != 0u);
            (void)emit_padded(digits, nd, 0u, width, zero_pad);
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            uart_put(&c, 1u);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == 0) {
                s = "(null)";
            }
            uint32_t n = 0u;
            while (s[n] != '\0') {
                n++;
            }
            uart_put(s, n);
            break;
        }
        case '%': {
            uart_put("%", 1u);
            break;
        }
        default: {
            uart_put("%", 1u);
            uart_put(p, 1u);
            break;
        }
        }
    }
    va_end(ap);
}

/* ---- 收包：轮询搬运到环形缓冲，应用层用 uart_console_pop() 取 ---- */
uint8_t uart_console_pop(uint8_t *byte)
{
    if (byte == 0 || s_rx_head == s_rx_tail) {
        return 0u;
    }
    s_rx_tail = (uint8_t)((s_rx_tail + 1u) % UART_RX_BUF_SIZE);
    *byte = s_rx[s_rx_tail];
    return 1u;
}

void uart_console_task(void)
{
    uint8_t b;

    while (HAL_UART_Receive(&huart1, &b, 1u, 0u) == HAL_OK) {
        uint8_t next = (uint8_t)((s_rx_head + 1u) % UART_RX_BUF_SIZE);
        if (next != s_rx_tail) {            /* 满则丢弃 */
            s_rx_head = next;
            s_rx[next] = b;
        }
    }
    /* TODO(应用): 基于收到的字节实现命令解析（调时/闹钟设置等） */
}