#include "usb.h"
#include "usb_cdc.h"

/// CDC ACM (Virtual COM Port) Class-Specific Request Codes
/// See USB CDC Spec 1.2, Table 3.1 (Abstract Control Model Requests)
typedef enum {
  CDC_REQUEST_SET_LINE_CODING = 0x20,         // Set serial line coding (baud rate, stop bits, parity, data bits) :contentReference[oaicite:0]{index=0}
  CDC_REQUEST_GET_LINE_CODING = 0x21,         // Get current serial line coding :contentReference[oaicite:1]{index=1}
  CDC_REQUEST_SET_CONTROL_LINE_STATE = 0x22,  // Control RTS/DTR tone (host signals presence) :contentReference[oaicite:2]{index=2}
  CDC_REQUEST_SEND_BREAK = 0x23               // Transmit break condition on the communication line :contentReference[oaicite:3]{index=3}
} cdc_acm_request_t;

typedef struct {
  __attribute__((aligned(4))) uint8_t epout[USB_EP0_BUFFER_SIZE];
  __attribute__((aligned(4))) uint8_t epin[USB_EP0_BUFFER_SIZE];
} usb_cdc_epbuf_t;

static usb_cdc_epbuf_t usb_cdc_epbuf;

// The USB CDC state and config
usb_cdc_t usb_cdc;

static bool usb_device_prep_out_transaction() {
  // Skip if usb is not yet configured
  if (!(usb_configured() && usb_cdc.ep_addr_out)) {
    return false;
  }

  // Get rx data available count
  uint16_t available_count = circular_buffer_space(&usb_cdc.rx_buffer);

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  // TODO Actually we can still carry out the transfer, keeping count of received bytes
  // and slowly move it to the buffer when read().
  // This pre-check reduces endpoint claiming
  if (available_count < USB_EP0_BUFFER_SIZE) {
    return false;
  }

  // Update available count
  available_count = circular_buffer_space(&usb_cdc.rx_buffer);

  if (available_count >= USB_EP0_BUFFER_SIZE) {
    return usb_ep_queue_transfer(usb_cdc.ep_addr_out, usb_cdc_epbuf.epout, USB_EP0_BUFFER_SIZE);
  } else {
    return false;
  }
}

uint16_t usb_cdc_open(const usb_control_interface_descriptor_t* control_descriptor, uint16_t descriptor_end) {
  // Only support ACM subclass
  if (control_descriptor->bInterfaceClass != USB_CLASS_CDC ||
      control_descriptor->bInterfaceSubClass != CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL) {
    return 0;
  }

  uint16_t descriptor_len = sizeof(usb_control_interface_descriptor_t);
  const usb_ep_descriptor_t* descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(control_descriptor);

  // Communication Functional Descriptors
  while (((const usb_descriptor_base_t*)descriptor)->bDescriptorType == USB_DESCRIPTOR_TYPE_CS_INTERFACE && descriptor_len <= descriptor_end) {
    descriptor_len += usb_descriptor_len(descriptor);
    descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor);
  }

  if (((const usb_descriptor_base_t*)descriptor)->bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT) {
    // notification endpoint
    const usb_ep_descriptor_t* ep_descriptor = (const usb_ep_descriptor_t*)descriptor;
    if (!usb_ep_open_hal(ep_descriptor)) {
      return 0;
    }

    descriptor_len += usb_descriptor_len(descriptor);
    descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor);
  }

  if ((((const usb_descriptor_base_t*)descriptor)->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) &&
      (((const usb_control_interface_descriptor_t*)descriptor)->bInterfaceClass) == USB_CLASS_CDC_DATA) {
    // next to endpoint descriptor
    descriptor_len += usb_descriptor_len(descriptor);
    descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor);

    // Open endpoint pair
    if (!usb_ep_open_in_out_pair((const usb_ep_descriptor_t*)descriptor, USB_EP_TYPE_BULK, &usb_cdc.ep_addr_out, &usb_cdc.ep_addr_in)) {
      return 0;
    }

    descriptor_len += 2 * sizeof(usb_ep_descriptor_t);
  }

  // Prepare for incoming data
  usb_device_prep_out_transaction();

  return descriptor_len;
}

