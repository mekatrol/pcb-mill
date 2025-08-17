#include "board_hal.h"
#include "usb_hal.h"

// Member unassigned value
#define UNASSIGNED_VALUE 0xFFU

typedef struct {
  uint8_t *buffer;           // The buffer location to transmit from/receive to
  uint16_t transfer_length;  // The amount of data to transmit/receive
  uint16_t queued_len;       // The number of bytes queued in the packet buffer for transmit/receive
  uint16_t max_packet_size;  // The maximum packet size that can be transferred
  uint8_t ep_idn;            // Endpoint identifier (is zero based so can be used as index to arrays)
} ep_transfer_packet_t;

typedef struct {
  uint8_t ep_idn;
  uint8_t ep_type;
  bool assigned[EP_IN_OUT_PAIR];
} ep_assignment_t;

// Active packets for each endpoint
ep_transfer_packet_t ep_packet[USB_EP_MAX][EP_IN_OUT_PAIR];

// State of endpoint assignment
ep_assignment_t ep_assignment[USB_EP_MAX];

// Next available USB PMA buffer pointer location
uint16_t usb_pma_next_available;

__attribute__((always_inline)) static inline uint32_t usb_pma_next_addr(uint32_t size) {
  // Get next available Packet Memory Area location
  uint32_t usb_pma_addr = usb_pma_next_available;

  // Update next available by adding size (size is assumed to be 32 bit aligned)
  usb_pma_next_available = (usb_pma_next_available + size);

  // Return the assigned address
  return usb_pma_addr;
}

__attribute__((always_inline)) static inline void usb_ep_data_toggle(uint32_t *ep_reg, usb_ep_direction_index_t ep_dir_idx, usb_ep_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in ep_reg
  *ep_reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (ep_dir_idx == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
}

__attribute__((always_inline)) static inline void usb_pma_set_count(uint32_t ep_idn, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline void usb_pma_set_ep_addr(uint32_t ep_idn, uint8_t idn_dir_idx, uint16_t addr) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[idn_dir_idx].count_addr;
  count_addr = (count_addr & 0xFFFF0000U) | (addr & 0x0000FFFCU);
  USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[idn_dir_idx].count_addr = count_addr;
}

__attribute__((always_inline)) static inline void ep_reset_assigned_state(uint32_t ep_idn) {
  ep_assignment[ep_idn].ep_idn = UNASSIGNED_VALUE;                   // Endpoint identity unassigned
  ep_assignment[ep_idn].ep_type = UNASSIGNED_VALUE;                  // Endpoint type unassigned
  ep_assignment[ep_idn].assigned[USB_EP_DIRECTION_OUT_IDX] = false;  // Out unassigned
  ep_assignment[ep_idn].assigned[USB_EP_DIRECTION_IN_IDX] = false;   // In unassigned
}

// Bit 31 BLSIZE: Block size
// This bit selects the size of memory block used to define the assigned buffer area.
//
// – If BLSIZE = 0, the memory block is 2-byte large, which is the minimum block
//   allowed in a half-word wide memory. With this block size the assigned buffer size
//   ranges from 2 to 62 bytes.
//
// – If BLSIZE = 1, the memory block is 32-byte large, which permits to reach the
//   maximum packet length defined by USB specifications. With this block size the
//   assigned buffer size theoretically ranges from 32 to 1024 bytes, which is the longest
//   packet size allowed by USB standard specifications. However, the applicable size is
//   limited by the available buffer memory
//
// Bits 30:26 NUM_BLOCK[4:0]: Number of blocks
// These bits define the number of memory blocks assigned to this packet buffer. The actual
// amount of assigned memory depends on the BLSIZE value as illustrated in RM0444 Table 239.
__attribute__((always_inline)) static inline uint32_t usb_ep_calc_rx_buffer_block_size(uint16_t buffer_size, uint32_t *blsize, uint32_t *num_block) {
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
  // See: RM0444 Table 239. Definition of assigned buffer memory
  // Easiest way is to just subtract BLSIZE from NUM_BLOCK
  *num_block = block_count - *blsize;

  // Same as:
  // block_count * 32 --> buffer_size  > 62
  // block_count *  2 --> buffer_size <= 62
  return block_count << block_size_log2;
}

void usb_ep_reset() {
  for (uint32_t idn = 0; idn < USB_EP_MAX; idn++) {
    ep_reset_assigned_state(idn);
  }

  // Reset PMA assignment (to end of EP buffer descriptor table)
  usb_pma_next_available = 8 * USB_EP_MAX;
}

uint8_t usb_ep_assign(uint8_t ep_addr, uint8_t ep_type) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  for (uint8_t idn = 0; idn < USB_EP_MAX; idn++) {
    // Check if already assigned, and return existing identifier if so
    if (ep_assignment[idn].assigned[ep_dir_idx] &&
        ep_assignment[idn].ep_type == ep_type &&
        ep_assignment[idn].ep_idn == ep_idn) {
      return idn;
    }

    // Assign only if currently not assigned
    if (!ep_assignment[idn].assigned[ep_dir_idx]) {
      // Check if EP number is the same
      if (ep_assignment[idn].ep_idn == UNASSIGNED_VALUE || ep_assignment[idn].ep_idn == ep_idn) {
        // One EP pair has to be the same type
        if (ep_assignment[idn].ep_type == UNASSIGNED_VALUE || ep_assignment[idn].ep_type == ep_type) {
          ep_assignment[idn].ep_idn = ep_idn;
          ep_assignment[idn].ep_type = ep_type;
          ep_assignment[idn].assigned[ep_dir_idx] = true;

          return idn;
        }
      }
    }
  }

  // Assignment failed
  return UNASSIGNED_VALUE;
}

