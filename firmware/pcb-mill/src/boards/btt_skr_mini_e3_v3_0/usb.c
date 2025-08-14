#include "board_hal.h"
#include "usb.h"
#include "usb_hal.h"
#include "dcd.h"

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

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
        usb_endpoint_reg_set_clear_ctr(endpoint_idn, USB_ENDPOINT_DIRECTION_OUT);
        handle_ctr_rx(endpoint_idn);
      }
    }

    if (endpoint_reg & USB_EP_VTTX) {
      usb_endpoint_reg_set_clear_ctr(endpoint_idn, USB_ENDPOINT_DIRECTION_IN);
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
  uint32_t usb_chep_txrxbd_n = USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[ENDPOINT_RX_BUFFER].count_addr;

  // Merge BLSIZE, NUM_BLOCK and ADDR_RX
  usb_chep_txrxbd_n = memory_buffer_allocation | (usb_chep_txrxbd_n & 0x0000FFFFU);

  // Update register
  USB_BUFFER_DESC_TABLE->endpoint[endpoint_idn].buffer[ENDPOINT_RX_BUFFER].count_addr = usb_chep_txrxbd_n;
}

// One of these for every EP IN & OUT, uses a bit of RAM....
typedef struct {
  uint8_t *buffer;
  uint16_t total_len;
  uint16_t queued_len;
  uint16_t max_packet_size;
  uint8_t ep_idx;  // index for USB_EPnR register
} xfer_ctl_t;

// EP allocator
typedef struct {
  uint8_t ep_num;
  uint8_t ep_type;
  bool allocated[2];
} ep_alloc_t;

static xfer_ctl_t xfer_status[USB_ENDPOINT_MAX][2];
static ep_alloc_t ep_alloc_status[USB_ENDPOINT_MAX];

// PMA allocation/access
static uint16_t ep_buf_ptr;  ///< Points to first free memory location

__attribute__((always_inline)) static inline uint32_t unaligned_read32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

__attribute__((always_inline)) static inline void unaligned_write32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)(value);
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
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

static uint32_t usb_pma_alloc(uint16_t len, bool dbuf) {
  uint32_t blsize, num_block;
  uint16_t aligned_len = usb_endpoint_calc_rx_buffer_block_size(len, &blsize, &num_block);
  (void)blsize;
  (void)num_block;

  uint32_t addr = ep_buf_ptr;
  ep_buf_ptr = (uint16_t)(ep_buf_ptr + aligned_len);  // increment buffer pointer

  if (dbuf) {
    addr |= ((uint32_t)ep_buf_ptr) << 16;
    ep_buf_ptr = (uint16_t)(ep_buf_ptr + aligned_len);  // increment buffer pointer
  }

  // Verify packet buffer is not overflowed
  if (ep_buf_ptr > USB_DRD_PMA_SIZE) {
    return 0xFFFF;
  }

  return addr;
}

static void usb_transmit_packet(xfer_ctl_t *xfer, uint16_t ep_ix) {
  uint32_t len = min_u16(xfer->total_len - xfer->queued_len, xfer->max_packet_size);
  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_ix) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR

  uint16_t addr_ptr = (uint16_t)usb_pma_get_addr(ep_ix, ENDPOINT_TX_BUFFER);

  usb_write_packet_data(addr_ptr, &(xfer->buffer[xfer->queued_len]), len);
  xfer->queued_len += len;

  usb_pma_set_count(ep_ix, ENDPOINT_TX_BUFFER, len);
  usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_IN, USB_ENDPOINT_STATE_VALID);

  endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(USB_ENDPOINT_DIRECTION_IN);  // only change TX Status, reserve other toggle bits
  usb_endpoint_reg_set(ep_ix, endpoint_reg, true);
}

static void usb_endpoint0_init();

__attribute__((always_inline)) static inline void edpt0_prepare_setup(void) {
  usb_endpoint_set_rx_buffer_block_size(0, 8UL);
}

