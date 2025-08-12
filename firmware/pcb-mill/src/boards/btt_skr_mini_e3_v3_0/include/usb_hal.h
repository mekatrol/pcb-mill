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

typedef struct {
  volatile uint32_t value;
} usb_pma_buf_t;

#define USB_PMA_BUF_AT(addr) ((usb_pma_buf_t*)(USB_DRD_PMAADDR + addr))

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

/* Aligned buffer size according to hardware */
__attribute__((always_inline)) static inline uint16_t pma_align_buffer_size(uint16_t size, uint8_t* blsize, uint8_t* num_block) {
  /* The STM32 full speed USB peripheral supports only a limited set of
   * buffer sizes given by the RX buffer entry format in the USB_BUFFER_DESC_TABLE. */
  uint16_t block_in_bytes;
  if (size > 62) {
    block_in_bytes = 32;
    *blsize = 1;
    *num_block = DIV_CEIL(size, 32);
  } else {
    block_in_bytes = 2;
    *blsize = 0;
    *num_block = DIV_CEIL(size, 2);
  }

  return (*num_block) * block_in_bytes;
}

__attribute__((always_inline)) static inline void usb_pma_set_rx_bufsize(uint32_t endpoint_idn, uint8_t buf_id, uint16_t wCount) {
  uint8_t blsize, num_block;
  (void)pma_align_buffer_size(wCount, &blsize, &num_block);

  /* Encode into register. When BLSIZE==1, we need to subtract 1 block count */
  uint16_t bl_nb = (blsize << 15) | ((num_block - blsize) << 10);
  if (bl_nb == 0) {
    // zlp but 0 is invalid value, set blsize to 1 (32 bytes)
    // Note: lower value can cause PMAOVR on setup with ch32v203
    bl_nb = 1 << 15;
  }

  uint32_t count_addr = USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr;
  count_addr = (bl_nb << 16) | (count_addr & 0x0000FFFFu);
  USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[buf_id].count_addr = count_addr;
}

#endif