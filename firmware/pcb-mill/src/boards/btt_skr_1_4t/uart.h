#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

void uart_init();
void uart_send_str(const char *s);

#endif  // __UART_H__