#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include <stdbool.h>

#define BAUD_RATE 115200UL

void uart2_init();
void uart_putc(uint8_t b);
uint8_t uart_getc();
bool uart_data_ready();
bool uart_can_send();

void uart4_init();
void uart4_send(uint8_t b);
uint8_t uart4_recv();

#endif // __UART_H__