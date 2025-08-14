#ifndef __USB_HAL_H__
#define __USB_HAL_H__

#include "board_hal.h"
#include "usb_hal.h"
#include "usb.h"

__attribute__((always_inline)) static inline uint32_t usb_endpoint_reg_get(uint32_t endpoint_idn) {
  return USB->chep[endpoint_idn].CHEPnR;
}

__attribute__((always_inline)) static inline void usb_endpoint_reg_set(uint32_t endpoint_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  USB->chep[endpoint_idn].CHEPnR = value;

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void usb_endpoint_reg_set_clear_ctr(uint32_t endpoint_idn, usb_endpoint_direction_t dir) {
  uint32_t endpoint_reg = USB->chep[endpoint_idn].CHEPnR;
  endpoint_reg |= USB_EP_VTTX | USB_EP_VTRX;
  endpoint_reg &= USB_CHEP_REG_MASK;
  endpoint_reg &= ~(1 << (USB_CHEP_VTTX_Pos + (dir == USB_ENDPOINT_DIRECTION_IN ? 0 : 8)));
  usb_endpoint_reg_set(endpoint_idn, endpoint_reg, false);
}

__attribute__((always_inline)) static inline void usb_endpoint_status(uint32_t* endpoint_reg, usb_endpoint_direction_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == USB_ENDPOINT_DIRECTION_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline void usb_endpoint_data_toggle(uint32_t* endpoint_reg, usb_endpoint_direction_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (dir == USB_ENDPOINT_DIRECTION_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline uint32_t usb_pma_get_addr(uint32_t endpoint_idn, uint8_t buf_id) {
  return USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr & 0x0000FFFFu;
}

__attribute__((always_inline)) static inline void usb_pma_set_addr(uint32_t endpoint_idn, uint8_t buf_id, uint16_t addr) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & 0xFFFF0000u) | (addr & 0x0000FFFCu);
  USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline uint16_t usb_pma_get_count(uint32_t endpoint_idn, uint8_t buf_id) {
  uint16_t count;
  count = (USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

__attribute__((always_inline)) static inline void usb_pma_set_count(uint32_t endpoint_idn, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr = count_addr;
}

// Bit 31 BLSIZE: Block size
// This bit selects the size of memory block used to define the allocated buffer area.
//
// – If BLSIZE = 0, the memory block is 2-byte large, which is the minimum block
//   allowed in a half-word wide memory. With this block size the allocated buffer size
//   ranges from 2 to 62 bytes.
//
// – If BLSIZE = 1, the memory block is 32-byte large, which permits to reach the
//   maximum packet length defined by USB specifications. With this block size the
//   allocated buffer size theoretically ranges from 32 to 1024 bytes, which is the longest
//   packet size allowed by USB standard specifications. However, the applicable size is
//   limited by the available buffer memory
//
// Bits 30:26 NUM_BLOCK[4:0]: Number of blocks
// These bits define the number of memory blocks allocated to this packet buffer. The actual
// amount of allocated memory depends on the BLSIZE value as illustrated in RM0444 Table 239.
__attribute__((always_inline)) static inline uint32_t usb_endpoint_calc_rx_buffer_block_size(uint16_t buffer_size, uint32_t* blsize, uint32_t* num_block) {
  uint32_t block_size_log2;  // log2(block_size)

  if (buffer_size > 62) {
    block_size_log2 = 5;  // 32 bytes
    *blsize = 1;
  } else {
    block_size_log2 = 1;  // 2 bytes
    *blsize = 0;
  }

  // Same as:
  // block_count = (buffer_size + (32 - 1)) / 32 --> buffer_size  > 62
  // block_count = (buffer_size + ( 2 - 1)) /  2 --> buffer_size <= 62
  uint8_t block_count = (buffer_size + ((1 << block_size_log2) - 1)) >> block_size_log2;

  // if BLSIZE == 1 then we need to subtract 1 from num_block
  // See: RM0444 Table 239. Definition of allocated buffer memory
  // Easiest way is to just subtract BLSIZE from NUM_BLOCK
  *num_block = block_count - *blsize;

  // Same as:
  // block_count * 32 --> buffer_size  > 62
  // block_count *  2 --> buffer_size <= 62
  return block_count << block_size_log2;
}

void usb_endpoint_set_rx_buffer_block_size(uint32_t endpoint_idn, uint32_t size);

#endif