void usb_ep_set_rx_buffer_block_size(uint32_t ep_idn, uint32_t size) {
  uint32_t blsize, num_block;
  usb_ep_calc_rx_buffer_block_size(size, &blsize, &num_block);

  // Merge BLSIZE and NUM_BLOCK and shift to correct bit positions
  uint32_t memory_buffer_assignment = (blsize << BIT_31_POS) | (num_block << BIT_26_POS);

  // Get existing register value (we don't want to override ADDR_RX), note this clears COUNT_RX
  // which is valid because we are setting the buffer size and previous received data likely invalid
  uint32_t usb_chep_txrxbd_n = USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[USB_EP_RX_BUFFER].count_addr;

  // Merge BLSIZE, NUM_BLOCK and ADDR_RX
  usb_chep_txrxbd_n = memory_buffer_assignment | (usb_chep_txrxbd_n & 0x0000FFFFU);

  // Update register
  USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[USB_EP_RX_BUFFER].count_addr = usb_chep_txrxbd_n;
}

void usb_ep_control_init() {
  usb_ep_assign(USB_DIR_OUT, USB_EP_TYPE_CONTROL);
  usb_ep_assign(USB_DIR_IN, USB_EP_TYPE_CONTROL);

  ep_packet[EP0_IDN][USB_EP_DIRECTION_OUT_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  ep_packet[EP0_IDN][USB_EP_DIRECTION_OUT_IDX].ep_idn = EP0_IDN;

  ep_packet[EP0_IDN][USB_EP_DIRECTION_IN_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  ep_packet[EP0_IDN][USB_EP_DIRECTION_IN_IDX].ep_idn = EP0_IDN;

  uint16_t pma_rx_addr = usb_pma_next_addr(USB_EP0_BUFFER_SIZE);
  uint16_t pma_tx_addr = usb_pma_next_addr(USB_EP0_BUFFER_SIZE);

  usb_pma_set_ep_addr(EP0_IDN, USB_EP_RX_BUFFER, pma_rx_addr);
  usb_pma_set_ep_addr(EP0_IDN, USB_EP_TX_BUFFER, pma_tx_addr);

  uint32_t ep_reg = USB->chep[EP0_IDN].CHEPnR & ~USB_CHEP_REG_MASK;
  ep_reg |= USB_EP_CONTROL;
  usb_ep_status(&ep_reg, USB_EP_DIRECTION_IN_IDX, USB_EP_STATE_NAK);
  usb_ep_status(&ep_reg, USB_EP_DIRECTION_OUT_IDX, USB_EP_STATE_NAK);

  usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
  usb_ep_reg_set(EP0_IDN, ep_reg, false);
}

void usb_ep_control_status_complete(const usb_control_request_t *request) {
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  if (request_recipient == USB_REQUEST_RECIPIENT_DEVICE &&
      request_type == USB_REQUEST_TYPE_STANDARD &&
      request->bRequest == USB_STD_SET_ADDRESS) {
    const uint8_t dev_addr = (uint8_t)request->wValue;
    USB->DADDR = (USB_DADDR_EF | dev_addr);
  }

  usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
}

bool usb_ep_open(const usb_ep_descriptor_t *ep_descriptor) {
  const uint8_t ep_addr = ep_descriptor->bEndpointAddress;
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  const uint32_t packet_size = USB_EP_PACKET_SIZE(ep_descriptor->wMaxPacketSize);
  const uint8_t ep_idn = usb_ep_assign(ep_addr, ep_descriptor->bmAttributes.type);

  // Fail if unassigned
  if (ep_idn == UNASSIGNED_VALUE) {
    return false;
  }

  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR & ~USB_CHEP_REG_MASK;
  ep_reg |= USB_EP_IDN(ep_addr);

  // Supported endpoint types
  switch (ep_descriptor->bmAttributes.type) {
    case USB_EP_TYPE_BULK:
      ep_reg |= USB_EP_BULK;
      break;

    case USB_EP_TYPE_INTERRUPT:
      ep_reg |= USB_EP_INTERRUPT;
      break;

    default:
      // End type is not supported
      return false;
  }

  /* Create a packet memory buffer area. */
  uint16_t pma_addr = usb_pma_next_addr(packet_size);
  usb_pma_set_ep_addr(ep_idn, ep_dir_idx == USB_EP_DIRECTION_IN_IDX ? USB_EP_TX_BUFFER : USB_EP_RX_BUFFER, pma_addr);

  ep_transfer_packet_t *packet = &ep_packet[ep_idn][ep_dir_idx];
  packet->max_packet_size = packet_size;
  packet->ep_idn = ep_idn;

  usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_NAK);
  usb_ep_data_toggle(&ep_reg, ep_dir_idx, 0);

  // reserve other direction toggle bits
  if (ep_dir_idx == USB_EP_DIRECTION_IN_IDX) {
    ep_reg &= ~(USB_CH_RX_VALID | USB_EP_DTOG_RX);
  } else {
    ep_reg &= ~(USB_CHEP_TX_STTX_Msk | USB_EP_DTOG_TX);
  }

  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);

  return true;
}

