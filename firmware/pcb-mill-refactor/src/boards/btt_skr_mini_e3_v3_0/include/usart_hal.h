#ifndef __SERIAL_BUFFER_H__
#define __SERIAL_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>
#include "board_hal.h"

typedef struct {
  volatile uint32_t baud_rate;
  volatile uint32_t rx_buffer_size;
  volatile uint32_t rx_buffer_head;
  volatile uint32_t rx_buffer_tail;
  volatile uint32_t rx_count;
  volatile uint32_t tx_buffer_size;
  volatile uint32_t tx_buffer_head;
  volatile uint32_t tx_buffer_tail;
  volatile uint8_t *tx_buffer;
  volatile uint8_t *rx_buffer;
  volatile void *hal;
} usart_instance_t;

void usart_init_hal(usart_instance_t *usart, GPIO_TypeDef *gpio, uint32_t rx_pin, uint32_t tx_pin, uint32_t usart_enable_bit, IRQn_Type irq_no);

int16_t usart_read(usart_instance_t *usart);
void usart_send(usart_instance_t *usart, uint8_t b);
void usart_wait_data_sent(usart_instance_t *usart);
void usart_irq_handler(usart_instance_t *usart);
bool usart_wait_for_count(usart_instance_t *usart, uint8_t count, uint32_t max_wait_ms);
void usart_clear_buffers(usart_instance_t *usart);

#endif  // __SERIAL_BUFFER_H__