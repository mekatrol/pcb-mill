/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#include "usbd.h"
#include "usbd_pvt.h"

#include "cdc_device.h"

typedef struct {
  uint8_t rhport;
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;

  uint8_t ep_notify;
  uint8_t line_state;  // Bit 0: DTR, Bit 1: RTS

  /*------------- From this point, data is not cleared by bus reset -------------*/
  char wanted_char;
  __attribute__((aligned(4)))
  cdc_line_coding_t line_coding;

  // FIFO
  tu_fifo_t rx_ff;
  tu_fifo_t tx_ff;

  uint8_t rx_ff_buf[CFG_TUD_CDC_RX_BUFSIZE];
  uint8_t tx_ff_buf[CFG_TUD_CDC_TX_BUFSIZE];
} cdcd_interface_t;

#define ITF_MEM_RESET_SIZE offsetof(cdcd_interface_t, wanted_char)

typedef struct {
  TUD_EPBUF_DEF(epout, CFG_TUD_ENDPOINT0_SIZE);
  TUD_EPBUF_DEF(epin, CFG_TUD_ENDPOINT0_SIZE);
} cdcd_epbuf_t;

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
static cdcd_interface_t _cdcd_itf;
static cdcd_epbuf_t _cdcd_epbuf;

static tud_cdc_configure_t _cdcd_cfg = TUD_CDC_CONFIGURE_DEFAULT();

