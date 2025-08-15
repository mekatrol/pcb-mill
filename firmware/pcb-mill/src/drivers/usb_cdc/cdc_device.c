#include "usbd.h"
#include "usbd_pvt.h"
#include "circular_buffer.h"
#include "cdc_device.h"
#include "dcd.h"

typedef enum {
  // wValue bits for CDC_REQUEST_SET_CONTROL_LINE_STATE (CDC Spec §6.2.14)
  CDC_CONTROL_LINE_STATE_DTR = (1U << 0U),  // Data Terminal Ready (DTR) signal
  CDC_CONTROL_LINE_STATE_RTS = (1U << 1U)   // Request To Send (RTS) signal
} handshake_state_t;

typedef struct {
  uint8_t interface_num;
  uint8_t ep_in;
  uint8_t ep_out;

  // Meaning:
  // Bit 0 (DTR) — Host is ready (DCE can establish a connection).
  // Bit 1 (RTS) — Host requests the device to prepare for data transmission.
  // Other bits — Reserved, must be set to zero.
  handshake_state_t handshake_state;

  // For a USB CDC Virtual COM Port, this struct represents the Line Coding object defined in USB CDC Specification 1.2, Section 6.2.13.
  usb_cdc_line_coding_t line_coding;

  // TX and RX circular buffer state
  circular_buffer_t rx_buffer;
  circular_buffer_t tx_buffer;

  // TX and RX circulars buffer data
  uint8_t rx_buffer_data[USB_ENDPOINT_RX_BUFFER_SIZE];
  uint8_t tx_buffer_data[USB_ENDPOINT_TX_BUFFER_SIZE];
} usb_cdc_interface_t;

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
} usb_cdc_epbuf_t;

static usb_cdc_interface_t usb_cdc_interface;
static usb_cdc_epbuf_t usb_cdc_epbuf;

static bool _prep_out_transaction() {
  // Skip if usb is not ready yet
  if (!(tud_ready() && usb_cdc_interface.ep_out)) {
    return false;
  }

  // Get rx data available count
  uint16_t available_count = circular_buffer_space(&usb_cdc_interface.rx_buffer);

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  // TODO Actually we can still carry out the transfer, keeping count of received bytes
  // and slowly move it to the buffer when read().
  // This pre-check reduces endpoint claiming
  if (available_count < USB_EP0_BUFFER_SIZE) {
    return false;
  }

  // claim endpoint
  if (!usbd_edpt_claim(usb_cdc_interface.ep_out)) {
    return false;
  }

  // Update available count
  available_count = circular_buffer_space(&usb_cdc_interface.rx_buffer);

  if (available_count >= USB_EP0_BUFFER_SIZE) {
    return usbd_edpt_xfer(usb_cdc_interface.ep_out, usb_cdc_epbuf.epout, USB_EP0_BUFFER_SIZE);
  } else {
    // Release endpoint since we don't make any transfer
    usbd_edpt_release(usb_cdc_interface.ep_out);
    return false;
  }
}

bool usb_cdc_connected() {
  // DTR set means connected
  return tud_ready() && (usb_cdc_interface.handshake_state & CDC_CONTROL_LINE_STATE_DTR) != 0;
}

uint8_t usb_cdc_get_handshake_state() {
  return usb_cdc_interface.handshake_state;
}

void usb_cdc_get_line_coding(usb_cdc_line_coding_t* coding) {
  (*coding) = usb_cdc_interface.line_coding;
}

uint32_t usb_cdc_available() {
  return circular_buffer_count(&usb_cdc_interface.rx_buffer);
}

uint32_t usb_cdc_read(void* buffer, uint32_t bufsize) {
  uint32_t num_read = circular_buffer_read(&usb_cdc_interface.rx_buffer, buffer, bufsize);
  _prep_out_transaction();
  return num_read;
}