//--------------------------------------------------------------------+
// Inline helper
//--------------------------------------------------------------------+

__attribute__((always_inline)) static inline xfer_ctl_t *xfer_ctl_ptr(uint8_t epnum, uint8_t dir) {
  return &xfer_status[epnum][dir];
}

//--------------------------------------------------------------------+
// Controller API
//--------------------------------------------------------------------+
void dcd_init() {
  // Perform USB peripheral reset
  USB->CNTR = USB_CNTR_USBRST | USB_CNTR_PDWN;
  USB->CNTR &= ~USB_CNTR_PDWN;
  USB->CNTR = 0;  // Enable USB
  USB->ISTR = 0;  // Clear pending interrupts

  // Reset endpoints to disabled
  for (uint32_t i = 0; i < USB_ENDPOINT_MAX; i++) {
    // This doesn't clear all bits since some bits are "toggle", but does set the type to DISABLED.
    usb_endpoint_reg_set(i, 0u, false);
  }

  USB->CNTR |= USB_CNTR_RESETM | USB_CNTR_ESOFM | USB_CNTR_CTRM |
               USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_PMAOVRM;

  handle_bus_reset();

  USB->BCDR |= USB_BCDR_DPPU;
}

void dcd_sof_enable(bool en) {
  if (en) {
    USB->CNTR |= USB_CNTR_SOFM;
  } else {
    USB->CNTR &= ~USB_CNTR_SOFM;
  }
}

// Receive Set Address request, mcu port must also include status IN response
void dcd_set_address(uint8_t dev_addr) {
  (void)dev_addr;

  // Respond with status
  dcd_edpt_xfer(USB_ENDPOINT_DIRECTION_IN_MASK | 0x00, NULL, 0);

  // DCD can only set address after status for this request is complete.
  // do it at dcd_edpt0_status_complete()
}

void handle_bus_reset() {
  USB->DADDR = 0u;  // disable USB Function

  for (uint32_t i = 0; i < USB_ENDPOINT_MAX; i++) {
    // Clear EP allocation status
    ep_alloc_status[i].ep_num = 0xFF;
    ep_alloc_status[i].ep_type = 0xFF;
    ep_alloc_status[i].allocated[0] = false;
    ep_alloc_status[i].allocated[1] = false;
  }

  // Reset PMA allocation (to end of EP buffer table)
  ep_buf_ptr = 8 * USB_ENDPOINT_MAX;

  usb_endpoint0_init();  // open control endpoint (both IN & OUT)

  USB->DADDR = USB_DADDR_EF;  // Enable USB Function
}

bool usbd_control_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes);
bool cdcd_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes);

static void transfer_complete(uint8_t ep_addr, uint32_t xferred_bytes) {
  // Invoke the class callback associated with the endpoint address
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const ep_dir = usb_endpoint_direction(ep_addr);

  _usbd_dev.ep_status[epnum][ep_dir].busy = 0;
  _usbd_dev.ep_status[epnum][ep_dir].claimed = 0;

  if (epnum == 0) {
    usbd_control_xfer_cb(ep_addr, xferred_bytes);
  } else {
    cdcd_xfer_cb(ep_addr, xferred_bytes);
  }
}

// Handle CTR interrupt for the TX/IN direction
void handle_ctr_tx(uint32_t endpoint_idn) {
  uint32_t endpoint_addr = usb_endpoint_reg_get(endpoint_idn) & USB_CHEP_ADDR;
  xfer_ctl_t *xfer = xfer_ctl_ptr(endpoint_addr, USB_ENDPOINT_DIRECTION_IN);

  if (xfer->total_len != xfer->queued_len) {
    usb_transmit_packet(xfer, endpoint_idn);
  } else {
    transfer_complete(endpoint_addr | USB_ENDPOINT_DIRECTION_IN_MASK, xfer->queued_len);
  }
}

