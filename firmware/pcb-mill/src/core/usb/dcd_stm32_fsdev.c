#include "tusb_option.h"
#include "dcd.h"
#include "fsdev_stm32.h"

#include "fsdev_type.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

// One of these for every EP IN & OUT, uses a bit of RAM....
typedef struct {
  uint8_t *buffer;
  tu_fifo_t *ff;
  uint16_t total_len;
  uint16_t queued_len;
  uint16_t max_packet_size;
  uint8_t ep_idx;       // index for USB_EPnR register
  bool iso_in_sending;  // Workaround for ISO IN EP doesn't have interrupt mask
} xfer_ctl_t;

// EP allocator
typedef struct {
  uint8_t ep_num;
  uint8_t ep_type;
  bool allocated[2];
} ep_alloc_t;

static xfer_ctl_t xfer_status[CFG_TUD_ENDPPOINT_MAX][2];
static ep_alloc_t ep_alloc_status[CFG_TUD_ENDPPOINT_MAX];
static uint8_t remoteWakeCountdown;  // When wake is requested

//--------------------------------------------------------------------+
// Prototypes
//--------------------------------------------------------------------+

// into the stack.
static void handle_bus_reset();
static void dcd_transmit_packet(xfer_ctl_t *xfer, uint16_t ep_ix);
static bool edpt_xfer(uint8_t ep_num, tusb_dir_t dir);

// PMA allocation/access
static uint16_t ep_buf_ptr;  ///< Points to first free memory location
static uint32_t dcd_pma_alloc(uint16_t len, bool dbuf);
static uint8_t dcd_ep_alloc(uint8_t ep_addr, uint8_t ep_type);
static bool dcd_write_packet_memory(uint16_t dst, const void *__restrict src, uint16_t nbytes);
static bool dcd_read_packet_memory(void *__restrict dst, uint16_t src, uint16_t nbytes);

static bool dcd_write_packet_memory_ff(tu_fifo_t *ff, uint16_t dst, uint16_t wNBytes);
static bool dcd_read_packet_memory_ff(tu_fifo_t *ff, uint16_t src, uint16_t wNBytes);

static void edpt0_open();

__attribute__((always_inline)) static inline void edpt0_prepare_setup(void) {
  btable_set_rx_bufsize(0, BTABLE_BUF_RX, 8);
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
  USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
  USB->CNTR &= ~USB_CNTR_PDWN;
  USB->CNTR = 0;  // Enable USB
  USB->ISTR = 0;  // Clear pending interrupts

  // Reset endpoints to disabled
  for (uint32_t i = 0; i < CFG_TUD_ENDPPOINT_MAX; i++) {
    // This doesn't clear all bits since some bits are "toggle", but does set the type to DISABLED.
    ep_write(i, 0u, false);
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
  dcd_edpt_xfer(TUSB_DIR_IN_MASK | 0x00, NULL, 0);

  // DCD can only set address after status for this request is complete.
  // do it at dcd_edpt0_status_complete()
}

void dcd_remote_wakeup() {
  USB->CNTR |= USB_CNTR_RESUME;
  remoteWakeCountdown = 4u;  // required to be 1 to 15 ms, ESOF should trigger every 1ms.
}

static void handle_bus_reset() {
  USB->DADDR = 0u;  // disable USB Function

  for (uint32_t i = 0; i < CFG_TUD_ENDPPOINT_MAX; i++) {
    // Clear EP allocation status
    ep_alloc_status[i].ep_num = 0xFF;
    ep_alloc_status[i].ep_type = 0xFF;
    ep_alloc_status[i].allocated[0] = false;
    ep_alloc_status[i].allocated[1] = false;
  }

  // Reset PMA allocation (to end of EP buffer table)
  ep_buf_ptr = FSDEV_BTABLE_BASE + 8 * CFG_TUD_ENDPPOINT_MAX;

  edpt0_open();  // open control endpoint (both IN & OUT)

  USB->DADDR = USB_DADDR_EF;  // Enable USB Function
}

// Handle CTR interrupt for the TX/IN direction
static void handle_ctr_tx(uint32_t ep_id) {
  uint32_t ep_reg = ep_read(ep_id) | USB_EP_CTR_TX | USB_EP_CTR_RX;

  uint8_t const ep_num = ep_reg & USB_EPADDR_FIELD;
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, TUSB_DIR_IN);

  if (ep_is_iso(ep_reg)) {
    // Ignore spurious interrupts that we don't schedule
    // host can send IN token while there is no data to send, since ISO does not have NAK
    // this will result to zero length packet --> trigger interrupt (which cannot be masked)
    if (!xfer->iso_in_sending) {
      return;
    }
    xfer->iso_in_sending = false;
    uint8_t buf_id = (ep_reg & USB_EP_DTOG_TX) ? 0 : 1;
    btable_set_count(ep_id, buf_id, 0);
  }

  if (xfer->total_len != xfer->queued_len) {
    dcd_transmit_packet(xfer, ep_id);
  } else {
    dcd_event_xfer_complete(ep_num | TUSB_DIR_IN_MASK, xfer->queued_len, XFER_RESULT_SUCCESS, true);
  }
}

