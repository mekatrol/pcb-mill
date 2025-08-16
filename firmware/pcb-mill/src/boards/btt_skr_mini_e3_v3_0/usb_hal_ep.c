#include "board_hal.h"
#include "usb_hal.h"

// EP0 identifier
#define EP0_IDN 0

typedef struct {
  uint8_t ep_idn;
  uint8_t ep_type;
  bool allocated[EP_IN_OUT_PAIR];
} ep_allocation_t;

endpoint_packet_t transfer_buffer_state[USB_EP_MAX][EP_IN_OUT_PAIR];
ep_allocation_t endpoint_allocated_state[USB_EP_MAX];

// Next available USB PMA buffer pointer location
uint16_t usb_pma_next_available;

uint8_t usb_endpoint_allocate(uint8_t ep_addr, uint8_t endpoint_type);

endpoint_packet_t *endpoint_buffer_state(uint8_t ep_idn, uint8_t dir) {
  return &transfer_buffer_state[ep_idn][dir];
}

__attribute__((always_inline)) static inline uint32_t usb_pma_next_addr(uint32_t usb_pma_next_available, uint32_t size) {
  // Get next available Packet Memory Area location
  uint32_t usb_pma_addr = usb_pma_next_available;

  // Update next available by adding size (size is assumed to be 32 bit aligned)
  usb_pma_next_available = (usb_pma_next_available + size);

  return usb_pma_addr;
}

__attribute__((always_inline)) static inline void usb_endpoint_data_toggle(uint32_t *endpoint_reg, usb_endpoint_direction_index_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
}

__attribute__((always_inline)) static inline void usb_pma_set_endpoint_addr(uint32_t ep_idn, uint8_t buf_id, uint16_t addr) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & 0xFFFF0000U) | (addr & 0x0000FFFCU);
  USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline void ep_reset_allocated_state(ep_allocation_t *endpoint_allocated_state, uint32_t ep_idn) {
  endpoint_allocated_state[ep_idn].ep_idn = 0xFF;
  endpoint_allocated_state[ep_idn].ep_type = 0xFF;
  endpoint_allocated_state[ep_idn].allocated[USB_EP_DIRECTION_OUT_IDX] = false;
  endpoint_allocated_state[ep_idn].allocated[USB_EP_DIRECTION_IN_IDX] = false;
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

void usb_endpoint_reset() {
  for (uint32_t i = 0; i < USB_EP_MAX; i++) {
    ep_reset_allocated_state(endpoint_allocated_state, i);
  }

  // Reset PMA allocation (to end of EP buffer descriptor table)
  usb_pma_next_available = 8 * USB_EP_MAX;
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

void usb_endpoint_set_rx_buffer_block_size(uint32_t ep_idn, uint32_t size) {
  uint32_t blsize, num_block;
  usb_endpoint_calc_rx_buffer_block_size(size, &blsize, &num_block);

  // Merge BLSIZE and NUM_BLOCK and shift to correct bit positions
  uint32_t memory_buffer_allocation = (blsize << BIT_31_POS) | (num_block << BIT_26_POS);

  // Get existing register value (we don't want to override ADDR_RX), note this clears COUNT_RX
  // which is valid because we are setting the buffer size and previous received data likely invalid
  uint32_t usb_chep_txrxbd_n = USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[USB_EP_RX_BUFFER].count_addr;

  // Merge BLSIZE, NUM_BLOCK and ADDR_RX
  usb_chep_txrxbd_n = memory_buffer_allocation | (usb_chep_txrxbd_n & 0x0000FFFFU);

  // Update register
  USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[USB_EP_RX_BUFFER].count_addr = usb_chep_txrxbd_n;
}

uint8_t usb_endpoint_allocate(uint8_t ep_addr, uint8_t endpoint_type) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  for (uint8_t i = 0; i < USB_EP_MAX; i++) {
    // Check if already allocated
    if (endpoint_allocated_state[i].allocated[ep_dir_idx] &&
        endpoint_allocated_state[i].ep_type == endpoint_type &&
        endpoint_allocated_state[i].ep_idn == ep_idn) {
      return i;
    }

    // If EP of current direction is not allocated
    if (!endpoint_allocated_state[i].allocated[ep_dir_idx]) {
      // Check if EP number is the same
      if (endpoint_allocated_state[i].ep_idn == 0xFF || endpoint_allocated_state[i].ep_idn == ep_idn) {
        // One EP pair has to be the same type
        if (endpoint_allocated_state[i].ep_type == 0xFF || endpoint_allocated_state[i].ep_type == endpoint_type) {
          endpoint_allocated_state[i].ep_idn = ep_idn;
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
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  const uint32_t packet_size = USB_EP_PACKET_SIZE(endpoint_descriptor->wMaxPacketSize);
  uint8_t const ep_idn = usb_endpoint_allocate(ep_addr, endpoint_descriptor->bmAttributes.type);

  if (ep_idn >= USB_EP_MAX) {
    return false;
  }

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn) & ~USB_CHEP_REG_MASK;
  endpoint_reg |= USB_EP_IDN(ep_addr);

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
  usb_pma_set_endpoint_addr(ep_idn, ep_dir_idx == USB_EP_DIRECTION_IN_IDX ? USB_EP_TX_BUFFER : USB_EP_RX_BUFFER, pma_addr);

  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_idn, ep_dir_idx);
  control_transfer->max_packet_size = packet_size;
  control_transfer->ep_idn = ep_idn;

  usb_endpoint_status(&endpoint_reg, ep_dir_idx, USB_EP_STATE_NAK);
  usb_endpoint_data_toggle(&endpoint_reg, ep_dir_idx, 0);

  // reserve other direction toggle bits
  if (ep_dir_idx == USB_EP_DIRECTION_IN_IDX) {
    endpoint_reg &= ~(USB_CH_RX_VALID | USB_EP_DTOG_RX);
  } else {
    endpoint_reg &= ~(USB_CHEP_TX_STTX_Msk | USB_EP_DTOG_TX);
  }

  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);

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

bool usb_endpoint_transfer_hal(uint8_t ep_idn, uint8_t ep_dir_idx, uint8_t *buffer, uint16_t total_bytes) {
  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_idn, ep_dir_idx);

  control_transfer->buffer = buffer;
  control_transfer->total_len = total_bytes;
  control_transfer->queued_len = 0;

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

void usb_endpoint_stall_set_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);
  usb_endpoint_status(&endpoint_reg, ep_dir_idx, USB_EP_STATE_STALL);

  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}

void usb_endpoint_stall_clear_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  // Get current value of CHEPnR
  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);

  // Clear state
  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx) | USB_EP_DATA_TOGGLE_MASK(ep_dir_idx);

  // Reset to DATA0
  usb_endpoint_data_toggle(&endpoint_reg, ep_dir_idx, 0);

  // Set value of CHEPnR
  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}