static void setup_received(tusb_control_request_t *setup_received) {
  // Mark as connected after receiving 1st setup packet.
  // But it is easier to set it every time instead of wasting time to check then set
  _usbd_dev.connected = 1;

  // mark both in & out control as free
  _usbd_dev.ep_status[USB_EP0_ADDR][USB_ENDPOINT_DIRECTION_OUT].busy = 0;
  _usbd_dev.ep_status[USB_EP0_ADDR][USB_ENDPOINT_DIRECTION_OUT].claimed = 0;
  _usbd_dev.ep_status[USB_EP0_ADDR][USB_ENDPOINT_DIRECTION_IN].busy = 0;
  _usbd_dev.ep_status[USB_EP0_ADDR][USB_ENDPOINT_DIRECTION_IN].claimed = 0;

  // Process control request
  if (!process_control_request(setup_received)) {
    // Failed -> stall both control endpoint IN and OUT
    usb_endpoint_stall(USB_EP0_ADDR);
    usb_endpoint_stall(USB_EP0_ADDR | USB_ENDPOINT_DIRECTION_IN_MASK);
  }
}

void handle_ctr_setup(uint32_t endpoint_idn) {
  uint16_t rx_count = usb_pma_get_count(endpoint_idn, ENDPOINT_RX_BUFFER);
  uint16_t rx_addr = usb_pma_get_addr(endpoint_idn, ENDPOINT_RX_BUFFER);
  uint8_t setup_packet[8] __attribute__((aligned(4)));

  usb_read_packet_data(setup_packet, rx_addr, rx_count);

  // Clear CTR RX if another setup packet arrived before this, it will be discarded
  usb_endpoint_reg_set_clear_ctr(endpoint_idn, USB_ENDPOINT_DIRECTION_OUT);

  // Setup packet should always be 8 bytes. If not, we probably missed the packet
  if (rx_count == 8) {
    setup_received((tusb_control_request_t *)setup_packet);
    // Hardware should reset EP0 RX/TX to NAK and both toggle to 1
  } else {
    // Missed setup packet !!!
    edpt0_prepare_setup();
  }
}

// Handle CTR interrupt for the RX/OUT direction
void handle_ctr_rx(uint32_t endpoint_idn) {
  uint32_t endpoint_reg = usb_endpoint_reg_get(endpoint_idn) | USB_EP_VTTX | USB_EP_VTRX;
  uint8_t const ep_num = endpoint_reg & USB_CHEP_ADDR;
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, USB_ENDPOINT_DIRECTION_OUT);

  uint16_t const rx_count = usb_pma_get_count(endpoint_idn, ENDPOINT_RX_BUFFER);
  uint16_t pma_addr = (uint16_t)usb_pma_get_addr(endpoint_idn, ENDPOINT_RX_BUFFER);

  usb_read_packet_data(xfer->buffer + xfer->queued_len, pma_addr, rx_count);
  xfer->queued_len += rx_count;

  if ((rx_count < xfer->max_packet_size) || (xfer->queued_len >= xfer->total_len)) {
    // all bytes received or short packet

    // For ch32v203: reset rx bufsize to mps to prevent race condition to cause PMAOVR (occurs with msc write10)
    usb_endpoint_set_rx_buffer_block_size(endpoint_idn, (uint32_t)xfer->max_packet_size);

    transfer_complete(ep_num, xfer->queued_len);

    // ch32 seems to unconditionally accept ZLP on EP0 OUT, which can incorrectly use queued_len of previous
    // transfer. So reset total_len and queued_len to 0.
    xfer->total_len = xfer->queued_len = 0;
  } else {
    endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(USB_ENDPOINT_DIRECTION_OUT);  // will change RX Status, reserved other toggle bits
    usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_OUT, USB_ENDPOINT_STATE_VALID);
    usb_endpoint_reg_set(endpoint_idn, endpoint_reg, false);
  }
}

//--------------------------------------------------------------------+
// Endpoint API
//--------------------------------------------------------------------+

