#ifndef __SERIAL_BUFFER_H__
#define __SERIAL_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>
#include "core.h"

typedef struct {
  volatile USART_TypeDef *uart;
  volatile uint8_t *rx_buffer;
  volatile uint32_t rx_buffer_size;
  volatile uint32_t rx_buffer_head;
  volatile uint32_t rx_buffer_tail;
  volatile uint32_t rx_count;
  volatile uint8_t *tx_buffer;
  volatile uint32_t tx_buffer_size;
  volatile uint32_t tx_buffer_head;
  volatile uint32_t tx_buffer_tail;
} serial_buffer_t;

int16_t serial_uart_get(serial_buffer_t *buffer);
void serial_uart_send(serial_buffer_t *buffer, uint8_t b);
void serial_wait_data_sent(serial_buffer_t *buffer);
void serial_uart_irq_handler(serial_buffer_t *buffer);
bool serial_wait_for_count(serial_buffer_t *buffer, uint8_t count, uint32_t max_wait_ms);
void serial_clear_buffers(serial_buffer_t *buffer);

#endif  // __SERIAL_BUFFER_H__