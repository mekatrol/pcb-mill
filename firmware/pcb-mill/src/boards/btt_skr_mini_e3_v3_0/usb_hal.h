#ifndef __USB_HAL_H__
#define __USB_HAL_H__

#include "usb.h"

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

#define USB_EP_STATUS_MASK(dir) (3U << (USB_CHEP_TX_STTX_Pos + ((dir) == USB_EP_DIRECTION_IN_IDX ? 0 : 8)))
#define USB_EP_DATA_TOGGLE_MASK(dir) (1U << (USB_CHEP_DTOG_TX_Pos + ((dir) == USB_EP_DIRECTION_IN_IDX ? 0 : 8)))

#define USB_EP_TX_BUFFER 0
#define USB_EP_RX_BUFFER 1

void usb_ep_reset();
void usb_ep_set_rx_buffer_block_size(uint32_t ep_idn, uint32_t size);
void usb_ep_control_init();
void usb_ep_rx(uint32_t ep_idn);
void usb_ep_tx_queued_bytes(uint32_t ep_idn);

bool usb_control_transfer(uint8_t ep_addr, uint32_t transferred_bytes);
bool usb_cdc_transfer(uint8_t ep_addr, uint32_t transferred_bytes);

bool usb_rx_packet(void *__restrict dst, uint16_t src, uint16_t byte_count);

__attribute__((always_inline)) static inline uint16_t usb_pma_get_count(uint32_t ep_idn, uint8_t buf_id) {
  uint16_t count;
  count = (USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

__attribute__((always_inline)) static inline uint32_t usb_pma_get_ep_addr(uint32_t ep_idn, uint8_t buf_id) {
  return USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr & 0x0000FFFFU;
}

__attribute__((always_inline)) static inline void usb_ep_reg_set(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  USB->chep[ep_idn].CHEPnR = value;

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void usb_ep_reg_set_preserve(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  // USB_EP_VTTX and USB_EP_VTRX are rc_w0 bits so setting them to 1 preserves the current register values
  // this will preserve  IN/OUT/SETUP transaction is successfully completed states
  USB->chep[ep_idn].CHEPnR = (value | USB_EP_VTTX | USB_EP_VTRX);

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void usb_ep_status(uint32_t *ep_reg, usb_ep_direction_index_t dir, usb_ep_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in ep_reg
  *ep_reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
}

#endif  // __USB_HAL_H__