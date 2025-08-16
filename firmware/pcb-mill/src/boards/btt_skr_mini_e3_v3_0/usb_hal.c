#include "board_hal.h"
#include "usb_hal.h"

endpoint_packet_t transfer_buffer_state[USB_EP_MAX][2];
ep_alloc_t endpoint_allocated_state[USB_EP_MAX];

// Next available USB PMA buffer pointer location
uint16_t usb_pma_next_available;

void usb_endpoint0_init();
static bool usb_read_packet_data(void *__restrict dst, uint16_t src, uint16_t byte_count);
static void usb_transmit_packet(endpoint_packet_t *control_transfer, uint16_t ep_idn);

bool usb_control_transfer_cb(uint8_t ep_addr, uint32_t xferred_bytes);
bool usb_cdc_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes);

__attribute__((always_inline)) static inline endpoint_packet_t *xfer_ctl_ptr(uint8_t ep_num, uint8_t dir) {
  return &transfer_buffer_state[ep_num][dir];
}

static void transfer_complete(uint8_t ep_addr, uint32_t xferred_bytes) {
  // Invoke the class callback associated with the endpoint address
  const uint8_t ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  usb_device.ep_status[ep_num][ep_dir_idx].busy = 0;
  usb_device.ep_status[ep_num][ep_dir_idx].claimed = 0;

  if (ep_num == 0) {
    usb_control_transfer_cb(ep_addr, xferred_bytes);
  } else {
    usb_cdc_xfer_cb(ep_addr, xferred_bytes);
  }
}

// Handle CTR interrupt for the RX/OUT direction
void handle_ctr_rx(uint32_t endpoint_idn) {
  uint32_t endpoint_reg = usb_endpoint_reg_get(endpoint_idn);
  const uint8_t ep_num = endpoint_reg & USB_CHEP_ADDR;
  endpoint_packet_t *control_transfer = xfer_ctl_ptr(ep_num, USB_EP_DIRECTION_OUT_IDX);

  uint16_t const rx_count = usb_pma_get_count(endpoint_idn, USB_EP_RX_BUFFER);
  uint16_t pma_addr = (uint16_t)usb_pma_get_addr(endpoint_idn, USB_EP_RX_BUFFER);

  usb_read_packet_data(control_transfer->buffer + control_transfer->queued_len, pma_addr, rx_count);
  control_transfer->queued_len += rx_count;

  if ((rx_count < control_transfer->max_packet_size) || (control_transfer->queued_len >= control_transfer->total_len)) {
    // all bytes received or short packet

    usb_endpoint_set_rx_buffer_block_size(endpoint_idn, (uint32_t)control_transfer->max_packet_size);

    transfer_complete(ep_num, control_transfer->queued_len);

    // ch32 seems to unconditionally accept ZLP on EP0 OUT, which can incorrectly use queued_len of previous
    // transfer. So reset total_len and queued_len to 0.
    control_transfer->total_len = control_transfer->queued_len = 0;
  } else {
    endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_EP_DIRECTION_OUT_IDX);  // will change RX Status, reserved other toggle bits
    usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_OUT_IDX, USB_EP_STATE_VALID);
    usb_endpoint_reg_set_preserve(endpoint_idn, endpoint_reg, false);
  }
}

// Handle CTR interrupt for the TX/IN direction
void handle_ctr_tx(uint32_t endpoint_idn) {
  uint32_t endpoint_addr = usb_endpoint_reg_get(endpoint_idn) & USB_CHEP_ADDR;
  endpoint_packet_t *control_transfer = xfer_ctl_ptr(endpoint_addr, USB_EP_DIRECTION_IN_IDX);

  if (control_transfer->total_len != control_transfer->queued_len) {
    usb_transmit_packet(control_transfer, endpoint_idn);
  } else {
    transfer_complete(endpoint_addr | USB_DIR_IN, control_transfer->queued_len);
  }
}

