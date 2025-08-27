#include "usb.h"
#include "usb_cdc.h"
#include "macros.h"

/// CDC ACM (Virtual COM Port) Class-Specific Request Codes
/// See USB CDC Spec 1.2, Table 3.1 (Abstract Control Model Requests)
typedef enum {
  CDC_REQUEST_SET_LINE_CODING = 0x20,         // Set serial line coding (baud rate, stop bits, parity, data bits) :contentReference[oaicite:0]{index=0}
  CDC_REQUEST_GET_LINE_CODING = 0x21,         // Get current serial line coding :contentReference[oaicite:1]{index=1}
  CDC_REQUEST_SET_CONTROL_LINE_STATE = 0x22,  // Control RTS/DTR tone (host signals presence) :contentReference[oaicite:2]{index=2}
  CDC_REQUEST_SEND_BREAK = 0x23               // Transmit break condition on the communication line :contentReference[oaicite:3]{index=3}
} cdc_acm_request_t;

// Staging buffers for CDC IN and OUT endpoints
static __attribute__((aligned(4))) uint8_t ep_out_buffer[USB_CDC_EP_BUFFER_SIZE];
static __attribute__((aligned(4))) uint8_t ep_in_buffer[USB_CDC_EP_BUFFER_SIZE];

// The USB CDC state and config
usb_cdc_t usb_cdc;

static bool usb_cdc_read_ep() {
  // Skip if usb is not yet configured or the ep not yet assigned
  if (!(usb_configured() && usb_cdc.ep_addr_out != 0)) {
    return false;
  }

  // Get available space in buffer
  uint16_t available_space = circular_buffer_space(&usb_cdc.rx_buffer);

  // If the buffer is empty then we can queue more data
  if (available_space >= USB_CDC_EP_BUFFER_SIZE) {
    return usb_ep_queue_transfer(usb_cdc.ep_addr_out, ep_out_buffer, USB_CDC_EP_BUFFER_SIZE);
  }

  return false;
}

uint32_t usb_cdc_write_ep() {
  // Skip if usb is not yet configured or the ep not yet assigned
  if (!(usb_configured() && usb_cdc.ep_addr_in != 0)) {
    return 0;
  }

  // No data to send
  if (circular_buffer_count(&usb_cdc.tx_buffer) == 0) {
    return 0;
  }

  // Read data from circular buffer
  const uint16_t count = circular_buffer_read(&usb_cdc.tx_buffer, ep_in_buffer, USB_CDC_EP_BUFFER_SIZE);

  // Any data?
  if (count > 0) {
    // Write to ep buffers
    return usb_ep_queue_transfer(usb_cdc.ep_addr_in, ep_in_buffer, count) ? count : 0;
  }

  // Nothing written to ep buffers
  return 0;
}

uint16_t usb_cdc_open(const usb_control_interface_descriptor_t* control_descriptor, uint16_t descriptor_end) {
  // Is the a CDC ACM subclass (Virtual COM)?
  if (control_descriptor->bInterfaceClass != USB_CLASS_CDC ||
      control_descriptor->bInterfaceSubClass != CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL) {
    return 0;
  }

  uint16_t descriptor_len = sizeof(usb_control_interface_descriptor_t);
  const usb_ep_descriptor_t* descriptor = (const usb_ep_descriptor_t*)usb_configuration_next_descriptor(control_descriptor);

  // Communication Functional Descriptors
  while (descriptor->bDescriptorType == USB_DESCRIPTOR_TYPE_CS_INTERFACE && descriptor_len <= descriptor_end) {
    descriptor_len += descriptor->bLength;
    descriptor = (const usb_ep_descriptor_t*)usb_configuration_next_descriptor(descriptor);
  }

  if (descriptor->bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT) {
    // notification endpoint
    const usb_ep_descriptor_t* ep_descriptor = (const usb_ep_descriptor_t*)descriptor;
    if (!usb_ep_open_hal(ep_descriptor)) {
      diag_print("usb_ep_open_hal - failed.\r\n");
      return 0;
    }

    descriptor_len += descriptor->bLength;
    descriptor = (const usb_ep_descriptor_t*)usb_configuration_next_descriptor(descriptor);
  }

  if ((descriptor->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) &&
      (((const usb_control_interface_descriptor_t*)descriptor)->bInterfaceClass) == USB_CLASS_CDC_DATA) {
    // Move to endpoint descriptor
    descriptor_len += descriptor->bLength;
    descriptor = (const usb_ep_descriptor_t*)usb_configuration_next_descriptor(descriptor);

    // Open endpoint pair
    if (!usb_ep_open_in_out_pair((const usb_ep_descriptor_t*)descriptor, USB_EP_TYPE_BULK, &usb_cdc.ep_addr_out, &usb_cdc.ep_addr_in)) {
      diag_print("usb_ep_open_in_out_pair - failed.\r\n");
      return 0;
    }

    // Move past end point descriptor pair
    descriptor_len += EP_IN_OUT_PAIR * sizeof(usb_ep_descriptor_t);
  }

  // Read from endpoint RX buffer to CDC RX buffer
  usb_cdc_read_ep();

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
  // Read from endpoint RX buffer to CDC RX buffer
  usb_cdc_read_ep();

  uint32_t read_count = circular_buffer_read(&usb_cdc.rx_buffer, buffer, bufsize);
  return read_count;
}

uint32_t usb_cdc_write(const uint8_t* buffer, uint32_t bufsize) {
  uint16_t write_count = circular_buffer_write(&usb_cdc.tx_buffer, buffer, bufsize);

  // Write from CDC TX buffer endpoint TX buffer
  usb_cdc_write_ep();

  return write_count;
}

bool usb_ep_buffer_transfer(uint8_t ep_addr, uint32_t transferred_bytes) {
  // Received new data
  if (ep_addr == usb_cdc.ep_addr_out) {
    circular_buffer_write(&usb_cdc.rx_buffer, ep_out_buffer, transferred_bytes);

    // invoke receive callback (if there is still data)
    if (usb_cdc_rx_cb && circular_buffer_count(&usb_cdc.rx_buffer) > 0) {
      usb_cdc_rx_cb();
    }

    // Read from endpoint RX buffer to CDC RX buffer
    usb_cdc_read_ep();
  }

  if (ep_addr == usb_cdc.ep_addr_in) {
    // If set, invoke callback in case more TX data needs to be queued prior to testing if sending is complete
    if (usb_cdc_tx_complete_cb) {
      usb_cdc_tx_complete_cb();
    }

    // All TX data sent to host?
    if (usb_cdc_write_ep() == 0 && circular_buffer_count(&usb_cdc.tx_buffer) == 0) {
      // The host knows a transfer is finished when:
      //    It receives a short packet (less than max packet size), or
      //    It receives a Zero-Length Packet (ZLP).

      // E.g. EP max packet (USB_CDC_EP_BUFFER_SIZE) = 64:
      //    If we send 100 bytes, packets are:
      //      64 (full) + 36 (short) → host sees short packet = transfer complete. No ZLP needed.
      //    If we send 128 bytes, packets are:
      //      64 + 64 (both full).
      // Host sees both full → expects more so need a ZLP to terminate transfer.
      if (transferred_bytes != 0 &&                                       // Bytes were transferred
          IS_MULTIPLE_POW2(transferred_bytes, USB_CDC_EP_BUFFER_SIZE)) {  // Transferred byte count is a multiple of USB_CDC_EP_BUFFER_SIZE

        // A ZLP is required to terminate the transfer.
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

  // Set default line coding to 115200bps N81
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
