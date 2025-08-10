#include "circular_buffer.h"
#include <string.h>  // for memcpy

void circular_buffer_init(circular_buffer_t* cb, uint8_t* data_buffer, uint32_t size) {
  cb->buffer = data_buffer;
  cb->size = size;
  cb->head = 0;
  cb->tail = 0;
}

void circular_buffer_reset(circular_buffer_t* cb) {
  cb->head = 0;
  cb->tail = 0;
}

uint32_t circular_buffer_read(circular_buffer_t* cb, uint8_t* data_buffer, uint32_t max_len) {
  uint32_t available;
  if (cb->head >= cb->tail) {
    available = cb->head - cb->tail;
  } else {
    available = cb->size - cb->tail + cb->head;
  }
  if (max_len > available) max_len = available;

  uint32_t first_part = cb->size - cb->tail;
  if (first_part > max_len) first_part = max_len;

  memcpy(data_buffer, (const void*)(cb->buffer + cb->tail), first_part);

  uint32_t second_part = max_len - first_part;
  if (second_part > 0) {
    memcpy(data_buffer + first_part, (const void*)(cb->buffer), second_part);
  }

  cb->tail = (cb->tail + max_len) % cb->size;
  return max_len;
}

bool circular_buffer_write(circular_buffer_t* cb, const uint8_t* data_buffer, uint32_t write_len) {
  uint32_t capacity;
  if (cb->head >= cb->tail) {
    capacity = cb->size - cb->head + cb->tail;
  } else {
    capacity = cb->tail - cb->head;
  }
  if (write_len > capacity) write_len = capacity;

  uint32_t first_part = cb->size - cb->head;
  if (first_part > write_len) first_part = write_len;

  memcpy((void*)(cb->buffer + cb->head), data_buffer, first_part);

  uint32_t second_part = write_len - first_part;
  if (second_part > 0) {
    memcpy((void*)(cb->buffer), data_buffer + first_part, second_part);
  }

  cb->head = (cb->head + write_len) % cb->size;
  return write_len > 0;
}