// Invoked when a control transfer's status stage is complete.
// May help DCD to prepare for next control transfer, this API is optional.
void dcd_edpt0_status_complete(tusb_control_request_t const *request) {
  if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
      request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD &&
      request->bRequest == TUSB_REQ_SET_ADDRESS) {
    uint8_t const dev_addr = (uint8_t)request->wValue;
    USB->DADDR = (USB_DADDR_EF | dev_addr);
  }

  edpt0_prepare_setup();
}

static uint8_t usb_endpoint_allocate(uint8_t endpoint_addr, uint8_t endpoint_type) {
  uint8_t const epnum = usb_endpoint_number(endpoint_addr);
  uint8_t const dir = usb_endpoint_direction(endpoint_addr);

  for (uint8_t i = 0; i < USB_ENDPOINT_MAX; i++) {
    // Check if already allocated
    if (ep_alloc_status[i].allocated[dir] &&
        ep_alloc_status[i].ep_type == endpoint_type &&
        ep_alloc_status[i].ep_num == epnum) {
      return i;
    }

    // If EP of current direction is not allocated
    if (!ep_alloc_status[i].allocated[dir]) {
      // Check if EP number is the same
      if (ep_alloc_status[i].ep_num == 0xFF || ep_alloc_status[i].ep_num == epnum) {
        // One EP pair has to be the same type
        if (ep_alloc_status[i].ep_type == 0xFF || ep_alloc_status[i].ep_type == endpoint_type) {
          ep_alloc_status[i].ep_num = epnum;
          ep_alloc_status[i].ep_type = endpoint_type;
          ep_alloc_status[i].allocated[dir] = true;

          return i;
        }
      }
    }
  }

  // Allocation failed
  return 0;
}

void usb_endpoint0_init() {
  usb_endpoint_allocate(0x0, USB_ENDPOINT_TYPE_CONTROL);
  usb_endpoint_allocate(0x80, USB_ENDPOINT_TYPE_CONTROL);

  xfer_status[0][0].max_packet_size = USB_EP0_BUFFER_SIZE;
  xfer_status[0][0].ep_idx = 0;

  xfer_status[0][1].max_packet_size = USB_EP0_BUFFER_SIZE;
  xfer_status[0][1].ep_idx = 0;

  uint16_t pma_addr0 = usb_pma_alloc(USB_EP0_BUFFER_SIZE, false);
  uint16_t pma_addr1 = usb_pma_alloc(USB_EP0_BUFFER_SIZE, false);

  usb_pma_set_addr(0, ENDPOINT_RX_BUFFER, pma_addr0);
  usb_pma_set_addr(0, ENDPOINT_TX_BUFFER, pma_addr1);

  uint32_t endpoint_reg = usb_endpoint_reg_get(0) & ~USB_CHEP_REG_MASK;  // only get toggle bits
  endpoint_reg |= USB_EP_CONTROL;
  usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_IN, USB_ENDPOINT_STATE_NAK);
  usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_OUT, USB_ENDPOINT_STATE_NAK);

  edpt0_prepare_setup();  // prepare for setup packet
  usb_endpoint_reg_set(0, endpoint_reg, false);
}

