#include "dcd.h"
#include "usbd_pvt.h"

typedef struct {
  usb_control_request_t request;
  uint8_t* buffer;
  uint16_t data_len;
  uint16_t total_xferred;
  usbd_control_xfer_cb_t complete_cb;
} usbd_control_xfer_t;

static usbd_control_xfer_t _ctrl_xfer;

static __attribute__((aligned(4))) uint8_t ep0_control_buffer[USB_EP0_BUFFER_SIZE];

static inline bool status_stage_xact(const usb_control_request_t* request) {
  // Opposite to endpoint in Data Phase
  const usb_endpoint_direction_index_t request_direction = usb_request_direction(request->bmRequestType);
  const uint8_t ep_addr = request_direction ? USB_DIR_OUT : USB_DIR_IN;
  return usb_endpoint_transfer(ep_addr, NULL, 0);
}

// Status phase
bool tud_control_status(const usb_control_request_t* request) {
  _ctrl_xfer.request = (*request);
  _ctrl_xfer.buffer = NULL;
  _ctrl_xfer.total_xferred = 0;
  _ctrl_xfer.data_len = 0;

  return status_stage_xact(request);
}

// Queue a transaction in Data Stage
// Each transaction has up to Endpoint0's max packet size.
// This function can also transfer an zero-length packet
static bool data_stage_xact() {
  const uint16_t xact_len = min_u16(_ctrl_xfer.data_len - _ctrl_xfer.total_xferred, USB_EP0_BUFFER_SIZE);
  uint8_t ep_addr = USB_DIR_OUT;

  const usb_endpoint_direction_index_t request_direction = usb_request_direction(_ctrl_xfer.request.bmRequestType);
  if (request_direction == USB_EP_DIRECTION_IN_IDX) {
    ep_addr = USB_DIR_IN;
    if (xact_len) {
      if (xact_len > USB_EP0_BUFFER_SIZE) {
        return false;
      }

      // Copy data to ep0_control_buffer
      memcpy(ep0_control_buffer, _ctrl_xfer.buffer, xact_len);
    }
  }

  return usb_endpoint_transfer(ep_addr, xact_len ? ep0_control_buffer : NULL, xact_len);
}

// Transmit data to/from the control endpoint.
// If the request's wLength is zero, a status packet is sent instead.
bool tud_control_xfer(const usb_control_request_t* request, void* buffer, uint16_t len) {
  _ctrl_xfer.request = (*request);
  _ctrl_xfer.buffer = (uint8_t*)buffer;
  _ctrl_xfer.total_xferred = 0U;
  _ctrl_xfer.data_len = min_u16(len, request->wLength);

  if (request->wLength > 0U) {
    if (_ctrl_xfer.data_len > 0U) {
      if (!buffer) {
        return false;
      }
    }
    if (!data_stage_xact()) {
      return false;
    }
  } else {
    if (!status_stage_xact(request)) {
      return false;
    }
  }

  return true;
}

//--------------------------------------------------------------------+
// USBD API
//--------------------------------------------------------------------+
void usbd_control_set_request(const usb_control_request_t* request);
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp);
bool usbd_control_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes);

void usbd_control_reset(void) {
  memset(&_ctrl_xfer, 0, sizeof(usbd_control_xfer_t));
}

// Set complete callback
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp) {
  _ctrl_xfer.complete_cb = fp;
}

// for dcd_set_address where DCD is responsible for status response
void usbd_control_set_request(const usb_control_request_t* request) {
  _ctrl_xfer.request = (*request);
  _ctrl_xfer.buffer = NULL;
  _ctrl_xfer.total_xferred = 0;
  _ctrl_xfer.data_len = 0;
}

// callback when a transaction complete on
// - DATA stage of control endpoint or
// - Status stage
bool usbd_control_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes) {
  const uint8_t request_direction = USB_EP_DIR(_ctrl_xfer.request.bmRequestType);

  // Endpoint Address is opposite to direction bit, this is Status Stage complete event
  if (USB_EP_DIR(ep_addr) != request_direction) {
    if (xferred_bytes != 0) {
      return false;
    }

    // invoke optional dcd hook if available
    dcd_edpt0_status_complete(&_ctrl_xfer.request);

    if (_ctrl_xfer.complete_cb) {
      // TODO refactor with usbd_driver_print_control_complete_name
      _ctrl_xfer.complete_cb(CONTROL_STAGE_ACK, &_ctrl_xfer.request);
    }

    return true;
  }

  if (request_direction == USB_DIR_OUT) {
    if (!_ctrl_xfer.buffer) {
      return false;
    }
    memcpy(_ctrl_xfer.buffer, ep0_control_buffer, xferred_bytes);
  }

  _ctrl_xfer.total_xferred += (uint16_t)xferred_bytes;
  _ctrl_xfer.buffer += xferred_bytes;

  // Data Stage is complete when all request's length are transferred or
  // a short packet is sent including zero-length packet.
  if ((_ctrl_xfer.request.wLength == _ctrl_xfer.total_xferred) ||
      (xferred_bytes < USB_EP0_BUFFER_SIZE)) {
    // DATA stage is complete
    bool is_ok = true;

    // invoke complete callback if set
    // callback can still stall control in status phase e.g out data does not make sense
    if (_ctrl_xfer.complete_cb) {
      is_ok = _ctrl_xfer.complete_cb(CONTROL_STAGE_DATA, &_ctrl_xfer.request);
    }

    if (is_ok) {
      if (!status_stage_xact(&_ctrl_xfer.request)) {
        return false;
      }
    } else {
      // Stall both IN and OUT control endpoint
      usb_endpoint_stall(USB_DIR_OUT);
      usb_endpoint_stall(USB_DIR_IN);
    }
  } else {
    // More data to transfer
    if (!data_stage_xact()) {
      return false;
    }
  }

  return true;
}
