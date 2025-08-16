#include "board_hal.h"
#include "usb_hal.h"

// EP0 identifier
#define EP0_IDN 0

extern endpoint_packet_t transfer_buffer_state[USB_EP_MAX][2];
extern ep_alloc_t endpoint_allocated_state[USB_EP_MAX];
extern uint16_t usb_pma_next_available;

uint8_t usb_endpoint_allocate(uint8_t ep_addr, uint8_t endpoint_type);

__attribute__((always_inline)) static inline endpoint_packet_t *endpoint_buffer_state(uint8_t ep_num, uint8_t dir) {
  return &transfer_buffer_state[ep_num][dir];
}

void usb_endpoint_control_init() {
  usb_endpoint_allocate(USB_DIR_OUT, USB_EP_TYPE_CONTROL);
  usb_endpoint_allocate(USB_DIR_IN, USB_EP_TYPE_CONTROL);

  transfer_buffer_state[EP0_IDN][USB_EP_DIRECTION_OUT_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  transfer_buffer_state[EP0_IDN][USB_EP_DIRECTION_OUT_IDX].ep_idn = EP0_IDN;

  transfer_buffer_state[EP0_IDN][USB_EP_DIRECTION_IN_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  transfer_buffer_state[EP0_IDN][USB_EP_DIRECTION_IN_IDX].ep_idn = EP0_IDN;

  uint16_t pma_rx_addr = usb_pma_next_addr(usb_pma_next_available, USB_EP0_BUFFER_SIZE);
  uint16_t pma_tx_addr = usb_pma_next_addr(usb_pma_next_available, USB_EP0_BUFFER_SIZE);

  usb_pma_set_endpoint_addr(EP0_IDN, USB_EP_RX_BUFFER, pma_rx_addr);
  usb_pma_set_endpoint_addr(EP0_IDN, USB_EP_TX_BUFFER, pma_tx_addr);

  uint32_t endpoint_reg = usb_endpoint_reg_get(EP0_IDN) & ~USB_CHEP_REG_MASK;
  endpoint_reg |= USB_EP_CONTROL;
  usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_IN_IDX, USB_EP_STATE_NAK);
  usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_OUT_IDX, USB_EP_STATE_NAK);

  usb_endpoint_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
  usb_endpoint_reg_set(EP0_IDN, endpoint_reg, false);
}

void usb_endpoint_control_status_complete(const usb_control_request_t *request) {
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  if (request_recipient == USB_REQUEST_RECIPIENT_DEVICE &&
      request_type == USB_REQUEST_TYPE_STANDARD &&
      request->bRequest == USB_STD_SET_ADDRESS) {
    uint8_t const dev_addr = (uint8_t)request->wValue;
    USB->DADDR = (USB_DADDR_EF | dev_addr);
  }

  usb_endpoint_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
}

void usb_endpoint_set_rx_buffer_block_size(uint32_t endpoint_idn, uint32_t size) {
  uint32_t blsize, num_block;
  usb_endpoint_calc_rx_buffer_block_size(size, &blsize, &num_block);

  // Merge BLSIZE and NUM_BLOCK and shift to correct bit positions
  uint32_t memory_buffer_allocation = (blsize << BIT_31_POS) | (num_block << BIT_26_POS);

  // Get existing register value (we don't want to override ADDR_RX), note this clears COUNT_RX
  // which is valid because we are setting the buffer size and previous received data likely invalid
  uint32_t usb_chep_txrxbd_n = USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[USB_EP_RX_BUFFER].count_addr;

  // Merge BLSIZE, NUM_BLOCK and ADDR_RX
  usb_chep_txrxbd_n = memory_buffer_allocation | (usb_chep_txrxbd_n & 0x0000FFFFU);

  // Update register
  USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[USB_EP_RX_BUFFER].count_addr = usb_chep_txrxbd_n;
}

uint8_t usb_endpoint_allocate(uint8_t ep_addr, uint8_t endpoint_type) {
  const uint8_t ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  for (uint8_t i = 0; i < USB_EP_MAX; i++) {
    // Check if already allocated
    if (endpoint_allocated_state[i].allocated[ep_dir_idx] &&
        endpoint_allocated_state[i].ep_type == endpoint_type &&
        endpoint_allocated_state[i].ep_num == ep_num) {
      return i;
    }

    // If EP of current direction is not allocated
    if (!endpoint_allocated_state[i].allocated[ep_dir_idx]) {
      // Check if EP number is the same
      if (endpoint_allocated_state[i].ep_num == 0xFF || endpoint_allocated_state[i].ep_num == ep_num) {
        // One EP pair has to be the same type
        if (endpoint_allocated_state[i].ep_type == 0xFF || endpoint_allocated_state[i].ep_type == endpoint_type) {
          endpoint_allocated_state[i].ep_num = ep_num;
          endpoint_allocated_state[i].ep_type = endpoint_type;
          endpoint_allocated_state[i].allocated[ep_dir_idx] = true;

          return i;
        }
      }
    }
  }

  // Allocation failed
  return 0;
}

