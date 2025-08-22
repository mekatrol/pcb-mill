#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>

#include "macros.h"

typedef struct {
  volatile uint8_t* buffer;
  volatile uint32_t size;
  volatile uint32_t head;
  volatile uint32_t tail;
} circular_buffer_t;

ALWAYS_INLINE static uint32_t circular_buffer_count(const circular_buffer_t* cb) {
  if (cb->head >= cb->tail) {
    return cb->head - cb->tail;
  } else {
    return cb->size - cb->tail + cb->head;
  }
}

ALWAYS_INLINE static uint32_t circular_buffer_space(const circular_buffer_t* cb) {
  return cb->size - circular_buffer_count(cb);
}

void circular_buffer_init(circular_buffer_t* cb, uint8_t* data_buffer, uint32_t size);
void circular_buffer_reset(circular_buffer_t* cb);
uint32_t circular_buffer_read(circular_buffer_t* cb, uint8_t* data_buffer, uint32_t max_len);
bool circular_buffer_write(circular_buffer_t* cb, const uint8_t* data_buffer, uint32_t write_len);

#endif  // __CIRCULAR_BUFFER_H__