#include "usb.h"
#include "tusb_types.h"
#include "dcd.h"
#include "fsdev_type.h"

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

//--------------------------------------------------------------------+
// Prototypes
//--------------------------------------------------------------------+

// into the stack.
static void dcd_transmit_packet(xfer_ctl_t *xfer, uint16_t ep_ix);
static bool edpt_xfer(uint8_t ep_num, usb_endpoint_direction_t dir);

// PMA allocation/access
static uint16_t ep_buf_ptr;  ///< Points to first free memory location
static uint32_t dcd_pma_alloc(uint16_t len, bool dbuf);
static uint8_t usb_endpoint_allocate(uint8_t ep_addr, uint8_t ep_type);
static bool dcd_write_packet_memory(uint16_t dst, const void *__restrict src, uint16_t nbytes);
static bool dcd_read_packet_memory(void *__restrict dst, uint16_t src, uint16_t nbytes);

static void usb_endpoint0_init();

__attribute__((always_inline)) static inline void edpt0_prepare_setup(void) {
  usb_pma_set_rx_bufsize(0, ENDPOINT_RX_BUFFER, 8);
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
    usb_endpoint_write(i, 0u, false);
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
  uint8_t const epnum = tu_edpt_number(ep_addr);
  uint8_t const ep_dir = usb_endpoint_direction(ep_addr);

  _usbd_dev.ep_status[epnum][ep_dir].busy = 0;
  _usbd_dev.ep_status[epnum][ep_dir].claimed = 0;

  if (epnum == 0) {
    usbd_control_xfer_cb(ep_addr, xferred_bytes);
  } else {
    cdcd_xfer_cb(ep_addr, xferred_bytes);
  }
}

__attribute__((always_inline)) static inline uint32_t usb_endpoint_read(uint32_t endpoint_id) {
  return USB->chep[endpoint_id].CHEPnR;
}

// Handle CTR interrupt for the TX/IN direction
void handle_ctr_tx(uint32_t endpoint_id) {
  uint32_t endpoint_addr = usb_endpoint_read(endpoint_id) & USB_CHEP_ADDR;
  xfer_ctl_t *xfer = xfer_ctl_ptr(endpoint_addr, USB_ENDPOINT_DIRECTION_IN);

  if (xfer->total_len != xfer->queued_len) {
    dcd_transmit_packet(xfer, endpoint_id);
  } else {
    transfer_complete(endpoint_addr | USB_ENDPOINT_DIRECTION_IN_MASK, xfer->queued_len);
  }
}

static void setup_received(tusb_control_request_t *setup_received) {
  // Mark as connected after receiving 1st setup packet.
  // But it is easier to set it every time instead of wasting time to check then set
  _usbd_dev.connected = 1;

  // mark both in & out control as free
  _usbd_dev.ep_status[0][USB_ENDPOINT_DIRECTION_OUT].busy = 0;
  _usbd_dev.ep_status[0][USB_ENDPOINT_DIRECTION_OUT].claimed = 0;
  _usbd_dev.ep_status[0][USB_ENDPOINT_DIRECTION_IN].busy = 0;
  _usbd_dev.ep_status[0][USB_ENDPOINT_DIRECTION_IN].claimed = 0;

  // Process control request
  if (!process_control_request(setup_received)) {
    // Failed -> stall both control endpoint IN and OUT
    dcd_edpt_stall(0);
    dcd_edpt_stall(0 | USB_ENDPOINT_DIRECTION_IN_MASK);
  }
}