static void handle_ctr_setup(uint32_t ep_id) {
  uint16_t rx_count = btable_get_count(ep_id, BTABLE_BUF_RX);
  uint16_t rx_addr = btable_get_addr(ep_id, BTABLE_BUF_RX);
  uint8_t setup_packet[8] __attribute__((aligned(4)));

  dcd_read_packet_memory(setup_packet, rx_addr, rx_count);

  // Clear CTR RX if another setup packet arrived before this, it will be discarded
  ep_write_clear_ctr(ep_id, TUSB_DIR_OUT);

  // Setup packet should always be 8 bytes. If not, we probably missed the packet
  if (rx_count == 8) {
    dcd_event_setup_received((uint8_t *)setup_packet, true);
    // Hardware should reset EP0 RX/TX to NAK and both toggle to 1
  } else {
    // Missed setup packet !!!
    edpt0_prepare_setup();
  }
}

// Handle CTR interrupt for the RX/OUT direction
static void handle_ctr_rx(uint32_t ep_id) {
  uint32_t ep_reg = ep_read(ep_id) | USB_EP_CTR_TX | USB_EP_CTR_RX;
  uint8_t const ep_num = ep_reg & USB_EPADDR_FIELD;
  bool const is_iso = ep_is_iso(ep_reg);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, TUSB_DIR_OUT);

  uint8_t buf_id;
  if (is_iso) {
    buf_id = (ep_reg & USB_EP_DTOG_RX) ? 0 : 1;  // ISO are double buffered
  } else {
    buf_id = BTABLE_BUF_RX;
  }
  uint16_t const rx_count = btable_get_count(ep_id, buf_id);
  uint16_t pma_addr = (uint16_t)btable_get_addr(ep_id, buf_id);

  if (xfer->ff) {
    dcd_read_packet_memory_ff(xfer->ff, pma_addr, rx_count);
  } else {
    dcd_read_packet_memory(xfer->buffer + xfer->queued_len, pma_addr, rx_count);
  }
  xfer->queued_len += rx_count;

  if ((rx_count < xfer->max_packet_size) || (xfer->queued_len >= xfer->total_len)) {
    // all bytes received or short packet

    // For ch32v203: reset rx bufsize to mps to prevent race condition to cause PMAOVR (occurs with msc write10)
    btable_set_rx_bufsize(ep_id, BTABLE_BUF_RX, xfer->max_packet_size);

    dcd_event_xfer_complete(ep_num, xfer->queued_len, XFER_RESULT_SUCCESS, true);

    // ch32 seems to unconditionally accept ZLP on EP0 OUT, which can incorrectly use queued_len of previous
    // transfer. So reset total_len and queued_len to 0.
    xfer->total_len = xfer->queued_len = 0;
  } else {
    // Set endpoint active again for receiving more data. Note that isochronous endpoints stay active always
    if (!is_iso) {
      uint16_t const cnt = min_u16(xfer->total_len - xfer->queued_len, xfer->max_packet_size);
      btable_set_rx_bufsize(ep_id, BTABLE_BUF_RX, cnt);
    }
    ep_reg &= USB_EPREG_MASK | EP_STAT_MASK(TUSB_DIR_OUT);  // will change RX Status, reserved other toggle bits
    ep_change_status(&ep_reg, TUSB_DIR_OUT, EP_STAT_VALID);
    ep_write(ep_id, ep_reg, false);
  }
}

