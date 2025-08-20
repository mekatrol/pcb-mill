#include "usb.h"
#include "usb_cdc.h"

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

static usb_cdc_epbuf_t usb_cdc_epbuf;

// The USB CDC state and config
usb_cdc_t usb_cdc;

// DTR/RTS changed from SET_CONTROL_LINE_STATE
__attribute__((weak)) void usb_cdc_handshake_cb(bool dtr, bool rts) {}

// Line coding change from SET_LINE_CODING
__attribute__((weak)) void usb_cdc_line_coding_cb(const usb_cdc_line_coding_t* p_line_coding) {}

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

uint16_t usb_cdc_open(const usb_control_interface_descriptor_t* control_descriptor, uint16_t max_len) {
  // Only support ACM subclass
  if (control_descriptor->bInterfaceClass != USB_CLASS_CDC ||
      control_descriptor->bInterfaceSubClass != CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL) {
    return 0;
  }

  uint16_t drv_len = sizeof(usb_control_interface_descriptor_t);
  const usb_ep_descriptor_t* descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(control_descriptor);

  // Communication Functional Descriptors
  while (usb_descriptor_type(descriptor) == USB_DESCRIPTOR_TYPE_CS_INTERFACE && drv_len <= max_len) {
    drv_len += usb_descriptor_len(descriptor);
    descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor);
  }

  if (usb_descriptor_type(descriptor) == USB_DESCRIPTOR_TYPE_ENDPOINT) {
    // notification endpoint
    const usb_ep_descriptor_t* ep_descriptor = (const usb_ep_descriptor_t*)descriptor;
    if (!usb_ep_open_hal(ep_descriptor)) {
      return 0;
    }

    drv_len += usb_descriptor_len(descriptor);
    descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor);
  }

  //------------- Data Interface (if any) -------------//
  if ((USB_DESCRIPTOR_TYPE_INTERFACE == usb_descriptor_type(descriptor)) &&
      (USB_CLASS_CDC_DATA == ((const usb_control_interface_descriptor_t*)descriptor)->bInterfaceClass)) {
    // next to endpoint descriptor
    drv_len += usb_descriptor_len(descriptor);
    descriptor = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor);

    // Open endpoint pair
    if (!usb_ep_open_in_out((const usb_ep_descriptor_t*)descriptor, USB_EP_TYPE_BULK, &usb_cdc.ep_addr_out, &usb_cdc.ep_addr_in)) {
      return 0;
    }

    drv_len += 2 * sizeof(usb_ep_descriptor_t);
  }

  // Prepare for incoming data
  usb_device_prep_out_transaction();

  return drv_len;
}

void usb_cdc_get_line_coding(usb_cdc_line_coding_t* coding) {
  (*coding) = usb_cdc.line_coding;
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