static void usb_endpoint_transfer_complete(uint8_t ep_addr, uint32_t transferred_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  usb_device.ep_status[ep_idn][ep_dir_idx].busy = 0;
  usb_device.ep_status[ep_idn][ep_dir_idx].claimed = 0;

  if (ep_idn == 0) {
    usb_control_transfer(ep_addr, transferred_bytes);
  } else {
    usb_cdc_transfer(ep_addr, transferred_bytes);
  }
}

void usb_endpoint_ctr_rx(uint32_t ep_idn) {
  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
  endpoint_packet_t *control_transfer = endpoint_buffer_state(ep_idn, USB_EP_DIRECTION_OUT_IDX);

  uint16_t const rx_count = usb_pma_get_count(ep_idn, USB_EP_RX_BUFFER);
  uint16_t pma_addr = (uint16_t)usb_pma_get_endpoint_addr(ep_idn, USB_EP_RX_BUFFER);

  usb_read_packet_data(control_transfer->buffer + control_transfer->queued_len, pma_addr, rx_count);
  control_transfer->queued_len += rx_count;

  if ((rx_count < control_transfer->max_packet_size) || (control_transfer->queued_len >= control_transfer->total_len)) {
    // all bytes received or short packet

    usb_endpoint_set_rx_buffer_block_size(ep_idn, (uint32_t)control_transfer->max_packet_size);

    usb_endpoint_transfer_complete(ep_idn, control_transfer->queued_len);

    // ch32 seems to unconditionally accept ZLP on EP0 OUT, which can incorrectly use queued_len of previous
    // transfer. So reset total_len and queued_len to 0.
    control_transfer->total_len = control_transfer->queued_len = 0;
  } else {
    endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_EP_DIRECTION_OUT_IDX);  // will change RX Status, reserved other toggle bits
    usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_OUT_IDX, USB_EP_STATE_VALID);
    usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, false);
  }
}

void usb_endpoint_ctr_tx(uint32_t ep_idn) {
  uint32_t endpoint_addr = usb_endpoint_reg_get(ep_idn) & USB_CHEP_ADDR;
  endpoint_packet_t *control_transfer = endpoint_buffer_state(endpoint_addr, USB_EP_DIRECTION_IN_IDX);

  if (control_transfer->total_len != control_transfer->queued_len) {
    usb_transmit_packet(control_transfer, ep_idn);
  } else {
    usb_endpoint_transfer_complete(endpoint_addr | USB_DIR_IN, control_transfer->queued_len);
  }
}
