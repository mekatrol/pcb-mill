#ifndef __LOG_UART_H__
#define __LOG_UART_H__

void diag_send(uint8_t b);
void diag_print(const char *s);
void diag_printf(const char *fmt, ...);

#endif  // __LOG_UART_H__