void dcd_int_handler() {
  uint32_t int_status = USB->ISTR;

  /* Put SOF flag at the beginning of ISR in case to get least amount of jitter if it is used for timing purposes */
  if (int_status & USB_ISTR_SOF) {
    USB->ISTR = ~USB_ISTR_SOF;
    dcd_event_sof(USB->FNR & USB_FNR_FN, true);
  }

  if (int_status & USB_ISTR_RESET) {
    // USBRST is start of reset.
    USB->ISTR = ~USB_ISTR_RESET;
    handle_bus_reset();
    dcd_event_bus_reset(TUSB_SPEED_FULL, true);
    return;  // Don't do the rest of the things here; perhaps they've been cleared?
  }

  if (int_status & USB_ISTR_WKUP) {
    USB->CNTR &= ~USB_CNTR_LPMODE;
    USB->CNTR &= ~USB_CNTR_FSUSP;

    USB->ISTR = ~USB_ISTR_WKUP;
    dcd_event_bus_signal(DCD_EVENT_RESUME, true);
  }

  if (int_status & USB_ISTR_SUSP) {
    /* Suspend is asserted for both suspend and unplug events. without Vbus monitoring,
     * these events cannot be differentiated, so we only trigger suspend. */

    /* Force low-power mode in the macrocell */
    USB->CNTR |= USB_CNTR_FSUSP;
    USB->CNTR |= USB_CNTR_LPMODE;

    /* clear of the ISTR bit must be done after setting of CNTR_FSUSP */
    USB->ISTR = ~USB_ISTR_SUSP;
    dcd_event_bus_signal(DCD_EVENT_SUSPEND, true);
  }

  if (int_status & USB_ISTR_ESOF) {
    if (remoteWakeCountdown == 1u) {
      USB->CNTR &= ~USB_CNTR_RESUME;
    }
    if (remoteWakeCountdown > 0u) {
      remoteWakeCountdown--;
    }
    USB->ISTR = ~USB_ISTR_ESOF;
  }

  // loop to handle all pending CTR interrupts
  while (USB->ISTR & USB_ISTR_CTR) {
    // skip DIR bit, and use CTR TX/RX instead, since there is chance we have both TX/RX completed in one interrupt
    uint32_t const ep_id = USB->ISTR & USB_ISTR_EP_ID;
    uint32_t const ep_reg = ep_read(ep_id);

    if (ep_reg & USB_EP_CTR_RX) {
      /* https://www.st.com/resource/en/errata_sheet/es0561-stm32h503cbebkbrb-device-errata-stmicroelectronics.pdf
       * https://www.st.com/resource/en/errata_sheet/es0587-stm32u535xx-and-stm32u545xx-device-errata-stmicroelectronics.pdf
       * From H503/U535 errata: Buffer description table update completes after CTR interrupt triggers
       * Description:
       * - During OUT transfers, the correct transfer interrupt (CTR) is triggered a little before the last USB SRAM accesses
       * have completed. If the software responds quickly to the interrupt, the full buffer contents may not be correct.
       * Workaround:
       * - Software should ensure that a small delay is included before accessing the SRAM contents. This delay
       * should be 800 ns in Full Speed mode and 6.4 μs in Low Speed mode
       * - Since H5 can run up to 250Mhz -> 1 cycle = 4ns. Per errata, we need to wait 200 cycles. Though executing code
       * also takes time, so we'll wait 60 cycles (count = 20).
       * - Since Low Speed mode is not supported/popular, we will ignore it for now.
       *
       * Note: this errata may also apply to G0, U5, H5 etc.
       */
      volatile uint32_t cycle_count = 20;  // defined as PCD_RX_PMA_CNT in stm32 hal_driver
      while (cycle_count > 0U) {
        cycle_count--;  // each count take 3 cycles (1 for sub, jump, and compare)
      }

      if (ep_reg & USB_EP_SETUP) {
        handle_ctr_setup(ep_id);  // CTR will be clear after copied setup packet
      } else {
        ep_write_clear_ctr(ep_id, TUSB_DIR_OUT);
        handle_ctr_rx(ep_id);
      }
    }

    if (ep_reg & USB_EP_CTR_TX) {
      ep_write_clear_ctr(ep_id, TUSB_DIR_IN);
      handle_ctr_tx(ep_id);
    }
  }

  if (int_status & USB_ISTR_PMAOVR) {
    USB->ISTR = ~USB_ISTR_PMAOVR;
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
  if (ep_buf_ptr > FSDEV_PMA_SIZE) {
    return 0xFFFF;
  }

  return addr;
}

/***
 * Allocate hardware endpoint
 */
static uint8_t dcd_ep_alloc(uint8_t ep_addr, uint8_t ep_type) {
  uint8_t const epnum = tu_edpt_number(ep_addr);
  uint8_t const dir = tu_edpt_dir(ep_addr);

  for (uint8_t i = 0; i < CFG_TUD_ENDPPOINT_MAX; i++) {
    // Check if already allocated
    if (ep_alloc_status[i].allocated[dir] &&
        ep_alloc_status[i].ep_type == ep_type &&
        ep_alloc_status[i].ep_num == epnum) {
      return i;
    }

    // If EP of current direction is not allocated
    // Except for ISO endpoint, both direction should be free
    if (!ep_alloc_status[i].allocated[dir] &&
        (ep_type != TUSB_XFER_ISOCHRONOUS || !ep_alloc_status[i].allocated[dir ^ 1])) {
      // Check if EP number is the same
      if (ep_alloc_status[i].ep_num == 0xFF || ep_alloc_status[i].ep_num == epnum) {
        // One EP pair has to be the same type
        if (ep_alloc_status[i].ep_type == 0xFF || ep_alloc_status[i].ep_type == ep_type) {
          ep_alloc_status[i].ep_num = epnum;
          ep_alloc_status[i].ep_type = ep_type;
          ep_alloc_status[i].allocated[dir] = true;

          return i;
        }
      }
    }
  }

  // Allocation failed
  return 0;
}

void edpt0_open() {
  dcd_ep_alloc(0x0, TUSB_XFER_CONTROL);
  dcd_ep_alloc(0x80, TUSB_XFER_CONTROL);

  xfer_status[0][0].max_packet_size = CFG_TUD_ENDPOINT0_SIZE;
  xfer_status[0][0].ep_idx = 0;

  xfer_status[0][1].max_packet_size = CFG_TUD_ENDPOINT0_SIZE;
  xfer_status[0][1].ep_idx = 0;

  uint16_t pma_addr0 = dcd_pma_alloc(CFG_TUD_ENDPOINT0_SIZE, false);
  uint16_t pma_addr1 = dcd_pma_alloc(CFG_TUD_ENDPOINT0_SIZE, false);

  btable_set_addr(0, BTABLE_BUF_RX, pma_addr0);
  btable_set_addr(0, BTABLE_BUF_TX, pma_addr1);

  uint32_t ep_reg = ep_read(0) & ~USB_EPREG_MASK;  // only get toggle bits
  ep_reg |= USB_EP_CONTROL;
  ep_change_status(&ep_reg, TUSB_DIR_IN, EP_STAT_NAK);
  ep_change_status(&ep_reg, TUSB_DIR_OUT, EP_STAT_NAK);
  // no need to explicitly set DTOG bits since we aren't masked DTOG bit

  edpt0_prepare_setup();  // prepare for setup packet
  ep_write(0, ep_reg, false);
}

bool dcd_edpt_open(tusb_desc_endpoint_t const *desc_ep) {
  uint8_t const ep_addr = desc_ep->bEndpointAddress;
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  tusb_dir_t const dir = tu_edpt_dir(ep_addr);
  const uint16_t packet_size = tu_edpt_packet_size(desc_ep);
  uint8_t const ep_idx = dcd_ep_alloc(ep_addr, desc_ep->bmAttributes.xfer);
  if (ep_idx >= CFG_TUD_ENDPPOINT_MAX) {
    return false;
  }

  uint32_t ep_reg = ep_read(ep_idx) & ~USB_EPREG_MASK;
  ep_reg |= tu_edpt_number(ep_addr) | USB_EP_CTR_TX | USB_EP_CTR_RX;

  // Set type
  switch (desc_ep->bmAttributes.xfer) {
    case TUSB_XFER_BULK:
      ep_reg |= USB_EP_BULK;
      break;
    case TUSB_XFER_INTERRUPT:
      ep_reg |= USB_EP_INTERRUPT;
      break;

    default:
      // Note: ISO endpoint should use alloc / active functions
      return false;
  }

  /* Create a packet memory buffer area. */
  uint16_t pma_addr = dcd_pma_alloc(packet_size, false);
  btable_set_addr(ep_idx, dir == TUSB_DIR_IN ? BTABLE_BUF_TX : BTABLE_BUF_RX, pma_addr);

  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  xfer->max_packet_size = packet_size;
  xfer->ep_idx = ep_idx;

  ep_change_status(&ep_reg, dir, EP_STAT_NAK);
  ep_change_dtog(&ep_reg, dir, 0);

  // reserve other direction toggle bits
  if (dir == TUSB_DIR_IN) {
    ep_reg &= ~(USB_EPRX_STAT | USB_EP_DTOG_RX);
  } else {
    ep_reg &= ~(USB_EPTX_STAT | USB_EP_DTOG_TX);
  }

  ep_write(ep_idx, ep_reg, true);

  return true;
}

void dcd_edpt_close_all() {
  dcd_int_disable();

  for (uint32_t i = 1; i < CFG_TUD_ENDPPOINT_MAX; i++) {
    // Reset endpoint
    ep_write(i, 0, false);
    // Clear EP allocation status
    ep_alloc_status[i].ep_num = 0xFF;
    ep_alloc_status[i].ep_type = 0xFF;
    ep_alloc_status[i].allocated[0] = false;
    ep_alloc_status[i].allocated[1] = false;
  }

  dcd_int_enable();

  // Reset PMA allocation
  ep_buf_ptr = FSDEV_BTABLE_BASE + 8 * CFG_TUD_ENDPPOINT_MAX + 2 * CFG_TUD_ENDPOINT0_SIZE;
}

bool dcd_edpt_iso_alloc(uint8_t ep_addr, uint16_t largest_packet_size) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  uint8_t const dir = tu_edpt_dir(ep_addr);
  uint8_t const ep_idx = dcd_ep_alloc(ep_addr, TUSB_XFER_ISOCHRONOUS);

  /* Create a packet memory buffer area. Enable double buffering for devices with 2048 bytes PMA,
     for smaller devices double buffering occupy too much space. */
#if FSDEV_PMA_SIZE > 1024u
  uint32_t pma_addr = dcd_pma_alloc(largest_packet_size, true);
  uint16_t pma_addr2 = pma_addr >> 16;
#else
  uint32_t pma_addr = dcd_pma_alloc(largest_packet_size, false);
  uint16_t pma_addr2 = pma_addr;
#endif

  btable_set_addr(ep_idx, 0, pma_addr);
  btable_set_addr(ep_idx, 1, pma_addr2);

  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  xfer->ep_idx = ep_idx;

  return true;
}

