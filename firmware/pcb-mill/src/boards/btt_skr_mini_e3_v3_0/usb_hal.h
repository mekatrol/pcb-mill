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

void usb_endpoint_reset();
void usb_endpoint_set_rx_buffer_block_size(uint32_t ep_idn, uint32_t size);
void usb_transmit_packet(endpoint_packet_t *control_transfer, uint16_t ep_idn);
bool usb_control_transfer(uint8_t ep_addr, uint32_t transferred_bytes);
bool usb_cdc_transfer(uint8_t ep_addr, uint32_t transferred_bytes);
bool usb_read_packet_data(void *__restrict dst, uint16_t src, uint16_t byte_count);
void usb_endpoint_ctr_rx(uint32_t ep_idn);
void usb_endpoint_ctr_tx(uint32_t ep_idn);

__attribute__((always_inline)) static inline uint16_t usb_pma_get_count(uint32_t ep_idn, uint8_t buf_id) {
  uint16_t count;
  count = (USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

__attribute__((always_inline)) static inline uint32_t usb_pma_get_endpoint_addr(uint32_t ep_idn, uint8_t buf_id) {
  return USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[buf_id].count_addr & 0x0000FFFFU;
}

__attribute__((always_inline)) static inline uint32_t usb_endpoint_reg_get(uint32_t ep_idn) {
  return USB->chep[ep_idn].CHEPnR;
}

__attribute__((always_inline)) static inline void usb_endpoint_reg_set(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  USB->chep[ep_idn].CHEPnR = value;

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void usb_endpoint_reg_set_preserve(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
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

__attribute__((always_inline)) static inline void usb_endpoint_status(uint32_t *endpoint_reg, usb_endpoint_direction_index_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
}

#endif  // __USB_HAL_H__