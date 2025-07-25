#ifndef __LOG_UART_H__
#define __LOG_UART_H__

void uart_puts(const char *s);
void uart_printf(const char *fmt, ...);

#endif // __LOG_UART_H__