bool dcd_edpt_iso_activate(tusb_desc_endpoint_t const *desc_ep) {
  uint8_t const ep_addr = desc_ep->bEndpointAddress;
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  tusb_dir_t const dir = tu_edpt_dir(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);

  uint8_t const ep_idx = xfer->ep_idx;

  xfer->max_packet_size = tu_edpt_packet_size(desc_ep);

  uint32_t ep_reg = ep_read(ep_idx) & ~USB_EPREG_MASK;
  ep_reg |= tu_edpt_number(ep_addr) | USB_EP_ISOCHRONOUS | USB_EP_CTR_TX | USB_EP_CTR_RX;
  ep_change_status(&ep_reg, TUSB_DIR_IN, EP_STAT_DISABLED);
  ep_change_status(&ep_reg, TUSB_DIR_OUT, EP_STAT_DISABLED);
  ep_change_dtog(&ep_reg, dir, 0);
  ep_change_dtog(&ep_reg, (tusb_dir_t)(1 - dir), 1);

  ep_write(ep_idx, ep_reg, true);

  return true;
}

// Currently, single-buffered, and only 64 bytes at a time (max)
static void dcd_transmit_packet(xfer_ctl_t *xfer, uint16_t ep_ix) {
  uint16_t len = min_u16(xfer->total_len - xfer->queued_len, xfer->max_packet_size);
  uint32_t ep_reg = ep_read(ep_ix) | USB_EP_CTR_TX | USB_EP_CTR_RX;  // reserve CTR

  bool const is_iso = ep_is_iso(ep_reg);

  uint8_t buf_id;
  if (is_iso) {
    buf_id = (ep_reg & USB_EP_DTOG_TX) ? 1 : 0;
  } else {
    buf_id = BTABLE_BUF_TX;
  }
  uint16_t addr_ptr = (uint16_t)btable_get_addr(ep_ix, buf_id);

  if (xfer->ff) {
    dcd_write_packet_memory_ff(xfer->ff, addr_ptr, len);
  } else {
    dcd_write_packet_memory(addr_ptr, &(xfer->buffer[xfer->queued_len]), len);
  }
  xfer->queued_len += len;

  btable_set_count(ep_ix, buf_id, len);
  ep_change_status(&ep_reg, TUSB_DIR_IN, EP_STAT_VALID);

  if (is_iso) {
    xfer->iso_in_sending = true;
  }
  ep_reg &= USB_EPREG_MASK | EP_STAT_MASK(TUSB_DIR_IN);  // only change TX Status, reserve other toggle bits
  ep_write(ep_ix, ep_reg, true);
}