bool usb_endpoint_open(const usb_endpoint_descriptor_t *endpoint_descriptor) {
  uint8_t const ep_addr = endpoint_descriptor->bEndpointAddress;
  const uint8_t ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  const uint32_t packet_size = USB_EP_PACKET_SIZE(endpoint_descriptor->wMaxPacketSize);
  uint8_t const endpoint_idn = usb_endpoint_allocate(ep_addr, endpoint_descriptor->bmAttributes.type);

  if (endpoint_idn >= USB_EP_MAX) {
    return false;
  }

  uint32_t endpoint_reg = usb_endpoint_reg_get(endpoint_idn) & ~USB_CHEP_REG_MASK;
  endpoint_reg |= USB_EP_NUM(ep_addr);

  // Supported endpoint types
  switch (endpoint_descriptor->bmAttributes.type) {
    case USB_EP_TYPE_BULK:
      endpoint_reg |= USB_EP_BULK;
      break;

    case USB_EP_TYPE_INTERRUPT:
      endpoint_reg |= USB_EP_INTERRUPT;
      break;

    default:
      // End type is not supported
      return false;
  }

  /* Create a packet memory buffer area. */
  uint16_t pma_addr = usb_pma_next_addr(usb_pma_next_available, packet_size);
  usb_pma_set_endpoint_addr(endpoint_idn, ep_dir_idx == USB_EP_DIRECTION_IN_IDX ? USB_EP_TX_BUFFER : USB_EP_RX_BUFFER, pma_addr);

  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_num, ep_dir_idx);
  control_transfer->max_packet_size = packet_size;
  control_transfer->ep_idn = endpoint_idn;

  usb_endpoint_status(&endpoint_reg, ep_dir_idx, USB_EP_STATE_NAK);
  usb_endpoint_data_toggle(&endpoint_reg, ep_dir_idx, 0);

  // reserve other direction toggle bits
  if (ep_dir_idx == USB_EP_DIRECTION_IN_IDX) {
    endpoint_reg &= ~(USB_CH_RX_VALID | USB_EP_DTOG_RX);
  } else {
    endpoint_reg &= ~(USB_CHEP_TX_STTX_Msk | USB_EP_DTOG_TX);
  }

  usb_endpoint_reg_set_preserve(endpoint_idn, endpoint_reg, true);

  return true;
}

void usb_endpoint_close_all() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

  for (uint32_t i = 1; i < USB_EP_MAX; i++) {
    usb_endpoint_reg_set(i, 0, false);
    ep_reset_allocated_state(endpoint_allocated_state, i);
  }

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  // Reset PMA allocation
  usb_pma_next_available = 8 * USB_EP_MAX + 2 * USB_EP0_BUFFER_SIZE;
}

bool usb_endpoint_transfer_hal(uint8_t ep_num, uint8_t ep_dir_idx, uint8_t *buffer, uint16_t total_bytes) {
  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_num, ep_dir_idx);

  control_transfer->buffer = buffer;
  control_transfer->total_len = total_bytes;
  control_transfer->queued_len = 0;

  uint8_t const ep_idn = control_transfer->ep_idn;

  if (ep_dir_idx == USB_EP_DIRECTION_IN_IDX) {
    usb_transmit_packet(control_transfer, ep_idn);
  } else {
    uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
    endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);

    uint16_t rx_size = min_u16(control_transfer->total_len, control_transfer->max_packet_size);

    usb_endpoint_set_rx_buffer_block_size(ep_idn, (uint32_t)rx_size);

    usb_endpoint_status(&endpoint_reg, ep_dir_idx, USB_EP_STATE_VALID);
    usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
  }

  return true;
}

void usb_endpoint_stall_set_hal(uint8_t ep_num, uint8_t ep_dir_idx) {
  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_num, ep_dir_idx);
  uint8_t const ep_idn = control_transfer->ep_idn;

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);
  usb_endpoint_status(&endpoint_reg, ep_dir_idx, USB_EP_STATE_STALL);

  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}

void usb_endpoint_stall_clear_hal(uint8_t ep_num, uint8_t ep_dir_idx) {
  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_num, ep_dir_idx);
  uint8_t const ep_idn = control_transfer->ep_idn;

  // Get current value of CHEPnR
  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);

  // Clear state
  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx) | USB_EP_DATA_TOGGLE_MASK(ep_dir_idx);

  // Reset to DATA0
  usb_endpoint_data_toggle(&endpoint_reg, ep_dir_idx, 0);

  // Set value of CHEPnR
  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}
