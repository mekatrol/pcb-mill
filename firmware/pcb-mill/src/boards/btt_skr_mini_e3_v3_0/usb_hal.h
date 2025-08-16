#ifndef __USB_HAL_H__
#define __USB_HAL_H__

#include "usb.h"

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

#define USB_EP_STATUS_MASK(dir) (3U << (USB_CHEP_TX_STTX_Pos + ((dir) == USB_EP_DIRECTION_IN_IDX ? 0 : 8)))
#define USB_EP_DATA_TOGGLE_MASK(dir) (1U << (USB_CHEP_DTOG_TX_Pos + ((dir) == USB_EP_DIRECTION_IN_IDX ? 0 : 8)))

#define USB_EP_TX_BUFFER 0
#define USB_EP_RX_BUFFER 1

typedef struct {
  uint8_t *buffer;
  uint16_t total_len;
  uint16_t queued_len;

  // ep0 this will be USB_EP0_BUFFER_SIZE
  // ep1 to ep7 this will be the pack size set in
  uint16_t max_packet_size;

  // Endpoint identifier (is zero based so can be used as index to arrays)
  uint8_t ep_idn;
} endpoint_packet_t;

// EP allocator
typedef struct {
  uint8_t ep_num;
  uint8_t ep_type;
  bool allocated[2];
} ep_alloc_t;

void usb_endpoint_set_rx_buffer_block_size(uint32_t endpoint_idn, uint32_t size);
void usb_transmit_packet(endpoint_packet_t *control_transfer, uint16_t ep_idn);

__attribute__((always_inline)) static inline void ep_reset_allocated_state(ep_alloc_t *endpoint_allocated_state, uint32_t ep_idn) {
  endpoint_allocated_state[ep_idn].ep_num = 0xFF;
  endpoint_allocated_state[ep_idn].ep_type = 0xFF;
  endpoint_allocated_state[ep_idn].allocated[USB_EP_DIRECTION_OUT_IDX] = false;
  endpoint_allocated_state[ep_idn].allocated[USB_EP_DIRECTION_IN_IDX] = false;
}

__attribute__((always_inline)) static inline uint32_t usb_endpoint_reg_get(uint32_t endpoint_idn) {
  return USB->chep[endpoint_idn].CHEPnR;
}

__attribute__((always_inline)) static inline uint32_t usb_endpoint_reg_get_preserve(uint32_t endpoint_idn) {
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

__attribute__((always_inline)) static inline void usb_endpoint_reg_set_preserve(uint32_t endpoint_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  // USB_EP_VTTX and USB_EP_VTRX are rc_w0 bits so setting them to 1 preserves the current register values
  // this will preserve  IN/OUT/SETUP transaction is successfully completed states
  USB->chep[endpoint_idn].CHEPnR = (value | USB_EP_VTTX | USB_EP_VTRX);

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline uint32_t unaligned_read32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

__attribute__((always_inline)) static inline void unaligned_write32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)(value);
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

__attribute__((always_inline)) static inline void usb_endpoint_reg_set_clear_ctr(uint32_t endpoint_idn, usb_endpoint_direction_index_t dir) {
  uint32_t endpoint_reg = USB->chep[endpoint_idn].CHEPnR;
  endpoint_reg |= USB_EP_VTTX | USB_EP_VTRX;
  endpoint_reg &= USB_CHEP_REG_MASK;
  endpoint_reg &= ~(1 << (USB_CHEP_VTTX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
  usb_endpoint_reg_set(endpoint_idn, endpoint_reg, false);
}

__attribute__((always_inline)) static inline void usb_endpoint_status(uint32_t *endpoint_reg, usb_endpoint_direction_index_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
}

__attribute__((always_inline)) static inline void usb_endpoint_data_toggle(uint32_t *endpoint_reg, usb_endpoint_direction_index_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
}

__attribute__((always_inline)) static inline uint32_t usb_pma_get_endpoint_addr(uint32_t endpoint_idn, uint8_t buf_id) {
  return USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr & 0x0000FFFFU;
}

__attribute__((always_inline)) static inline void usb_pma_set_endpoint_addr(uint32_t endpoint_idn, uint8_t buf_id, uint16_t addr) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & 0xFFFF0000U) | (addr & 0x0000FFFCU);
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
__attribute__((always_inline)) static inline uint32_t usb_endpoint_calc_rx_buffer_block_size(uint16_t buffer_size, uint32_t *blsize, uint32_t *num_block) {
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

__attribute__((always_inline)) static inline uint32_t usb_pma_next_addr(uint32_t usb_pma_next_available, uint32_t size) {
  // Get next available Packet Memory Area location
  uint32_t usb_pma_addr = usb_pma_next_available;

  // Update next available by adding size (size is assumed to be 32 bit aligned)
  usb_pma_next_available = (usb_pma_next_available + size);

  return usb_pma_addr;
}

#endif  // __USB_HAL_H__