uint32_t usb_cdc_write(const uint8_t* buffer, uint32_t bufsize) {
  uint16_t wr_count = circular_buffer_write(&usb_cdc_interface.tx_buffer, buffer, bufsize);

  // flush if queue more than packet size
  if (circular_buffer_count(&usb_cdc_interface.tx_buffer) >= USB_ENDPOINT_TX_BUFFER_SIZE) {
    usb_cdc_write_flush();
  }

  return wr_count;
}

uint32_t usb_cdc_write_flush() {
  usb_cdc_epbuf_t* p_epbuf = &usb_cdc_epbuf;
  // Skip if usb is not ready yet
  if (!tud_ready()) {
    return 0;
  }

  // No data to send
  if (circular_buffer_count(&usb_cdc_interface.tx_buffer) == 0) {
    return 0;
  }

  // Claim the endpoint
  if (!usbd_edpt_claim(usb_cdc_interface.ep_in)) {
    return 0;
  }

  // Pull data from buffer
  const uint16_t count = circular_buffer_read(&usb_cdc_interface.tx_buffer, p_epbuf->epin, USB_EP0_BUFFER_SIZE);

  if (count) {
    if (!usbd_edpt_xfer(usb_cdc_interface.ep_in, p_epbuf->epin, count)) {
      return 0;
    }
    return count;
  } else {
    // Release endpoint since we don't make any transfer
    // Note: data is dropped if terminal is not connected
    usbd_edpt_release(usb_cdc_interface.ep_in);
    return 0;
  }
}

void usb_cdc_init() {
  memset(&usb_cdc_interface, 0, sizeof(usb_cdc_interface_t));

  // default line coding is : stop bit = 1, parity = none, data bits = 8
  usb_cdc_interface.line_coding.dwDTERate = 115200;
  usb_cdc_interface.line_coding.bCharFormat = 0;
  usb_cdc_interface.line_coding.bParityType = 0;
  usb_cdc_interface.line_coding.bDataBits = 8;

  // Config circular buffers
  circular_buffer_init(&usb_cdc_interface.rx_buffer, usb_cdc_interface.rx_buffer_data, (sizeof(usb_cdc_interface.rx_buffer_data) / sizeof(usb_cdc_interface.rx_buffer_data[0])));
  circular_buffer_init(&usb_cdc_interface.tx_buffer, usb_cdc_interface.tx_buffer_data, (sizeof(usb_cdc_interface.tx_buffer_data) / sizeof(usb_cdc_interface.tx_buffer_data[0])));
}

void usb_cdc_reset() {
  circular_buffer_reset(&usb_cdc_interface.rx_buffer);
  circular_buffer_reset(&usb_cdc_interface.tx_buffer);
}