bool usb_cdc_control_transfer(uint8_t control_stage, const usb_control_request_t* request) {
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);

  // Handle class request only
  if (request_type != USB_REQUEST_TYPE_CLASS) {
    return false;
  }

  switch (request->bRequest) {
    case CDC_REQUEST_SET_LINE_CODING:
      if (control_stage == CONTROL_STAGE_SETUP) {
        usb_ep_initiate_control_response(request, (const uint8_t*)&usb_cdc.line_coding, sizeof(usb_cdc_line_coding_t));
      } else if (control_stage == CONTROL_STAGE_STATUS) {
        if (usb_cdc_line_coding_cb) {
          usb_cdc_line_coding_cb(&usb_cdc.line_coding);
        }
      }
      break;

    case CDC_REQUEST_GET_LINE_CODING:
      if (control_stage == CONTROL_STAGE_SETUP) {
        usb_ep_initiate_control_response(request, (const uint8_t*)&usb_cdc.line_coding, sizeof(usb_cdc_line_coding_t));
      }
      break;

    case CDC_REQUEST_SET_CONTROL_LINE_STATE:
      if (control_stage == CONTROL_STAGE_SETUP) {
        usb_control_init_status_stage(request);
      } else if (control_stage == CONTROL_STAGE_STATUS) {
        usb_cdc.flow_control_state = (uint8_t)request->wValue;

        const bool dtr = (request->wValue & CDC_CONTROL_LINE_STATE_DTR) != 0;
        const bool rts = (request->wValue & CDC_CONTROL_LINE_STATE_RTS) != 0;

        // Invoke callback
        if (usb_cdc_handshake_cb) {
          usb_cdc_handshake_cb(dtr, rts);
        }
      }
      break;

    case CDC_REQUEST_SEND_BREAK:
      if (control_stage == CONTROL_STAGE_SETUP) {
        usb_control_init_status_stage(request);
      } else if (control_stage == CONTROL_STAGE_STATUS) {
      }
      break;

    default:
      return false;  // stall unsupported request
  }

  return true;
}

uint32_t usb_cdc_available() {
  return circular_buffer_count(&usb_cdc.rx_buffer);
}

uint32_t usb_cdc_read(void* buffer, uint32_t bufsize) {
  uint32_t num_read = circular_buffer_read(&usb_cdc.rx_buffer, buffer, bufsize);
  usb_device_prep_out_transaction();
  return num_read;
}

uint32_t usb_cdc_write(const uint8_t* buffer, uint32_t bufsize) {
  uint16_t wr_count = circular_buffer_write(&usb_cdc.tx_buffer, buffer, bufsize);

  // flush if queue more than packet size
  if (circular_buffer_count(&usb_cdc.tx_buffer) >= USB_CDC_EP_BUFFER_SIZE) {
    usb_cdc_write_flush();
  }

  return wr_count;
}

uint32_t usb_cdc_write_flush() {
  // Skip if usb is not ready yet
  if (!usb_configured()) {
    return 0;
  }

  // No data to send
  if (circular_buffer_count(&usb_cdc.tx_buffer) == 0) {
    return 0;
  }

  // Pull data from buffer
  const uint16_t count = circular_buffer_read(&usb_cdc.tx_buffer, usb_cdc_epbuf.epin, USB_EP0_BUFFER_SIZE);

  if (count) {
    if (!usb_ep_queue_transfer(usb_cdc.ep_addr_in, usb_cdc_epbuf.epin, count)) {
      return 0;
    }
    return count;
  } else {
    return 0;
  }
}

bool usb_cdc_transfer(uint8_t ep_addr, uint32_t transferred_bytes) {
  // Received new data
  if (ep_addr == usb_cdc.ep_addr_out) {
    circular_buffer_write(&usb_cdc.rx_buffer, usb_cdc_epbuf.epout, transferred_bytes);

    // invoke receive callback (if there is still data)
    if (usb_cdc_rx_cb && circular_buffer_count(&usb_cdc.rx_buffer) > 0) {
      usb_cdc_rx_cb();
    }

    // prepare for OUT transaction
    usb_device_prep_out_transaction();
  }

  // Data sent to host, we continue to fetch from tx buffer to send.
  // Note: This will cause incorrect baudrate set in line coding.
  //       Though maybe the baudrate is not really important !!!
  if (ep_addr == usb_cdc.ep_addr_in) {
    // invoke transmit callback to possibly refill tx buffer
    if (usb_cdc_tx_complete_cb) {
      usb_cdc_tx_complete_cb();
    }

    if (usb_cdc_write_flush() == 0) {
      // If there is no data left, a ZLP should be sent if
      // transferred_bytes is multiple of EP Packet size and not zero
      if (circular_buffer_count(&usb_cdc.tx_buffer) == 0 && transferred_bytes && (0 == (transferred_bytes & (USB_CDC_EP_BUFFER_SIZE - 1)))) {
        if (!usb_ep_queue_transfer(usb_cdc.ep_addr_in, NULL, 0)) {
          return false;
        }
      }
    }
  }

  return true;
}

void usb_cdc_init() {
  memset(&usb_cdc, 0, sizeof(usb_cdc_t));

  // default line coding is : stop bit = 1, parity = none, data bits = 8
  usb_cdc.line_coding.dwDTERate = 115200;
  usb_cdc.line_coding.bCharFormat = 0;
  usb_cdc.line_coding.bParityType = 0;
  usb_cdc.line_coding.bDataBits = 8;

  // Config circular buffers
  circular_buffer_init(&usb_cdc.rx_buffer, usb_cdc.rx_buffer_data, (sizeof(usb_cdc.rx_buffer_data) / sizeof(usb_cdc.rx_buffer_data[0])));
  circular_buffer_init(&usb_cdc.tx_buffer, usb_cdc.tx_buffer_data, (sizeof(usb_cdc.tx_buffer_data) / sizeof(usb_cdc.tx_buffer_data[0])));
}

void usb_cdc_reset() {
  circular_buffer_reset(&usb_cdc.rx_buffer);
  circular_buffer_reset(&usb_cdc.tx_buffer);
}
