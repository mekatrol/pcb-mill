/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _TUSB_COMMON_H_
#define _TUSB_COMMON_H_

//--------------------------------------------------------------------+
// Macros Helper
//--------------------------------------------------------------------+
#define TU_ARRAY_SIZE(_arr) (sizeof(_arr) / sizeof(_arr[0]))
#define TU_MIN(_x, _y) (((_x) < (_y)) ? (_x) : (_y))
#define TU_MAX(_x, _y) (((_x) > (_y)) ? (_x) : (_y))
#define TU_DIV_CEIL(n, d) (((n) + (d) - 1) / (d))

#define TU_U16(_high, _low) ((uint16_t)(((_high) << 8) | (_low)))
#define TU_U16_HIGH(_u16) ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define TU_U16_LOW(_u16) ((uint8_t)((_u16) & 0x00ff))
#define U16_TO_U8S_BE(_u16) TU_U16_HIGH(_u16), TU_U16_LOW(_u16)
#define U16_TO_U8S_LE(_u16) TU_U16_LOW(_u16), TU_U16_HIGH(_u16)

#define TU_U32_BYTE3(_u32) ((uint8_t)((((uint32_t)_u32) >> 24) & 0x000000ff))  // MSB
#define TU_U32_BYTE2(_u32) ((uint8_t)((((uint32_t)_u32) >> 16) & 0x000000ff))
#define TU_U32_BYTE1(_u32) ((uint8_t)((((uint32_t)_u32) >> 8) & 0x000000ff))
#define TU_U32_BYTE0(_u32) ((uint8_t)(((uint32_t)_u32) & 0x000000ff))  // LSB

#define U32_TO_U8S_BE(_u32) TU_U32_BYTE3(_u32), TU_U32_BYTE2(_u32), TU_U32_BYTE1(_u32), TU_U32_BYTE0(_u32)
#define U32_TO_U8S_LE(_u32) TU_U32_BYTE0(_u32), TU_U32_BYTE1(_u32), TU_U32_BYTE2(_u32), TU_U32_BYTE3(_u32)

#define TU_BIT(n) (1UL << (n))

// Generate a mask with bit from high (31) to low (0) set, e.g TU_GENMASK(3, 0) = 0b1111
#define TU_GENMASK(h, l) ((UINT32_MAX << (l)) & (UINT32_MAX >> (31 - (h))))

//--------------------------------------------------------------------+
// Includes
//--------------------------------------------------------------------+

// Standard Headers
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Tinyusb Common Headers
#include "tusb_option.h"
#include "tusb_types.h"

//--------------------------------------------------------------------+
// Internal Inline Functions
//--------------------------------------------------------------------+

// This is a backport of memcpy_s from c11
__attribute__((always_inline)) static inline int memcpy_s(void *dest, size_t destsz, const void *src, size_t count) {
  // TODO may check if desst and src is not NULL
  if (count > destsz) {
    return -1;
  }
  memcpy(dest, src, count);
  return 0;
}

__attribute__((always_inline)) static inline uint8_t tu_u32_byte3(uint32_t ui32) { return TU_U32_BYTE3(ui32); }
__attribute__((always_inline)) static inline uint8_t tu_u32_byte2(uint32_t ui32) { return TU_U32_BYTE2(ui32); }
__attribute__((always_inline)) static inline uint8_t tu_u32_byte1(uint32_t ui32) { return TU_U32_BYTE1(ui32); }
__attribute__((always_inline)) static inline uint8_t tu_u32_byte0(uint32_t ui32) { return TU_U32_BYTE0(ui32); }

__attribute__((always_inline)) static inline uint16_t tu_u32_high16(uint32_t ui32) { return (uint16_t)(ui32 >> 16); }
__attribute__((always_inline)) static inline uint16_t tu_u32_low16(uint32_t ui32) { return (uint16_t)(ui32 & 0x0000ffffu); }

__attribute__((always_inline)) static inline uint8_t tu_u16_high(uint16_t ui16) { return TU_U16_HIGH(ui16); }
__attribute__((always_inline)) static inline uint8_t tu_u16_low(uint16_t ui16) { return TU_U16_LOW(ui16); }

//------------- Bits -------------//
__attribute__((always_inline)) static inline uint32_t tu_bit_set(uint32_t value, uint8_t pos) { return value | TU_BIT(pos); }
__attribute__((always_inline)) static inline uint32_t tu_bit_clear(uint32_t value, uint8_t pos) { return value & (~TU_BIT(pos)); }
__attribute__((always_inline)) static inline bool tu_bit_test(uint32_t value, uint8_t pos) { return (value & TU_BIT(pos)) ? true : false; }