void handle_ctr_setup(uint32_t endpoint_id) {
  uint16_t rx_count = usb_pma_get_count(endpoint_id, ENDPOINT_RX_BUFFER);
  uint16_t rx_addr = usb_pma_get_addr(endpoint_id, ENDPOINT_RX_BUFFER);
  uint8_t setup_packet[8] __attribute__((aligned(4)));

  dcd_read_packet_memory(setup_packet, rx_addr, rx_count);

  // Clear CTR RX if another setup packet arrived before this, it will be discarded
  usb_endpoint_write_clear_ctr(endpoint_id, USB_ENDPOINT_DIRECTION_OUT);

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
void handle_ctr_rx(uint32_t endpoint_id) {
  uint32_t endpoint_reg = usb_endpoint_read(endpoint_id) | USB_EP_VTTX | USB_EP_VTRX;
  uint8_t const ep_num = endpoint_reg & USB_CHEP_ADDR;
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, USB_ENDPOINT_DIRECTION_OUT);

  uint16_t const rx_count = usb_pma_get_count(endpoint_id, ENDPOINT_RX_BUFFER);
  uint16_t pma_addr = (uint16_t)usb_pma_get_addr(endpoint_id, ENDPOINT_RX_BUFFER);

  dcd_read_packet_memory(xfer->buffer + xfer->queued_len, pma_addr, rx_count);
  xfer->queued_len += rx_count;

  if ((rx_count < xfer->max_packet_size) || (xfer->queued_len >= xfer->total_len)) {
    // all bytes received or short packet

    // For ch32v203: reset rx bufsize to mps to prevent race condition to cause PMAOVR (occurs with msc write10)
    usb_pma_set_rx_bufsize(endpoint_id, ENDPOINT_RX_BUFFER, xfer->max_packet_size);

    transfer_complete(ep_num, xfer->queued_len);

    // ch32 seems to unconditionally accept ZLP on EP0 OUT, which can incorrectly use queued_len of previous
    // transfer. So reset total_len and queued_len to 0.
    xfer->total_len = xfer->queued_len = 0;
  } else {
    endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(USB_ENDPOINT_DIRECTION_OUT);  // will change RX Status, reserved other toggle bits
    usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_OUT, USB_ENDPOINT_STATE_VALID);
    usb_endpoint_write(endpoint_id, endpoint_reg, false);
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

/***
 * Allocate a section of PMA
 * In case of double buffering, high 16bit is the address of 2nd buffer
 * During failure, 0xFFFF is returned. If this happens, rework/reallocate memory manually.
 */
static uint32_t dcd_pma_alloc(uint16_t len, bool dbuf) {
  uint8_t blsize, num_block;
  uint16_t aligned_len = pma_align_buffer_size(len, &blsize, &num_block);
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

static uint8_t usb_endpoint_allocate(uint8_t endpoint_addr, uint8_t endpoint_type) {
  uint8_t const epnum = tu_edpt_number(endpoint_addr);
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
  usb_endpoint_allocate(0x0, TUSB_XFER_CONTROL);
  usb_endpoint_allocate(0x80, TUSB_XFER_CONTROL);

  xfer_status[0][0].max_packet_size = USB_EP0_BUFFER_SIZE;
  xfer_status[0][0].ep_idx = 0;

  xfer_status[0][1].max_packet_size = USB_EP0_BUFFER_SIZE;
  xfer_status[0][1].ep_idx = 0;

  uint16_t pma_addr0 = dcd_pma_alloc(USB_EP0_BUFFER_SIZE, false);
  uint16_t pma_addr1 = dcd_pma_alloc(USB_EP0_BUFFER_SIZE, false);

  usb_pma_set_addr(0, ENDPOINT_RX_BUFFER, pma_addr0);
  usb_pma_set_addr(0, ENDPOINT_TX_BUFFER, pma_addr1);

  uint32_t endpoint_reg = usb_endpoint_read(0) & ~USB_CHEP_REG_MASK;  // only get toggle bits
  endpoint_reg |= USB_EP_CONTROL;
  usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_IN, USB_ENDPOINT_STATE_NAK);
  usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_OUT, USB_ENDPOINT_STATE_NAK);

  edpt0_prepare_setup();  // prepare for setup packet
  usb_endpoint_write(0, endpoint_reg, false);
}

bool usb_endpoint_open(tusb_desc_endpoint_t const *desc_ep) {
  uint8_t const ep_addr = desc_ep->bEndpointAddress;
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  const uint16_t packet_size = tu_edpt_packet_size(desc_ep);
  uint8_t const ep_idx = usb_endpoint_allocate(ep_addr, desc_ep->bmAttributes.xfer);

  if (ep_idx >= USB_ENDPOINT_MAX) {
    return false;
  }

  uint32_t endpoint_reg = usb_endpoint_read(ep_idx) & ~USB_CHEP_REG_MASK;
  endpoint_reg |= tu_edpt_number(ep_addr) | USB_EP_VTTX | USB_EP_VTRX;

  // Set type
  switch (desc_ep->bmAttributes.xfer) {
    case TUSB_XFER_BULK:
      endpoint_reg |= USB_EP_BULK;
      break;
    case TUSB_XFER_INTERRUPT:
      endpoint_reg |= USB_EP_INTERRUPT;
      break;

    default:
      return false;
  }

  /* Create a packet memory buffer area. */
  uint16_t pma_addr = dcd_pma_alloc(packet_size, false);
  usb_pma_set_addr(ep_idx, dir == USB_ENDPOINT_DIRECTION_IN ? ENDPOINT_TX_BUFFER : ENDPOINT_RX_BUFFER, pma_addr);

  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  xfer->max_packet_size = packet_size;
  xfer->ep_idx = ep_idx;

  usb_endpoint_status(&endpoint_reg, dir, USB_ENDPOINT_STATE_NAK);
  usb_endpoint_data_toggle(&endpoint_reg, dir, 0);

  // reserve other direction toggle bits
  if (dir == USB_ENDPOINT_DIRECTION_IN) {
    endpoint_reg &= ~(USB_CH_RX_VALID | USB_EP_DTOG_RX);
  } else {
    endpoint_reg &= ~(USB_CHEP_TX_STTX_Msk | USB_EP_DTOG_TX);
  }

  usb_endpoint_write(ep_idx, endpoint_reg, true);

  return true;
}