uint16_t usb_cdc_open(const usb_control_interface_descriptor_t* descriptor, uint16_t max_len) {
  // Only support ACM subclass
  if (descriptor->bInterfaceClass != TUSB_CLASS_CDC ||
      descriptor->bInterfaceSubClass != CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL) {
    return 0;
  }

  usb_cdc_interface.interface_num = descriptor->bInterfaceNumber;

  uint16_t drv_len = sizeof(usb_control_interface_descriptor_t);
  const usb_endpoint_descriptor_t* p_desc = (const usb_endpoint_descriptor_t*)tu_desc_next(descriptor);

  // Communication Functional Descriptors
  while (USB_DESC_CS_INTERFACE == tu_desc_type(p_desc) && drv_len <= max_len) {
    drv_len += tu_desc_len(p_desc);
    p_desc = (const usb_endpoint_descriptor_t*)tu_desc_next(p_desc);
  }

  if (USB_DESC_ENDPOINT == tu_desc_type(p_desc)) {
    // notification endpoint
    const usb_endpoint_descriptor_t* endpoint_descriptor = (const usb_endpoint_descriptor_t*)p_desc;
    if (!usb_endpoint_open(endpoint_descriptor)) {
      return 0;
    }

    drv_len += tu_desc_len(p_desc);
    p_desc = (const usb_endpoint_descriptor_t*)tu_desc_next(p_desc);
  }

  //------------- Data Interface (if any) -------------//
  if ((USB_DESC_INTERFACE == tu_desc_type(p_desc)) &&
      (TUSB_CLASS_CDC_DATA == ((const usb_control_interface_descriptor_t*)p_desc)->bInterfaceClass)) {
    // next to endpoint descriptor
    drv_len += tu_desc_len(p_desc);
    p_desc = (const usb_endpoint_descriptor_t*)tu_desc_next(p_desc);

    // Open endpoint pair
    if (!usb_endpoint_open_set((const usb_endpoint_descriptor_t*)p_desc, USB_ENDPOINT_TYPE_BULK, &usb_cdc_interface.ep_out, &usb_cdc_interface.ep_in)) {
      return 0;
    }

    drv_len += 2 * sizeof(usb_endpoint_descriptor_t);
  }

  // Prepare for incoming data
  _prep_out_transaction();

  return drv_len;
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool usb_cdc_control_xfer_cb(uint8_t stage, const usb_control_request_t* request) {
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);

  // Handle class request only
  if (request_type != USB_REQUEST_TYPE_CLASS) {
    return false;
  }

  switch (request->bRequest) {
    case CDC_REQUEST_SET_LINE_CODING:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_xfer(request, &usb_cdc_interface.line_coding, sizeof(usb_cdc_line_coding_t));
      } else if (stage == CONTROL_STAGE_ACK) {
        if (tud_cdc_line_coding_cb) {
          tud_cdc_line_coding_cb(&usb_cdc_interface.line_coding);
        }
      }
      break;

    case CDC_REQUEST_GET_LINE_CODING:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_xfer(request, &usb_cdc_interface.line_coding, sizeof(usb_cdc_line_coding_t));
      }
      break;

    case CDC_REQUEST_SET_CONTROL_LINE_STATE:
      if (stage == CONTROL_STAGE_SETUP) {
        tud_control_status(request);
      } else if (stage == CONTROL_STAGE_ACK) {
        usb_cdc_interface.handshake_state = (uint8_t)request->wValue;

        const bool dtr = (request->wValue & CDC_CONTROL_LINE_STATE_DTR) != 0;
        const bool rts = (request->wValue & CDC_CONTROL_LINE_STATE_RTS) != 0;

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

bool usb_cdc_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes) {
  // Received new data
  if (ep_addr == usb_cdc_interface.ep_out) {
    circular_buffer_write(&usb_cdc_interface.rx_buffer, usb_cdc_epbuf.epout, xferred_bytes);

    // invoke receive callback (if there is still data)
    if (tud_cdc_rx_cb && circular_buffer_count(&usb_cdc_interface.rx_buffer) > 0) {
      tud_cdc_rx_cb();
    }

    // prepare for OUT transaction
    _prep_out_transaction();
  }

  // Data sent to host, we continue to fetch from tx buffer to send.
  // Note: This will cause incorrect baudrate set in line coding.
  //       Though maybe the baudrate is not really important !!!
  if (ep_addr == usb_cdc_interface.ep_in) {
    // invoke transmit callback to possibly refill tx buffer
    if (tud_cdc_tx_complete_cb) {
      tud_cdc_tx_complete_cb();
    }

    if (usb_cdc_write_flush() == 0) {
      // If there is no data left, a ZLP should be sent if
      // xferred_bytes is multiple of EP Packet size and not zero
      if (circular_buffer_count(&usb_cdc_interface.tx_buffer) == 0 && xferred_bytes && (0 == (xferred_bytes & (USB_ENDPOINT_TX_BUFFER_SIZE - 1)))) {
        if (usbd_edpt_claim(usb_cdc_interface.ep_in)) {
          if (!usbd_edpt_xfer(usb_cdc_interface.ep_in, NULL, 0)) {
            return false;
          }
        }
      }
    }
  }

  return true;
}
