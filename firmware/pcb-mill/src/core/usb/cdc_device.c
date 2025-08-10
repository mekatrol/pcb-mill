#include "usbd.h"
#include "usbd_pvt.h"
#include "circular_buffer.h"
#include "cdc_device.h"

typedef struct {
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;

  uint8_t ep_notify;
  uint8_t line_state;  // Bit 0: DTR, Bit 1: RTS

  char wanted_char;
  __attribute__((aligned(4))) cdc_line_coding_t line_coding;

  // TX and RX circulate buffers
  circular_buffer_t rx_buffer;
  circular_buffer_t tx_buffer;

  uint8_t rx_buf_array[USB_ENDPOINT_RX_BUFFER_SIZE];
  uint8_t tx_buf_array[USB_ENDPOINT_TX_BUFFER_SIZE];
} cdcd_interface_t;

typedef struct {
  union {
    __attribute__((aligned(4)))
    uint8_t epout[USB_EP0_BUFFER_SIZE];

    __attribute__((aligned(1)))
    uint8_t epout_dcache_padding[USB_EP0_BUFFER_SIZE];
  };

  union {
    __attribute__((aligned(4)))
    uint8_t epin[USB_EP0_BUFFER_SIZE];

    __attribute__((aligned(1)))
    uint8_t epin_dcache_padding[USB_EP0_BUFFER_SIZE];
  };
} cdcd_epbuf_t;

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
static cdcd_interface_t _cdcd_itf;
static cdcd_epbuf_t _cdcd_epbuf;

static bool _prep_out_transaction() {
  cdcd_epbuf_t* p_epbuf = &_cdcd_epbuf;

  // Skip if usb is not ready yet
  if (!(tud_ready() && _cdcd_itf.ep_out)) {
    return false;
  }

  uint16_t available = circular_buffer_space(&_cdcd_itf.rx_buffer);

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  // TODO Actually we can still carry out the transfer, keeping count of received bytes
  // and slowly move it to the FIFO when read().
  // This pre-check reduces endpoint claiming
  if (available < USB_EP0_BUFFER_SIZE) {
    return false;
  }

  // claim endpoint
  if (!usbd_edpt_claim(_cdcd_itf.ep_out)) {
    return false;
  }

  // fifo can be changed before endpoint is claimed
  available = circular_buffer_space(&_cdcd_itf.rx_buffer);

  if (available >= USB_EP0_BUFFER_SIZE) {
    return usbd_edpt_xfer(_cdcd_itf.ep_out, p_epbuf->epout, USB_EP0_BUFFER_SIZE);
  } else {
    // Release endpoint since we don't make any transfer
    usbd_edpt_release(_cdcd_itf.ep_out);
    return false;
  }
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_cdc_n_ready() {
  return tud_ready() && _cdcd_itf.ep_in != 0 && _cdcd_itf.ep_out != 0;
}

bool tud_cdc_n_connected() {
  // DTR (bit 0) active  is considered as connected
  return tud_ready() && bit_set_test(_cdcd_itf.line_state, 0);
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
  return circular_buffer_count(&_cdcd_itf.rx_buffer);
}

uint32_t tud_cdc_n_read(void* buffer, uint32_t bufsize) {
  uint32_t num_read = circular_buffer_read(&_cdcd_itf.rx_buffer, buffer, bufsize);
  _prep_out_transaction();
  return num_read;
}

//--------------------------------------------------------------------+
// WRITE API
//--------------------------------------------------------------------+
uint32_t tud_cdc_n_write(const uint8_t* buffer, uint32_t bufsize) {
  uint16_t wr_count = circular_buffer_write(&_cdcd_itf.tx_buffer, buffer, bufsize);

  // flush if queue more than packet size
  if (circular_buffer_count(&_cdcd_itf.tx_buffer) >= USB_ENDPOINT_TX_BUFFER_SIZE) {
    tud_cdc_n_write_flush();
  }

  return wr_count;
}

uint32_t tud_cdc_n_write_flush() {
  cdcd_epbuf_t* p_epbuf = &_cdcd_epbuf;
  // Skip if usb is not ready yet
  if (!tud_ready()) {
    return 0;
  }

  // No data to send
  if (circular_buffer_count(&_cdcd_itf.tx_buffer) == 0) {
    return 0;
  }

  // Claim the endpoint
  if (!usbd_edpt_claim(_cdcd_itf.ep_in)) {
    return 0;
  }

  // Pull data from FIFO
  const uint16_t count = circular_buffer_read(&_cdcd_itf.tx_buffer, p_epbuf->epin, USB_EP0_BUFFER_SIZE);

  if (count) {
    if (!usbd_edpt_xfer(_cdcd_itf.ep_in, p_epbuf->epin, count)) {
      return 0;
    }
    return count;
  } else {
    // Release endpoint since we don't make any transfer
    // Note: data is dropped if terminal is not connected
    usbd_edpt_release(_cdcd_itf.ep_in);
    return 0;
  }
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void cdcd_init() {
  memset(&_cdcd_itf, 0, sizeof(cdcd_interface_t));

  _cdcd_itf.wanted_char = (char)-1;

  // default line coding is : stop bit = 1, parity = none, data bits = 8
  _cdcd_itf.line_coding.bit_rate = 115200;
  _cdcd_itf.line_coding.stop_bits = 0;
  _cdcd_itf.line_coding.parity = 0;
  _cdcd_itf.line_coding.data_bits = 8;

  // Config RX fifo
  circular_buffer_init(&_cdcd_itf.rx_buffer, _cdcd_itf.rx_buf_array, ARRAY_SIZE(_cdcd_itf.rx_buf_array));
  circular_buffer_init(&_cdcd_itf.tx_buffer, _cdcd_itf.tx_buf_array, ARRAY_SIZE(_cdcd_itf.tx_buf_array));
}

bool cdcd_deinit(void) {
  return true;
}

void cdcd_reset() {
  memset(&_cdcd_itf, 0, offsetof(cdcd_interface_t, wanted_char));
  circular_buffer_reset(&_cdcd_itf.rx_buffer);
  circular_buffer_reset(&_cdcd_itf.tx_buffer);
}

uint16_t cdcd_open(const tusb_desc_interface_t* itf_desc, uint16_t max_len) {
  // Only support ACM subclass
  if (itf_desc->bInterfaceClass != TUSB_CLASS_CDC ||
      itf_desc->bInterfaceSubClass != CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL) {
    return 0;
  }

  //------------- Control Interface -------------//
  _cdcd_itf.itf_num = itf_desc->bInterfaceNumber;

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
    if (!usbd_edpt_open(desc_ep)) {
      return 0;
    }
    _cdcd_itf.ep_notify = desc_ep->bEndpointAddress;

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
    if (!usbd_open_edpt_pair(p_desc, 2, TUSB_XFER_BULK, &_cdcd_itf.ep_out, &_cdcd_itf.ep_in)) {
      return 0;
    }

    drv_len += 2 * sizeof(tusb_desc_endpoint_t);
  }

  // Prepare for incoming data
  _prep_out_transaction();

  return drv_len;
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool cdcd_control_xfer_cb(uint8_t stage, const tusb_control_request_t* request) {
  // Handle class request only
  if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_CLASS) {
    return false;
  }

  switch (request->bRequest) {
    case CDC_REQUEST_SET_LINE_CODING:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_xfer(request, &_cdcd_itf.line_coding, sizeof(cdc_line_coding_t));
      } else if (stage == CONTROL_STAGE_ACK) {
        if (tud_cdc_line_coding_cb) {
          tud_cdc_line_coding_cb(&_cdcd_itf.line_coding);
        }
      }
      break;

    case CDC_REQUEST_GET_LINE_CODING:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_xfer(request, &_cdcd_itf.line_coding, sizeof(cdc_line_coding_t));
      }
      break;

    case CDC_REQUEST_SET_CONTROL_LINE_STATE:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_status(request);
      } else if (stage == CONTROL_STAGE_ACK) {
        // CDC PSTN v1.2 section 6.3.12
        // Bit 0: Indicates if DTE is present or not.
        //        This signal corresponds to V.24 signal 108/2 and RS-232 signal DTR (Data Terminal Ready)
        // Bit 1: Carrier control for half-duplex modems.
        //        This signal corresponds to V.24 signal 105 and RS-232 signal RTS (Request to Send)
        bool const dtr = bit_set_test(request->wValue, 0);
        bool const rts = bit_set_test(request->wValue, 1);

        _cdcd_itf.line_state = (uint8_t)request->wValue;

        // Invoke callback
        if (tud_cdc_line_state_cb) {
          tud_cdc_line_state_cb(dtr, rts);
        }
      }
      break;

    case CDC_REQUEST_SEND_BREAK:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_status(request);
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