static bool edpt_xfer(uint8_t ep_num, tusb_dir_t dir) {
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  if (dir == TUSB_DIR_IN) {
    dcd_transmit_packet(xfer, ep_idx);
  } else {
    uint32_t ep_reg = ep_read(ep_idx) | USB_EP_CTR_TX | USB_EP_CTR_RX;  // reserve CTR
    ep_reg &= USB_EPREG_MASK | EP_STAT_MASK(dir);

    uint16_t cnt = min_u16(xfer->total_len, xfer->max_packet_size);

    if (ep_is_iso(ep_reg)) {
      btable_set_rx_bufsize(ep_idx, 0, cnt);
      btable_set_rx_bufsize(ep_idx, 1, cnt);
    } else {
      btable_set_rx_bufsize(ep_idx, BTABLE_BUF_RX, cnt);
    }

    ep_change_status(&ep_reg, dir, EP_STAT_VALID);
    ep_write(ep_idx, ep_reg, true);
  }

  return true;
}

bool dcd_edpt_xfer(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  tusb_dir_t const dir = tu_edpt_dir(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);

  xfer->buffer = buffer;
  xfer->ff = NULL;
  xfer->total_len = total_bytes;
  xfer->queued_len = 0;

  return edpt_xfer(ep_num, dir);
}

