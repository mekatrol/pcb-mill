#include "usb_hal.h"

extern endpoint_packet_t transfer_buffer_state[USB_EP_MAX][2];
extern ep_alloc_t endpoint_allocated_state[USB_EP_MAX];
extern uint16_t usb_pma_next_available;

uint8_t usb_endpoint_allocate(uint8_t ep_addr, uint8_t endpoint_type);

void usb_endpoint0_init() {
  usb_endpoint_allocate(USB_DIR_OUT, USB_EP_TYPE_CONTROL);
  usb_endpoint_allocate(USB_DIR_IN, USB_EP_TYPE_CONTROL);

  transfer_buffer_state[0][USB_EP_DIRECTION_OUT_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  transfer_buffer_state[0][USB_EP_DIRECTION_OUT_IDX].ep_idn = 0;

  transfer_buffer_state[0][USB_EP_DIRECTION_IN_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  transfer_buffer_state[0][USB_EP_DIRECTION_IN_IDX].ep_idn = 0;

  uint16_t pma_rx_addr = usb_pma_next_addr(usb_pma_next_available, USB_EP0_BUFFER_SIZE);
  uint16_t pma_tx_addr = usb_pma_next_addr(usb_pma_next_available, USB_EP0_BUFFER_SIZE);

  usb_pma_set_endpoint_addr(0, USB_EP_RX_BUFFER, pma_rx_addr);
  usb_pma_set_endpoint_addr(0, USB_EP_TX_BUFFER, pma_tx_addr);

  uint32_t endpoint_reg = usb_endpoint_reg_get(0) & ~USB_CHEP_REG_MASK;
  endpoint_reg |= USB_EP_CONTROL;
  usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_IN_IDX, USB_EP_STATE_NAK);
  usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_OUT_IDX, USB_EP_STATE_NAK);

  usb_endpoint_set_rx_buffer_block_size(0, sizeof(usb_control_request_t));
  usb_endpoint_reg_set(0, endpoint_reg, false);
}