void usb_ep_close_all() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

  for (uint32_t i = 1; i < USB_EP_MAX; i++) {
    usb_ep_reg_set(i, 0, false);
    ep_reset_assigned_state(i);
  }

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  // Reset PMA assignment
  usb_pma_next_available = 8 * USB_EP_MAX + 2 * USB_EP0_BUFFER_SIZE;
}

/*
 * This method writes data from an unaligned buffer to the endpoint buffer
 */
__attribute__((always_inline)) static inline bool usb_write_unaligned_data(uint16_t dst, const void *__restrict src, uint16_t byte_count) {
  if (byte_count == 0) {
    // No count then nothing to write
    return true;
  }

  // We are writing 32 bit values from unaligned byte locations
  uint32_t write_count = byte_count / sizeof(uint32_t);

  // The PMA buffer we are writing to
  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + dst);

  // The unaligned buffer we area reading from
  const uint8_t *src8 = src;

  // Read unaligned byte and write to PMA buffer
  while (write_count--) {
    *pma_buf = unaligned_read_32(src8);
    src8 += sizeof(uint32_t);
    pma_buf++;
  }

  // Write an remaining bytes (for odd byte_count)
  // ie:
  //    1   for 16-bit
  //    1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t b = 0;
    for (uint16_t i = 0; i < odd; i++) {
      b |= *src8++ << (i * 8);
    }
    *pma_buf = b;
  }

  return true;
}

