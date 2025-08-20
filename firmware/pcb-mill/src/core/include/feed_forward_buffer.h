#ifndef __FEED_FORWARD_BUFFER_H__
#define __FEED_FORWARD_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>

#include "macros.h"

typedef struct {
  const uint8_t* buffer;  // Buffer location
  uint16_t total_count;   // Total size to feed
  uint16_t fed_count;     // Bytes already fed (processed)
} feed_forward_buffer_t;

// If total_count >  fed_count → returns the number of bytes remaining.
// If total_count == fed_count → returns 0.
// If total_count <  fed_count → returns underflows (wraps around, since unsigned).
// Given feed_forward_remaining_count(buffer, max_buffer_size).
//    uint16_t remaining = buffer->total_count - buffer->fed_count;
//    - If remaining > 0 → you get the smaller of (remaining, max_buffer_size).
//    - If remaining == 0 → you get 0.
//    - If remaining underflows → you get some large number (65535−x), so the min will be max_buffer_size
ALWAYS_INLINE static uint16_t feed_forward_remaining_count(feed_forward_buffer_t* buffer, uint16_t max_buffer_size) {
  uint16_t remaining = buffer->total_count - buffer->fed_count;
  return (remaining < max_buffer_size) ? remaining : max_buffer_size;
}

#endif  // __FEED_FORWARD_BUFFER_H__