static bool _prep_out_transaction() {
  const uint8_t rhport = 0;
  cdcd_interface_t* p_cdc = &_cdcd_itf;
  cdcd_epbuf_t* p_epbuf = &_cdcd_epbuf;

  // Skip if usb is not ready yet
  TU_VERIFY(tud_ready() && p_cdc->ep_out);

  uint16_t available = tu_fifo_remaining(&p_cdc->rx_ff);

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  // TODO Actually we can still carry out the transfer, keeping count of received bytes
  // and slowly move it to the FIFO when read().
  // This pre-check reduces endpoint claiming
  TU_VERIFY(available >= CFG_TUD_ENDPOINT0_SIZE);

  // claim endpoint
  TU_VERIFY(usbd_edpt_claim(p_cdc->rhport, p_cdc->ep_out));

  // fifo can be changed before endpoint is claimed
  available = tu_fifo_remaining(&p_cdc->rx_ff);

  if (available >= CFG_TUD_ENDPOINT0_SIZE) {
    return usbd_edpt_xfer(rhport, p_cdc->ep_out, p_epbuf->epout, CFG_TUD_ENDPOINT0_SIZE);
  } else {
    // Release endpoint since we don't make any transfer
    usbd_edpt_release(p_cdc->rhport, p_cdc->ep_out);
    return false;
  }
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_cdc_configure(const tud_cdc_configure_t* driver_cfg) {
  TU_VERIFY(driver_cfg);
  _cdcd_cfg = *driver_cfg;
  return true;
}

bool tud_cdc_n_ready() {
  return tud_ready() && _cdcd_itf.ep_in != 0 && _cdcd_itf.ep_out != 0;
}

bool tud_cdc_n_connected() {
  // DTR (bit 0) active  is considered as connected
  return tud_ready() && tu_bit_test(_cdcd_itf.line_state, 0);
}

uint8_t tud_cdc_n_get_line_state() {
  return _cdcd_itf.line_state;
}

void tud_cdc_n_get_line_coding(cdc_line_coding_t* coding) {
  (*coding) = _cdcd_itf.line_coding;
}

void tud_cdc_n_set_wanted_char(char wanted) {
  _cdcd_itf.wanted_char = wanted;
}

//--------------------------------------------------------------------+
// READ API
//--------------------------------------------------------------------+
uint32_t tud_cdc_n_available() {
  return tu_fifo_count(&_cdcd_itf.rx_ff);
}

uint32_t tud_cdc_n_read(void* buffer, uint32_t bufsize) {
  cdcd_interface_t* p_cdc = &_cdcd_itf;
  uint32_t num_read = tu_fifo_read_n(&p_cdc->rx_ff, buffer, (uint16_t)TU_MIN(bufsize, UINT16_MAX));
  _prep_out_transaction();
  return num_read;
}

bool tud_cdc_n_peek(uint8_t* chr) {
  return tu_fifo_peek(&_cdcd_itf.rx_ff, chr);
}

void tud_cdc_n_read_flush() {
  cdcd_interface_t* p_cdc = &_cdcd_itf;
  tu_fifo_clear(&p_cdc->rx_ff);
  _prep_out_transaction();
}

//--------------------------------------------------------------------+
// WRITE API
//--------------------------------------------------------------------+
uint32_t tud_cdc_n_write(const void* buffer, uint32_t bufsize) {
  cdcd_interface_t* p_cdc = &_cdcd_itf;
  uint16_t wr_count = tu_fifo_write_n(&p_cdc->tx_ff, buffer, (uint16_t)TU_MIN(bufsize, UINT16_MAX));

  // flush if queue more than packet size
  if (tu_fifo_count(&p_cdc->tx_ff) >= CFG_TUD_CDC_TX_BUFSIZE) {
    tud_cdc_n_write_flush();
  }

  return wr_count;
}

uint32_t tud_cdc_n_write_flush() {
  cdcd_interface_t* p_cdc = &_cdcd_itf;
  cdcd_epbuf_t* p_epbuf = &_cdcd_epbuf;
  TU_VERIFY(tud_ready(), 0);  // Skip if usb is not ready yet

  // No data to send
  if (!tu_fifo_count(&p_cdc->tx_ff)) {
    return 0;
  }

  TU_VERIFY(usbd_edpt_claim(p_cdc->rhport, p_cdc->ep_in), 0);  // Claim the endpoint

  // Pull data from FIFO
  const uint16_t count = tu_fifo_read_n(&p_cdc->tx_ff, p_epbuf->epin, CFG_TUD_ENDPOINT0_SIZE);

  if (count) {
    TU_ASSERT(usbd_edpt_xfer(p_cdc->rhport, p_cdc->ep_in, p_epbuf->epin, count), 0);
    return count;
  } else {
    // Release endpoint since we don't make any transfer
    // Note: data is dropped if terminal is not connected
    usbd_edpt_release(p_cdc->rhport, p_cdc->ep_in);
    return 0;
  }
}

uint32_t tud_cdc_n_write_available() {
  return tu_fifo_remaining(&_cdcd_itf.tx_ff);
}

bool tud_cdc_n_write_clear() {
  return tu_fifo_clear(&_cdcd_itf.tx_ff);
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void cdcd_init() {
  memset(&_cdcd_itf, 0, sizeof(cdcd_interface_t));
  cdcd_interface_t* p_cdc = &_cdcd_itf;

  p_cdc->wanted_char = (char)-1;

  // default line coding is : stop bit = 1, parity = none, data bits = 8
  p_cdc->line_coding.bit_rate = 115200;
  p_cdc->line_coding.stop_bits = 0;
  p_cdc->line_coding.parity = 0;
  p_cdc->line_coding.data_bits = 8;

  // Config RX fifo
  tu_fifo_config(&p_cdc->rx_ff, p_cdc->rx_ff_buf, TU_ARRAY_SIZE(p_cdc->rx_ff_buf), 1, false);

  // TX fifo can be configured to change to overwritable if not connected (DTR bit not set). Without DTR we do not
  // know if data is actually polled by terminal. This way the most current data is prioritized.
  // Default: is overwritable
  tu_fifo_config(&p_cdc->tx_ff, p_cdc->tx_ff_buf, TU_ARRAY_SIZE(p_cdc->tx_ff_buf), 1, _cdcd_cfg.tx_overwritabe_if_not_connected);
}

bool cdcd_deinit(void) {
  return true;
}

void cdcd_reset(uint8_t rhport) {
  (void)rhport;

  cdcd_interface_t* p_cdc = &_cdcd_itf;

  memset(p_cdc, 0, ITF_MEM_RESET_SIZE);
  if (!_cdcd_cfg.rx_persistent) {
    tu_fifo_clear(&p_cdc->rx_ff);
  }
  if (!_cdcd_cfg.tx_persistent) {
    tu_fifo_clear(&p_cdc->tx_ff);
  }
  tu_fifo_set_overwritable(&p_cdc->tx_ff, _cdcd_cfg.tx_overwritabe_if_not_connected);
}

uint16_t cdcd_open(uint8_t rhport, const tusb_desc_interface_t* itf_desc, uint16_t max_len) {
  // Only support ACM subclass
  TU_VERIFY(TUSB_CLASS_CDC == itf_desc->bInterfaceClass &&
                CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL == itf_desc->bInterfaceSubClass,
            0);

  // Find available interface
  cdcd_interface_t* p_cdc;
  p_cdc = &_cdcd_itf;

  //------------- Control Interface -------------//
  p_cdc->rhport = rhport;
  p_cdc->itf_num = itf_desc->bInterfaceNumber;

  uint16_t drv_len = sizeof(tusb_desc_interface_t);
  const uint8_t* p_desc = tu_desc_next(itf_desc);

  // Communication Functional Descriptors
  while (TUSB_DESC_CS_INTERFACE == tu_desc_type(p_desc) && drv_len <= max_len) {
    drv_len += tu_desc_len(p_desc);
    p_desc = tu_desc_next(p_desc);
  }

  if (TUSB_DESC_ENDPOINT == tu_desc_type(p_desc)) {
    // notification endpoint
    const tusb_desc_endpoint_t* desc_ep = (const tusb_desc_endpoint_t*)p_desc;
    TU_ASSERT(usbd_edpt_open(rhport, desc_ep), 0);
    p_cdc->ep_notify = desc_ep->bEndpointAddress;

    drv_len += tu_desc_len(p_desc);
    p_desc = tu_desc_next(p_desc);
  }

  //------------- Data Interface (if any) -------------//
  if ((TUSB_DESC_INTERFACE == tu_desc_type(p_desc)) &&
      (TUSB_CLASS_CDC_DATA == ((const tusb_desc_interface_t*)p_desc)->bInterfaceClass)) {
    // next to endpoint descriptor
    drv_len += tu_desc_len(p_desc);
    p_desc = tu_desc_next(p_desc);

    // Open endpoint pair
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, 2, TUSB_XFER_BULK, &p_cdc->ep_out, &p_cdc->ep_in), 0);

    drv_len += 2 * sizeof(tusb_desc_endpoint_t);
  }

  // Prepare for incoming data
  _prep_out_transaction();

  return drv_len;
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool cdcd_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t* request) {
  // Handle class request only
  TU_VERIFY(request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS);

  cdcd_interface_t* p_cdc;

  // Identify which interface to use
  p_cdc = &_cdcd_itf;

  switch (request->bRequest) {
    case CDC_REQUEST_SET_LINE_CODING:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_xfer(rhport, request, &p_cdc->line_coding, sizeof(cdc_line_coding_t));
      } else if (stage == CONTROL_STAGE_ACK) {
        if (tud_cdc_line_coding_cb) {
          tud_cdc_line_coding_cb(&p_cdc->line_coding);
        }
      }
      break;

    case CDC_REQUEST_GET_LINE_CODING:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_xfer(rhport, request, &p_cdc->line_coding, sizeof(cdc_line_coding_t));
      }
      break;

    case CDC_REQUEST_SET_CONTROL_LINE_STATE:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_status(rhport, request);
      } else if (stage == CONTROL_STAGE_ACK) {
        // CDC PSTN v1.2 section 6.3.12
        // Bit 0: Indicates if DTE is present or not.
        //        This signal corresponds to V.24 signal 108/2 and RS-232 signal DTR (Data Terminal Ready)
        // Bit 1: Carrier control for half-duplex modems.
        //        This signal corresponds to V.24 signal 105 and RS-232 signal RTS (Request to Send)
        bool const dtr = tu_bit_test(request->wValue, 0);
        bool const rts = tu_bit_test(request->wValue, 1);

        p_cdc->line_state = (uint8_t)request->wValue;

        // If enabled: fifo overwriting is disabled if DTR bit is set and vice versa
        if (_cdcd_cfg.tx_overwritabe_if_not_connected) {
          tu_fifo_set_overwritable(&p_cdc->tx_ff, !dtr);
        } else {
          tu_fifo_set_overwritable(&p_cdc->tx_ff, false);
        }

        // Invoke callback
        if (tud_cdc_line_state_cb) {
          tud_cdc_line_state_cb(dtr, rts);
        }
      }
      break;

    case CDC_REQUEST_SEND_BREAK:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_status(rhport, request);
      } else if (stage == CONTROL_STAGE_ACK) {
        if (tud_cdc_send_break_cb) {
          tud_cdc_send_break_cb(request->wValue);
        }
      }
      break;

    default:
      return false;  // stall unsupported request
  }

  return true;
}

