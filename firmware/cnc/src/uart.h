#ifndef UART_H
#define UART_H

void uart_init(uint32_t baud);
void uart_send_char(char c);
void uart_send_str(const char *s);

#endif