void dcd_edpt_close_all() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

  for (uint32_t i = 1; i < USB_ENDPOINT_MAX; i++) {
    // Reset endpoint
    usb_endpoint_write(i, 0, false);
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

// Currently, single-buffered, and only 64 bytes at a time (max)
static void dcd_transmit_packet(xfer_ctl_t *xfer, uint16_t ep_ix) {
  uint16_t len = min_u16(xfer->total_len - xfer->queued_len, xfer->max_packet_size);
  uint32_t endpoint_reg = usb_endpoint_read(ep_ix) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR

  uint16_t addr_ptr = (uint16_t)usb_pma_get_addr(ep_ix, ENDPOINT_TX_BUFFER);

  dcd_write_packet_memory(addr_ptr, &(xfer->buffer[xfer->queued_len]), len);
  xfer->queued_len += len;

  usb_pma_set_count(ep_ix, ENDPOINT_TX_BUFFER, len);
  usb_endpoint_status(&endpoint_reg, USB_ENDPOINT_DIRECTION_IN, USB_ENDPOINT_STATE_VALID);

  endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(USB_ENDPOINT_DIRECTION_IN);  // only change TX Status, reserve other toggle bits
  usb_endpoint_write(ep_ix, endpoint_reg, true);
}

static bool edpt_xfer(uint8_t ep_num, usb_endpoint_direction_t dir) {
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  if (dir == USB_ENDPOINT_DIRECTION_IN) {
    dcd_transmit_packet(xfer, ep_idx);
  } else {
    uint32_t endpoint_reg = usb_endpoint_read(ep_idx) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR
    endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(dir);

    uint16_t cnt = min_u16(xfer->total_len, xfer->max_packet_size);

    usb_pma_set_rx_bufsize(ep_idx, ENDPOINT_RX_BUFFER, cnt);

    usb_endpoint_status(&endpoint_reg, dir, USB_ENDPOINT_STATE_VALID);
    usb_endpoint_write(ep_idx, endpoint_reg, true);
  }

  return true;
}

bool dcd_edpt_xfer(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);

  xfer->buffer = buffer;
  xfer->total_len = total_bytes;
  xfer->queued_len = 0;

  return edpt_xfer(ep_num, dir);
}

void dcd_edpt_stall(uint8_t ep_addr) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  uint32_t endpoint_reg = usb_endpoint_read(ep_idx) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR bits
  endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(dir);
  usb_endpoint_status(&endpoint_reg, dir, USB_ENDPOINT_STATE_STALL);

  usb_endpoint_write(ep_idx, endpoint_reg, true);
}

void dcd_edpt_clear_stall(uint8_t ep_addr) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  usb_endpoint_direction_t const dir = usb_endpoint_direction(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  uint32_t endpoint_reg = usb_endpoint_read(ep_idx) | USB_EP_VTTX | USB_EP_VTRX;  // reserve CTR bits
  endpoint_reg &= USB_CHEP_REG_MASK | USB_ENDPOINT_STATUS_MASK(dir) | USB_ENDPOINT_DATA_TOGGLE_MASK(dir);

  usb_endpoint_data_toggle(&endpoint_reg, dir, 0);  // Reset to DATA0
  usb_endpoint_write(ep_idx, endpoint_reg, true);
}

//--------------------------------------------------------------------+
// PMA read/write
//--------------------------------------------------------------------+

// Write to packet memory area (PMA) from user memory
// - Uses unaligned for RAM (since M0 cannot access unaligned address)
static bool dcd_write_packet_memory(uint16_t dst, const void *__restrict src, uint16_t nbytes) {
  if (nbytes == 0) return true;
  uint32_t n_write = nbytes / sizeof(uint32_t);

  usb_pma_buf_t *pma_buf = USB_PMA_BUF_AT(dst);
  const uint8_t *src8 = src;

  while (n_write--) {
    pma_buf->value = tu_unaligned_read32(src8);
    src8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = nbytes & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = 0;
    for (uint16_t i = 0; i < odd; i++) {
      temp |= *src8++ << (i * 8);
    }
    pma_buf->value = temp;
  }

  return true;
}

// Read from packet memory area (PMA) to user memory.
// - Packet memory must be either strictly 32-bit
// - Uses unaligned for RAM (since M0 cannot access unaligned address)
static bool dcd_read_packet_memory(void *__restrict dst, uint16_t src, uint16_t nbytes) {
  if (nbytes == 0) return true;
  uint32_t n_read = nbytes / sizeof(uint32_t);

  usb_pma_buf_t *pma_buf = USB_PMA_BUF_AT(src);
  uint8_t *dst8 = (uint8_t *)dst;

  while (n_read--) {
    tu_unaligned_write32(dst8, (uint32_t)pma_buf->value);
    dst8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = nbytes & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = pma_buf->value;
    while (odd--) {
      *dst8++ = (uint8_t)(temp & 0xfful);
      temp >>= 8;
    }
  }

  return true;
}