static void setup_received(usb_control_request_t *setup_received) {
  // Mark as connected after receiving 1st setup packet.
  // But it is easier to set it every time instead of wasting time to check then set
  usb_device.connected = 1;

  // mark both in & out control as free
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_OUT_IDX].busy = 0;
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_OUT_IDX].claimed = 0;
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_IN_IDX].busy = 0;
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_IN_IDX].claimed = 0;

  // Process control request
  if (!process_control_request(setup_received)) {
    // Failed -> stall both control endpoint IN and OUT
    usb_endpoint_stall_set(USB_EP0_ADDR | USB_DIR_IN);
    usb_endpoint_stall_set(USB_EP0_ADDR | USB_DIR_OUT);
  }
}

void usb_init_hal() {
  // Set PA11 and PA12 to Alternate Function mode
  GPIO_SET_MODE(GPIOA, BIT_11_POS, MODER_ALT);
  GPIO_SET_MODE(GPIOA, BIT_12_POS, MODER_ALT);

  // Set AF0 (USB) for PA11 and PA12 (AFR = 0)
  GPIOA->AFR[1] &= ~((GPIO_AF_MSK << ((BIT_11_POS - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << ((BIT_12_POS - 8) * GPIO_AF_BIT_COUNT)));

  // No pull-up/pull-down
  GPIOA->PUPDR &= ~((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));

  // Push-pull output (default)
  GPIOA->OTYPER &= ~(BIT_11 | BIT_12);

  // Very high speed (11b)
  GPIOA->OSPEEDR &= ~((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));
  GPIOA->OSPEEDR |= ((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));

  // Set up 48MHz clock for USB
  RCC->CR |= BIT_22;                     // Enable HSI48
  while ((RCC->CR & BIT_23) == 0);       // Wait for HSI48 ready
  RCC->CCIPR2 &= ~(0b11 << BIT_12_POS);  // Select HSI48 as USB clock source

  // Enable USB peripheral clock
  RCC->APBENR1 |= RCC_APBENR1_USBEN;

  // Enable USB IRQ
  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

void handle_ctr_setup(uint32_t endpoint_idn) {
  uint16_t rx_count = usb_pma_get_count(endpoint_idn, USB_EP_RX_BUFFER);
  uint16_t rx_addr = usb_pma_get_addr(endpoint_idn, USB_EP_RX_BUFFER);
  uint8_t setup_packet[8] __attribute__((aligned(4)));

  usb_read_packet_data(setup_packet, rx_addr, rx_count);

  // Clear CTR RX if another setup packet arrived before this, it will be discarded
  usb_endpoint_reg_set_clear_ctr(endpoint_idn, USB_EP_DIRECTION_OUT_IDX);

  // Setup packet should always be 8 bytes. If not, we probably missed the packet
  if (rx_count == 8) {
    setup_received((usb_control_request_t *)setup_packet);
    // Hardware should reset EP0 RX/TX to NAK and both toggle to 1
  } else {
    usb_endpoint_set_rx_buffer_block_size(0, sizeof(usb_control_request_t));
  }
}

void USB_UCPD1_2_IRQHandler() {
  uint32_t int_status = USB->ISTR;

  if (int_status & USB_ISTR_SOF) {
    USB->ISTR = ~USB_ISTR_SOF;
    // Start of Frame
  }

  if (int_status & USB_ISTR_RESET) {
    // Clear reset flag
    USB->ISTR = ~USB_ISTR_RESET;

    // Call driver reset
    usb_reset();

    // Return after resetting USB as all should be clear
    return;
  }

  if (int_status & USB_ISTR_WKUP) {
    USB->CNTR &= ~USB_CNTR_SUSPRDY;
    USB->CNTR &= ~USB_CNTR_SUSPEN;

    USB->ISTR = ~USB_ISTR_WKUP;
  }

  if (int_status & USB_ISTR_SUSP) {
    /* Suspend is asserted for both suspend and unplug events. without Vbus monitoring,
     * these events cannot be differentiated, so we only trigger suspend. */

    /* Force low-power mode in the macrocell */
    USB->CNTR |= USB_CNTR_SUSPEN;
    USB->CNTR |= USB_CNTR_SUSPRDY;

    /* clear of the ISTR bit must be done after setting of CNTR_FSUSP */
    USB->ISTR = ~USB_ISTR_SUSP;
  }

  if (int_status & USB_ISTR_ESOF) {
    USB->ISTR = ~USB_ISTR_ESOF;
  }

  // loop to handle all pending CTR interrupts
  while (USB->ISTR & USB_ISTR_CTR) {
    // These bits are written by the hardware according to the host channel or device endpoint number
    uint32_t const endpoint_idn = USB->ISTR & USB_ISTR_IDN;
    uint32_t const endpoint_reg = usb_endpoint_reg_get(endpoint_idn);

    if (endpoint_reg & USB_EP_VTRX) {
      if (endpoint_reg & USB_EP_SETUP) {
        handle_ctr_setup(endpoint_idn);  // CTR will be clear after copied setup packet
      } else {
        usb_endpoint_reg_set_clear_ctr(endpoint_idn, USB_EP_DIRECTION_OUT_IDX);
        handle_ctr_rx(endpoint_idn);
      }
    }

    if (endpoint_reg & USB_EP_VTTX) {
      usb_endpoint_reg_set_clear_ctr(endpoint_idn, USB_EP_DIRECTION_IN_IDX);
      handle_ctr_tx(endpoint_idn);
    }
  }

  if (int_status & USB_ISTR_PMAOVR) {
    USB->ISTR = ~USB_ISTR_PMAOVR;

    // TODO: overrun/underrun
  }
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

static bool usb_read_packet_data(void *__restrict dst, uint16_t src, uint16_t byte_count) {
  if (byte_count == 0) {
    // Not count then nothing to read
    return true;
  }

  // We are readng 32 bit values from unaligned byte locations
  uint32_t read_count = byte_count / sizeof(uint32_t);

  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + src);
  uint8_t *dst8 = (uint8_t *)dst;

  while (read_count--) {
    unaligned_write32(dst8, (uint32_t)(*pma_buf));
    dst8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = *pma_buf;
    while (odd--) {
      *dst8++ = (uint8_t)(temp & 0xfful);
      temp >>= 8;
    }
  }

  return true;
}

static bool usb_write_packet_data(uint16_t dst, const void *__restrict src, uint16_t byte_count) {
  if (byte_count == 0) {
    // Not count then nothing to write
    return true;
  }

  // We are writing 32 bit values from unaligned byte locations
  uint32_t write_count = byte_count / sizeof(uint32_t);

  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + dst);
  const uint8_t *src8 = src;

  while (write_count--) {
    *pma_buf = unaligned_read32(src8);
    src8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = 0;
    for (uint16_t i = 0; i < odd; i++) {
      temp |= *src8++ << (i * 8);
    }
    *pma_buf = temp;
  }

  return true;
}

void usb_device_init() {
  // Perform USB peripheral reset
  USB->CNTR = USB_CNTR_USBRST | USB_CNTR_PDWN;
  USB->CNTR &= ~USB_CNTR_PDWN;
  USB->CNTR = 0;  // Enable USB
  USB->ISTR = 0;  // Clear pending interrupts

  // Reset endpoints to disabled
  for (uint32_t i = 0; i < USB_EP_MAX; i++) {
    // This doesn't clear all bits since some bits are "toggle", but does set the type to DISABLED.
    usb_endpoint_reg_set(i, 0, false);
  }

  USB->CNTR |= USB_CNTR_RESETM | USB_CNTR_ESOFM | USB_CNTR_CTRM |
               USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_PMAOVRM;

  handle_bus_reset();

  // Enable pull up to tell host it can enumerate device
  USB->BCDR |= USB_BCDR_DPPU;
}

void usb_sof_set_enable(bool enable) {
  if (enable) {
    USB->CNTR |= USB_CNTR_SOFM;
  } else {
    USB->CNTR &= ~USB_CNTR_SOFM;
  }
}

void handle_bus_reset() {
  USB->DADDR = 0u;  // disable USB Function

  for (uint32_t i = 0; i < USB_EP_MAX; i++) {
    ep_reset_allocated_state(endpoint_allocated_state, i);
  }

  // Reset PMA allocation (to end of EP buffer descriptor table)
  usb_pma_next_available = 8 * USB_EP_MAX;

  // EP0 must exist
  usb_endpoint0_init();

  // Enable USB
  USB->DADDR = USB_DADDR_EF;
}

// Invoked when a control transfer's status stage is complete.
// May help DCD to prepare for next control transfer, this API is optional.
void dcd_edpt0_status_complete(usb_control_request_t const *request) {
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  if (request_recipient == USB_REQUEST_RECIPIENT_DEVICE &&
      request_type == USB_REQUEST_TYPE_STANDARD &&
      request->bRequest == USB_STD_SET_ADDRESS) {
    uint8_t const dev_addr = (uint8_t)request->wValue;

    USB->DADDR = (USB_DADDR_EF | dev_addr);
  }

  usb_endpoint_set_rx_buffer_block_size(0, sizeof(usb_control_request_t));
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

bool usb_endpoint_open(usb_endpoint_descriptor_t const *endpoint_descriptor) {
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
  usb_pma_set_addr(endpoint_idn, ep_dir_idx == USB_EP_DIRECTION_IN_IDX ? USB_EP_TX_BUFFER : USB_EP_RX_BUFFER, pma_addr);

  endpoint_packet_t *control_transfer = xfer_ctl_ptr(ep_num, ep_dir_idx);
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

void usb_close_all_endpoints() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

  for (uint32_t i = 1; i < USB_EP_MAX; i++) {
    usb_endpoint_reg_set(i, 0, false);
    ep_reset_allocated_state(endpoint_allocated_state, i);
  }

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  // Reset PMA allocation
  usb_pma_next_available = 8 * USB_EP_MAX + 2 * USB_EP0_BUFFER_SIZE;
}

static void usb_transmit_packet(endpoint_packet_t *control_transfer, uint16_t ep_idn) {
  uint32_t len = min_u16(control_transfer->total_len - control_transfer->queued_len, control_transfer->max_packet_size);

  uint16_t addr_ptr = (uint16_t)usb_pma_get_addr(ep_idn, USB_EP_TX_BUFFER);

  usb_write_packet_data(addr_ptr, &(control_transfer->buffer[control_transfer->queued_len]), len);
  control_transfer->queued_len += len;

  usb_pma_set_count(ep_idn, USB_EP_TX_BUFFER, len);

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
  usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_IN_IDX, USB_EP_STATE_VALID);

  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_EP_DIRECTION_IN_IDX);  // only change TX Status, reserve other toggle bits
  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}

static bool edpt_xfer(uint8_t ep_num, usb_endpoint_direction_index_t dir) {
  endpoint_packet_t *control_transfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idn = control_transfer->ep_idn;

  if (dir == USB_EP_DIRECTION_IN_IDX) {
    usb_transmit_packet(control_transfer, ep_idn);
  } else {
    uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
    endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(dir);

    uint16_t rx_size = min_u16(control_transfer->total_len, control_transfer->max_packet_size);

    usb_endpoint_set_rx_buffer_block_size(ep_idn, (uint32_t)rx_size);

    usb_endpoint_status(&endpoint_reg, dir, USB_EP_STATE_VALID);
    usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
  }

  return true;
}

bool dcd_edpt_xfer(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes) {
  const uint8_t ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_packet_t *control_transfer = xfer_ctl_ptr(ep_num, ep_dir_idx);

  control_transfer->buffer = buffer;
  control_transfer->total_len = total_bytes;
  control_transfer->queued_len = 0;

  return edpt_xfer(ep_num, ep_dir_idx);
}

void usb_endpoint_stall_set(uint8_t ep_addr) {
  const uint8_t ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_packet_t *control_transfer = xfer_ctl_ptr(ep_num, ep_dir_idx);
  uint8_t const ep_idn = control_transfer->ep_idn;

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);
  usb_endpoint_status(&endpoint_reg, ep_dir_idx, USB_EP_STATE_STALL);

  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}

void dcd_edpt_clear_stall(uint8_t ep_addr) {
  const uint8_t ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_packet_t *control_transfer = xfer_ctl_ptr(ep_num, ep_dir_idx);
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
