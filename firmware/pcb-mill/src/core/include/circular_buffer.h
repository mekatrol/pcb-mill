#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  volatile uint8_t* buffer;
  volatile uint32_t size;
  volatile uint32_t head;
  volatile uint32_t tail;
  volatile uint32_t count;
  volatile bool overrun;
} circular_buffer_t;

void circular_buffer_init(circular_buffer_t* circular_buffer, uint8_t* data_buffer, uint32_t size);
void circular_buffer_reset(circular_buffer_t* circular_buffer);
uint32_t circular_buffer_available_capacity(circular_buffer_t* circular_buffer);
uint32_t circular_buffer_read(circular_buffer_t* circular_buffer, uint8_t* data_buffer, uint32_t max_len);
bool circular_buffer_write(circular_buffer_t* circular_buffer, const uint8_t* data_buffer, uint32_t write_len);

#endif