bool cdcd_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes) {
  // Received new data
  if (ep_addr == _cdcd_itf.ep_out) {
    circular_buffer_write(&_cdcd_itf.rx_buffer, _cdcd_epbuf.epout, xferred_bytes);

    // Check for wanted char and invoke callback if needed
    if (tud_cdc_rx_wanted_cb && (((signed char)_cdcd_itf.wanted_char) != -1)) {
      for (uint32_t i = 0; i < xferred_bytes; i++) {
        if ((_cdcd_itf.wanted_char == _cdcd_epbuf.epout[i]) && circular_buffer_count(&_cdcd_itf.rx_buffer) > 0) {
          tud_cdc_rx_wanted_cb(_cdcd_itf.wanted_char);
        }
      }
    }

    // invoke receive callback (if there is still data)
    if (tud_cdc_rx_cb && circular_buffer_count(&_cdcd_itf.rx_buffer) > 0) {
      tud_cdc_rx_cb();
    }

    // prepare for OUT transaction
    _prep_out_transaction();
  }

  // Data sent to host, we continue to fetch from tx fifo to send.
  // Note: This will cause incorrect baudrate set in line coding.
  //       Though maybe the baudrate is not really important !!!
  if (ep_addr == _cdcd_itf.ep_in) {
    // invoke transmit callback to possibly refill tx fifo
    if (tud_cdc_tx_complete_cb) {
      tud_cdc_tx_complete_cb();
    }

    if (tud_cdc_n_write_flush() == 0) {
      // If there is no data left, a ZLP should be sent if
      // xferred_bytes is multiple of EP Packet size and not zero
      if (circular_buffer_count(&_cdcd_itf.tx_buffer) == 0 && xferred_bytes && (0 == (xferred_bytes & (USB_ENDPOINT_TX_BUFFER_SIZE - 1)))) {
        if (usbd_edpt_claim(_cdcd_itf.ep_in)) {
          if (!usbd_edpt_xfer(_cdcd_itf.ep_in, NULL, 0)) {
            return false;
          }
        }
      }
    }
  }

  // Sent notification to host
  if (ep_addr == _cdcd_itf.ep_notify) {
    if (tud_cdc_notify_complete_cb) {
      tud_cdc_notify_complete_cb();
    }
  }

  return true;
}