//------------- Min -------------//
__attribute__((always_inline)) static inline uint8_t tu_min8(uint8_t x, uint8_t y) { return (x < y) ? x : y; }
__attribute__((always_inline)) static inline uint16_t tu_min16(uint16_t x, uint16_t y) { return (x < y) ? x : y; }
__attribute__((always_inline)) static inline uint32_t tu_min32(uint32_t x, uint32_t y) { return (x < y) ? x : y; }

//------------- Max -------------//
__attribute__((always_inline)) static inline uint8_t tu_max8(uint8_t x, uint8_t y) { return (x > y) ? x : y; }
__attribute__((always_inline)) static inline uint16_t tu_max16(uint16_t x, uint16_t y) { return (x > y) ? x : y; }
__attribute__((always_inline)) static inline uint32_t tu_max32(uint32_t x, uint32_t y) { return (x > y) ? x : y; }

//------------- Align -------------//
__attribute__((always_inline)) static inline uint32_t tu_align(uint32_t value, uint32_t alignment) {
  return value & ((uint32_t)~(alignment - 1));
}

__attribute__((always_inline)) static inline uint32_t tu_align4(uint32_t value) { return (value & 0xFFFFFFFCUL); }
__attribute__((always_inline)) static inline uint32_t tu_align8(uint32_t value) { return (value & 0xFFFFFFF8UL); }
__attribute__((always_inline)) static inline uint32_t tu_align16(uint32_t value) { return (value & 0xFFFFFFF0UL); }
__attribute__((always_inline)) static inline uint32_t tu_align32(uint32_t value) { return (value & 0xFFFFFFE0UL); }
__attribute__((always_inline)) static inline uint32_t tu_align4k(uint32_t value) { return (value & 0xFFFFF000UL); }
__attribute__((always_inline)) static inline uint32_t tu_offset4k(uint32_t value) { return (value & 0xFFFUL); }

__attribute__((always_inline)) static inline bool tu_is_aligned32(uint32_t value) { return (value & 0x1FUL) == 0; }
__attribute__((always_inline)) static inline bool tu_is_aligned64(uint64_t value) { return (value & 0x3FUL) == 0; }

//------------- Mathematics -------------//
__attribute__((always_inline)) static inline uint32_t tu_div_ceil(uint32_t v, uint32_t d) { return TU_DIV_CEIL(v, d); }
__attribute__((always_inline)) static inline uint32_t tu_round_up(uint32_t v, uint32_t f) { return tu_div_ceil(v, f) * f; }

// log2 of a value is its MSB's position
// TODO use clz TODO remove
__attribute__((always_inline)) static inline uint8_t tu_log2(uint32_t value) {
  uint8_t result = 0;
  while (value >>= 1) {
    result++;
  }
  return result;
}

__attribute__((always_inline)) static inline bool tu_is_power_of_two(uint32_t value) {
  return (value != 0) && ((value & (value - 1)) == 0);
}

// Rely on compiler to generate correct code for unaligned access
typedef struct {
  uint16_t val;
} __attribute__((packed)) tu_unaligned_uint16_t;
typedef struct {
  uint32_t val;
} __attribute__((packed)) tu_unaligned_uint32_t;

__attribute__((always_inline)) static inline uint32_t tu_unaligned_read32(const void *mem) {
  tu_unaligned_uint32_t const *ua32 = (tu_unaligned_uint32_t const *)mem;
  return ua32->val;
}

__attribute__((always_inline)) static inline void tu_unaligned_write32(void *mem, uint32_t value) {
  tu_unaligned_uint32_t *ua32 = (tu_unaligned_uint32_t *)mem;
  ua32->val = value;
}

__attribute__((always_inline)) static inline uint16_t tu_unaligned_read16(const void *mem) {
  tu_unaligned_uint16_t const *ua16 = (tu_unaligned_uint16_t const *)mem;
  return ua16->val;
}

__attribute__((always_inline)) static inline void tu_unaligned_write16(void *mem, uint16_t value) {
  tu_unaligned_uint16_t *ua16 = (tu_unaligned_uint16_t *)mem;
  ua16->val = value;
}

#endif /* _TUSB_COMMON_H_ */
