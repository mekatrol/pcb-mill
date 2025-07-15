#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

#include "clock.h"
#include "memory_map.h"

#define BAUD_RATE 115200UL

void uart4_init(void);
void uart4_send(uint8_t b);

void tmc2209_read_gconf(uint8_t slave);
int tmc2209_parse_reply(uint8_t *data_out, uint8_t slave);

#endif // __UART_H__