#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

#include "clock.h"
#include "memory_map.h"

#define BAUD_RATE 115200UL

void uart2_init();
void uart2_send(uint8_t b);
uint8_t uart2_recv();

void uart4_init();
void uart4_send(uint8_t b);
uint8_t uart4_recv();

#endif // __UART_H__