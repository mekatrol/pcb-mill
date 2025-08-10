#include "circular_buffer.h"

void circular_buffer_init(circular_buffer_t* circular_buffer, uint8_t* data_buffer, uint32_t size) {
  circular_buffer->buffer = data_buffer;
  circular_buffer->size = size;
  circular_buffer->head = 0;
  circular_buffer->tail = 0;
  circular_buffer->count = 0;
  circular_buffer->overrun = false;
}

void circular_buffer_reset(circular_buffer_t* circular_buffer) {
  circular_buffer->head = 0;
  circular_buffer->tail = 0;
  circular_buffer->count = 0;
  circular_buffer->overrun = false;
}

uint32_t circular_buffer_available_capacity(circular_buffer_t* circular_buffer) {
  return circular_buffer->size - circular_buffer->count;
}

uint32_t circular_buffer_read(circular_buffer_t* circular_buffer, uint8_t* data_buffer, uint32_t max_len) {
  uint32_t i = 0;
  while (circular_buffer->tail != circular_buffer->head && i < max_len) {
    // Copy byte from circular buffer to data buffer
    data_buffer[i++] = circular_buffer->buffer[circular_buffer->tail++];

    // Reduce count
    circular_buffer->count--;

    // Wrap tail if past buffer size
    if (circular_buffer->tail >= circular_buffer->size) {
      circular_buffer->tail = 0;
    }
  }

  return i;
}

bool circular_buffer_write(circular_buffer_t* circular_buffer, const uint8_t* data_buffer, uint32_t write_len) {
  uint32_t i = 0;
  bool overrun = false;

  while (write_len > 0) {
    // If the head is already at tail and there is a buffered byte then we are going to overrun the buffer
    if (circular_buffer->head == circular_buffer->tail && circular_buffer->count > 0) {
      circular_buffer->overrun = true;
    }

    // Write to curcular buffer
    circular_buffer->buffer[circular_buffer->head++] = data_buffer[i++];

    // Increment buffered count
    circular_buffer->count++;

    // Decrease write length remaining
    write_len--;

    // Wrap head if past buffer size
    if (circular_buffer->head >= circular_buffer->size) {
      circular_buffer->head = 0;
    }
  }

  return overrun;
}