bool dcd_edpt_xfer_fifo(uint8_t ep_addr, tu_fifo_t *ff, uint16_t total_bytes) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  tusb_dir_t const dir = tu_edpt_dir(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);

  xfer->buffer = NULL;
  xfer->ff = ff;
  xfer->total_len = total_bytes;
  xfer->queued_len = 0;

  return edpt_xfer(ep_num, dir);
}

void dcd_edpt_stall(uint8_t ep_addr) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  tusb_dir_t const dir = tu_edpt_dir(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  uint32_t ep_reg = ep_read(ep_idx) | USB_EP_CTR_TX | USB_EP_CTR_RX;  // reserve CTR bits
  ep_reg &= USB_EPREG_MASK | EP_STAT_MASK(dir);
  ep_change_status(&ep_reg, dir, EP_STAT_STALL);

  ep_write(ep_idx, ep_reg, true);
}

void dcd_edpt_clear_stall(uint8_t ep_addr) {
  uint8_t const ep_num = tu_edpt_number(ep_addr);
  tusb_dir_t const dir = tu_edpt_dir(ep_addr);
  xfer_ctl_t *xfer = xfer_ctl_ptr(ep_num, dir);
  uint8_t const ep_idx = xfer->ep_idx;

  uint32_t ep_reg = ep_read(ep_idx) | USB_EP_CTR_TX | USB_EP_CTR_RX;  // reserve CTR bits
  ep_reg &= USB_EPREG_MASK | EP_STAT_MASK(dir) | EP_DTOG_MASK(dir);

  if (!ep_is_iso(ep_reg)) {
    ep_change_status(&ep_reg, dir, EP_STAT_NAK);
  }
  ep_change_dtog(&ep_reg, dir, 0);  // Reset to DATA0
  ep_write(ep_idx, ep_reg, true);
}

//--------------------------------------------------------------------+
// PMA read/write
//--------------------------------------------------------------------+

