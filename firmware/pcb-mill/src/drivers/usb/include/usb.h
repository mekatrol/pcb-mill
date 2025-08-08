#ifndef __USB_H__
#define __USB_H__

// Standard Headers
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "stm32g0xx.h"

// USB registers strong type
#define USB ((USB_DRD_TypeDef *)USB_BASE)

// The number of endpoints the device supports
#define USB_ENDPOINT_MAX 8

// The size of endpoint 0 buffer
#define USB_EP0_BUFFER_SIZE 64

// The size of other endpoint buffers (e.g. CDC)
#define USB_ENDPOINT_RX_BUFFER_SIZE 64
#define USB_ENDPOINT_TX_BUFFER_SIZE 64

#define CFG_TUD_INTERFACE_MAX 16

// Size of array based on total memory size dived by size of single element
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// Ceiling division
//  DIV_CEIL(5, 2) => ((5 + 2 - 1) / 2) => (6 / 2) => 3
//  DIV_CEIL(9, 4) => ((9 + 4 - 1) / 4) => (12 / 4) => 3
//  DIV_CEIL(8, 4) => ((8 + 4 - 1) / 4) => (11 / 4) => 2
#define DIV_CEIL(n, d) (((n) + (d) - 1) / (d))

// Bit mask based on bit position
#define BIT_MASK(n) (1UL << (n))

#define MIN(_x, _y) (((_x) < (_y)) ? (_x) : (_y))

enum {
  ENDPOINT_TX_BUFFER = 0,
  ENDPOINT_RX_BUFFER = 1
};

// This is the same as USB_DRD_PMABuffDescTypeDef
typedef struct
{
  union {
    struct {
      volatile uint32_t count_addr;
    } buffer[2];

    volatile uint32_t TXBD;
    volatile uint32_t RXBD;
  };
} usbram_register_t;

// Buffer Table is located in Packet Memory Area (PMA)
typedef struct {
  usbram_register_t endpoint[USB_ENDPOINT_MAX];
} usbram_register_map_t;

#define USBRAM_REGSITER ((volatile usbram_register_map_t *)(USB_DRD_PMAADDR))

__attribute__((always_inline)) static inline bool bit_set_test(uint32_t value, uint32_t pos) { return (value & BIT_MASK(pos)) ? true : false; }
__attribute__((always_inline)) static inline uint16_t min_u16(uint16_t x, uint16_t y) { return (x < y) ? x : y; }

// Get high or low byte
#define U16_HIGH(_u16) ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16) ((uint8_t)((_u16) & 0x00ff))

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

bool usb_init_driver();

#endif  // __USB_H__