void usb_tx_packet(ep_transfer_packet_t *packet) {
  uint32_t len = min_u16(packet->transfer_length - packet->queued_len, packet->max_packet_size);

  uint16_t addr_ptr = (uint16_t)usb_pma_get_ep_addr(packet->ep_idn, USB_EP_TX_BUFFER);

  usb_write_unaligned_data(addr_ptr, &(packet->buffer[packet->queued_len]), len);
  packet->queued_len += len;

  usb_pma_set_count(packet->ep_idn, USB_EP_TX_BUFFER, len);

  uint32_t ep_reg = USB->chep[packet->ep_idn].CHEPnR;
  usb_ep_status(&ep_reg, USB_EP_DIRECTION_IN_IDX, USB_EP_STATE_VALID);

  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_EP_DIRECTION_IN_IDX);  // only change TX Status, reserve other toggle bits
  usb_ep_reg_set_preserve(packet->ep_idn, ep_reg, true);
}

/*
 * Prepare HAL for sending / receiving data from host
 */
bool usb_ep_transfer_queue_hal(uint8_t ep_idn, uint8_t ep_dir_idx, uint8_t *buffer, uint16_t total_bytes) {
  ep_transfer_packet_t *packet = &ep_packet[ep_idn][ep_dir_idx];

  // Cannot transfer more than configured packet size
  if (total_bytes > packet->max_packet_size) {
    return false;
  }

  // Initialise packet
  packet->buffer = buffer;
  packet->transfer_length = total_bytes;
  packet->queued_len = 0;

  if (ep_dir_idx == USB_EP_DIRECTION_IN_IDX) {
    // Transmit from device is USB_EP_DIRECTION_IN_IDX to host
    usb_tx_packet(packet);
  } else {
    // Receive to device is USB_EP_DIRECTION_OUT_IDX from host
    uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
    ep_reg &= (USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx));

    usb_ep_set_rx_buffer_block_size(ep_idn, (uint32_t)packet->transfer_length);
    usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_VALID);
    usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
  }

  // STM32G0B1 does not detect failures in this method so always return true (assume success)
  return true;
}

void usb_ep_stall_set_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);
  usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_STALL);

  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
}

void usb_ep_stall_clear_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  // Get current value of CHEPnR
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;

  // Clear state
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx) | USB_EP_DATA_TOGGLE_MASK(ep_dir_idx);

  // Reset to DATA0
  usb_ep_data_toggle(&ep_reg, ep_dir_idx, 0);

  // Set value of CHEPnR
  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
}

static void usb_ep_transfer_complete(uint8_t ep_addr, uint32_t transferred_bytes) {
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

void usb_ep_rx(uint32_t ep_idn) {
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
  ep_transfer_packet_t *packet = &ep_packet[ep_idn][USB_EP_DIRECTION_OUT_IDX];
  const uint16_t rx_count = usb_pma_get_count(ep_idn, USB_EP_RX_BUFFER);
  uint16_t pma_addr = (uint16_t)usb_pma_get_ep_addr(ep_idn, USB_EP_RX_BUFFER);

  usb_rx_packet(packet->buffer + packet->queued_len, pma_addr, rx_count);
  packet->queued_len += rx_count;

  if ((rx_count < packet->max_packet_size) || (packet->queued_len >= packet->transfer_length)) {
    // All bytes now received

    usb_ep_set_rx_buffer_block_size(ep_idn, (uint32_t)packet->max_packet_size);

    usb_ep_transfer_complete(ep_idn, packet->queued_len);

    packet->transfer_length = packet->queued_len = 0;
  } else {
    ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_EP_DIRECTION_OUT_IDX);  // will change RX Status, reserved other toggle bits
    usb_ep_status(&ep_reg, USB_EP_DIRECTION_OUT_IDX, USB_EP_STATE_VALID);
    usb_ep_reg_set_preserve(ep_idn, ep_reg, false);
  }
}

void usb_ep_tx_queued_bytes(uint32_t ep_idn) {
  ep_transfer_packet_t *packet = &ep_packet[ep_idn][USB_EP_DIRECTION_IN_IDX];

  if (packet->transfer_length != packet->queued_len) {
    usb_tx_packet(packet);
  } else {
    uint32_t ep_addr = USB->chep[ep_idn].CHEPnR & USB_CHEP_ADDR;
    usb_ep_transfer_complete(ep_addr | USB_DIR_IN, packet->queued_len);
  }
}