// Write to packet memory area (PMA) from user memory
// - Uses unaligned for RAM (since M0 cannot access unaligned address)
static bool dcd_write_packet_memory(uint16_t dst, const void *__restrict src, uint16_t nbytes) {
  if (nbytes == 0) return true;
  uint32_t n_write = nbytes / sizeof(uint32_t);

  fsdev_pma_buf_t *pma_buf = PMA_BUF_AT(dst);
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

  fsdev_pma_buf_t *pma_buf = PMA_BUF_AT(src);
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

// Write to PMA from FIFO
static bool dcd_write_packet_memory_ff(tu_fifo_t *ff, uint16_t dst, uint16_t wNBytes) {
  if (wNBytes == 0) return true;

  // Since we copy from a ring buffer FIFO, a wrap might occur making it necessary to conduct two copies
  tu_fifo_buffer_info_t info;
  tu_fifo_get_read_info(ff, &info);

  uint16_t cnt_lin = min_u16(wNBytes, info.len_lin);
  uint16_t cnt_wrap = min_u16(wNBytes - cnt_lin, info.len_wrap);
  uint16_t const cnt_total = cnt_lin + cnt_wrap;

  // We want to read from the FIFO and write it into the PMA, if LIN part is ODD and has WRAPPED part,
  // last lin byte will be combined with wrapped part To ensure PMA is always access aligned
  uint16_t lin_even = cnt_lin & ~(sizeof(uint32_t) - 1);
  uint16_t lin_odd = cnt_lin & (sizeof(uint32_t) - 1);
  uint8_t const *src8 = (uint8_t const *)info.ptr_lin;

  // write even linear part
  dcd_write_packet_memory(dst, src8, lin_even);
  dst += lin_even;
  src8 += lin_even;

  if (lin_odd == 0) {
    src8 = (uint8_t const *)info.ptr_wrap;
  } else {
    // Combine last linear bytes + first wrapped bytes to form fsdev bus width data
    uint32_t temp = 0;
    uint16_t i;
    for (i = 0; i < lin_odd; i++) {
      temp |= *src8++ << (i * 8);
    }

    src8 = (uint8_t const *)info.ptr_wrap;
    for (; i < sizeof(uint32_t) && cnt_wrap > 0; i++, cnt_wrap--) {
      temp |= *src8++ << (i * 8);
    }

    dcd_write_packet_memory(dst, &temp, sizeof(uint32_t));
    dst += sizeof(uint32_t);
  }

  // write the rest of the wrapped part
  dcd_write_packet_memory(dst, src8, cnt_wrap);

  tu_fifo_advance_read_pointer(ff, cnt_total);
  return true;
}

// Read from PMA to FIFO
static bool dcd_read_packet_memory_ff(tu_fifo_t *ff, uint16_t src, uint16_t wNBytes) {
  if (wNBytes == 0) return true;

  // Since we copy into a ring buffer FIFO, a wrap might occur making it necessary to conduct two copies
  // Check for first linear part
  tu_fifo_buffer_info_t info;
  tu_fifo_get_write_info(ff, &info);  // We want to read from the FIFO

  uint16_t cnt_lin = min_u16(wNBytes, info.len_lin);
  uint16_t cnt_wrap = min_u16(wNBytes - cnt_lin, info.len_wrap);
  uint16_t cnt_total = cnt_lin + cnt_wrap;

  // We want to read from the FIFO and write it into the PMA, if LIN part is ODD and has WRAPPED part,
  // last lin byte will be combined with wrapped part To ensure PMA is always access aligned

  uint16_t lin_even = cnt_lin & ~(sizeof(uint32_t) - 1);
  uint16_t lin_odd = cnt_lin & (sizeof(uint32_t) - 1);
  uint8_t *dst8 = (uint8_t *)info.ptr_lin;

  // read even linear part
  dcd_read_packet_memory(dst8, src, lin_even);
  dst8 += lin_even;
  src += lin_even;

  if (lin_odd == 0) {
    dst8 = (uint8_t *)info.ptr_wrap;
  } else {
    // Combine last linear bytes + first wrapped bytes to form fsdev bus width data
    uint32_t temp;
    dcd_read_packet_memory(&temp, src, sizeof(uint32_t));
    src += sizeof(uint32_t);

    uint16_t i;
    for (i = 0; i < lin_odd; i++) {
      *dst8++ = (uint8_t)(temp & 0xfful);
      temp >>= 8;
    }

    dst8 = (uint8_t *)info.ptr_wrap;
    for (; i < sizeof(uint32_t) && cnt_wrap > 0; i++, cnt_wrap--) {
      *dst8++ = (uint8_t)(temp & 0xfful);
      temp >>= 8;
    }
  }

  // read the rest of the wrapped part
  dcd_read_packet_memory(dst8, src, cnt_wrap);

  tu_fifo_advance_write_pointer(ff, cnt_total);
  return true;
}