bool cdcd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
  (void)result;

  cdcd_interface_t* p_cdc;

  // Identify which interface to use
  p_cdc = &_cdcd_itf;
  cdcd_epbuf_t* p_epbuf = &_cdcd_epbuf;

  // Received new data
  if (ep_addr == p_cdc->ep_out) {
    tu_fifo_write_n(&p_cdc->rx_ff, p_epbuf->epout, (uint16_t)xferred_bytes);

    // Check for wanted char and invoke callback if needed
    if (tud_cdc_rx_wanted_cb && (((signed char)p_cdc->wanted_char) != -1)) {
      for (uint32_t i = 0; i < xferred_bytes; i++) {
        if ((p_cdc->wanted_char == p_epbuf->epout[i]) && !tu_fifo_empty(&p_cdc->rx_ff)) {
          tud_cdc_rx_wanted_cb(p_cdc->wanted_char);
        }
      }
    }

    // invoke receive callback (if there is still data)
    if (tud_cdc_rx_cb && !tu_fifo_empty(&p_cdc->rx_ff)) {
      tud_cdc_rx_cb();
    }

    // prepare for OUT transaction
    _prep_out_transaction();
  }

  // Data sent to host, we continue to fetch from tx fifo to send.
  // Note: This will cause incorrect baudrate set in line coding.
  //       Though maybe the baudrate is not really important !!!
  if (ep_addr == p_cdc->ep_in) {
    // invoke transmit callback to possibly refill tx fifo
    if (tud_cdc_tx_complete_cb) {
      tud_cdc_tx_complete_cb();
    }

    if (0 == tud_cdc_n_write_flush()) {
      // If there is no data left, a ZLP should be sent if
      // xferred_bytes is multiple of EP Packet size and not zero
      if (!tu_fifo_count(&p_cdc->tx_ff) && xferred_bytes && (0 == (xferred_bytes & (CFG_TUD_CDC_TX_BUFSIZE - 1)))) {
        if (usbd_edpt_claim(rhport, p_cdc->ep_in)) {
          TU_ASSERT(usbd_edpt_xfer(rhport, p_cdc->ep_in, NULL, 0));
        }
      }
    }
  }

  // Sent notification to host
  if (ep_addr == p_cdc->ep_notify) {
    if (tud_cdc_notify_complete_cb) {
      tud_cdc_notify_complete_cb();
    }
  }

  return true;
}