bool usb_endpoint_open(usb_endpoint_descriptor_t const *desc_ep) {
  uint8_t const ep_addr = desc_ep->bEndpointAddress;
  uint8_t const ep_num = usb_endpoint_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  const uint16_t packet_size = usb_endpoint_packet_size(desc_ep);
  uint8_t const endpoint_idn = usb_endpoint_allocate(ep_addr, desc_ep->bmAttributes.type);

  if (endpoint_idn >= USB_ENDPOINT_MAX) {
    return false;
  }

  uint32_t endpoint_reg = usb_endpoint_reg_get(endpoint_idn) & ~USB_CHEP_REG_MASK;
  endpoint_reg |= usb_endpoint_number(ep_addr) | USB_EP_VTTX | USB_EP_VTRX;

  // Supported endpoint types
  switch (desc_ep->bmAttributes.type) {
    case USB_ENDPOINT_TYPE_BULK:
      endpoint_reg |= USB_EP_BULK;
      break;

    case USB_ENDPOINT_TYPE_INTERRUPT:
      endpoint_reg |= USB_EP_INTERRUPT;
      break;

    default:
      // End type is not supported
      return false;
  }

  /* Create a packet memory buffer area. */
  uint16_t pma_addr = usb_pma_alloc(packet_size, false);
  usb_pma_set_addr(endpoint_idn, dir == USB_ENDPOINT_DIRECTION_IN ? ENDPOINT_TX_BUFFER : ENDPOINT_RX_BUFFER, pma_addr);

  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  xfer->max_packet_size = packet_size;
  xfer->ep_idx = endpoint_idn;

  usb_endpoint_status(&endpoint_reg, dir, USB_ENDPOINT_STATE_NAK);
  usb_endpoint_data_toggle(&endpoint_reg, dir, 0);

  // reserve other direction toggle bits
  if (dir == USB_ENDPOINT_DIRECTION_IN) {
    endpoint_reg &= ~(USB_CH_RX_VALID | USB_EP_DTOG_RX);
  } else {
    endpoint_reg &= ~(USB_CHEP_TX_STTX_Msk | USB_EP_DTOG_TX);
  }

  usb_endpoint_reg_set(endpoint_idn, endpoint_reg, true);

  return true;
}

void dcd_edpt_close_all() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

  for (uint32_t i = 1; i < USB_ENDPOINT_MAX; i++) {
    // Reset endpoint
    usb_endpoint_reg_set(i, 0, false);
    // Clear EP allocation status
    ep_alloc_status[i].ep_num = 0xFF;
    ep_alloc_status[i].ep_type = 0xFF;
    ep_alloc_status[i].allocated[0] = false;
    ep_alloc_status[i].allocated[1] = false;
  }

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  // Reset PMA allocation
  ep_buf_ptr = 8 * USB_ENDPOINT_MAX + 2 * USB_EP0_BUFFER_SIZE;
}

static bool edpt_xfer(uint8_t ep_num, usb_endpoint_direction_t dir) {
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  if (dir == USB_ENDPOINT_DIRECTION_IN) {
    usb_transmit_packet(xfer, ep_idx);
  } else {
    uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idx) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR
    endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(dir);

    uint16_t cnt = min_u16(xfer->total_len, xfer->max_packet_size);

    usb_endpoint_set_rx_buffer_block_size(ep_idx, (uint32_t)cnt);

    usb_endpoint_status(&endpoint_reg, dir, USB_ENDPOINT_STATE_VALID);
    usb_endpoint_reg_set(ep_idx, endpoint_reg, true);
  }

  return true;
}

bool dcd_edpt_xfer(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes) {
  uint8_t const ep_num = usb_endpoint_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);

  xfer->buffer = buffer;
  xfer->total_len = total_bytes;
  xfer->queued_len = 0;

  return edpt_xfer(ep_num, dir);
}

void usb_endpoint_stall(uint8_t ep_addr) {
  uint8_t const ep_num = usb_endpoint_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idx) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR bits
  endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(dir);
  usb_endpoint_status(&endpoint_reg, dir, USB_ENDPOINT_STATE_STALL);

  usb_endpoint_reg_set(ep_idx, endpoint_reg, true);
}

void dcd_edpt_clear_stall(uint8_t ep_addr) {
  uint8_t const ep_num = usb_endpoint_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idx) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR bits
  endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(dir) | USB_ENDPOINT_DATA_TOGGLE_MASK(dir);

  usb_endpoint_data_toggle(&endpoint_reg, dir, 0);  // Reset to DATA0
  usb_endpoint_reg_set(ep_idx, endpoint_reg, true);
}
