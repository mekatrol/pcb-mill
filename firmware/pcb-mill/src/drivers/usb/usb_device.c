#include "feed_forward_buffer.h"
#include "usb.h"

/*
 * The USB device state
 */
typedef struct {
  volatile uint8_t connected : 1;      // Device is connected
  volatile uint8_t addressed : 1;      // Device has been assigned an address
  volatile uint8_t remote_wakeup : 1;  // Remote wakeup enabled
  volatile uint8_t self_powered : 1;   // Device is self-powered
  volatile uint8_t reserved : 4;       // Padding to make a full byte
  volatile uint8_t address_pending;    // USB device address is pending status stage
  volatile uint8_t address;            // USB device address
} usbd_device_t;

usbd_device_t usb_device = {
    .self_powered = 1,     // Set to 1 if device is self powered
    .connected = 0,        // Set to 1 if device is connected
    .addressed = 0,        // Set to 1 if device has recieved its address
    .remote_wakeup = 0,    // Set to 1 if remote wakeup is enabled
    .address_pending = 0,  // The pending device address (only set while pending status stage for USB_STD_SET_ADDRESS)
    .address = 0,          // The device address (only valid if addressed == 1)
};

typedef bool (*usb_control_transfer_complete_t)(const usb_control_request_t* request);

typedef struct {
  feed_forward_buffer_t feed;                           // "Inherited" fields
  usb_control_request_t request;                        // The control request being transferred
  usb_control_transfer_complete_t control_complete_cb;  // Callback when transfer is complete
} usb_control_transfer_t;

/*
 * The currently active control transfer (if there is one)
 */
static usb_control_transfer_t control_transfer;

/*
 * The buffer used for control requests/responses
 */
static uint8_t ep0_control_buffer[USB_EP0_BUFFER_SIZE];

/*
 * Queue a transfer in HAL
 */
ALWAYS_INLINE static bool usb_ep_queue_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  return usb_ep_queue_transfer_hal(ep_idn, ep_dir_idx, buffer, total_bytes);
}

/*
 * Clear the control transfer request
 */
ALWAYS_INLINE static void usb_control_transfer_clear() {
  memset(&control_transfer, 0, sizeof(usb_control_transfer_t));
}

/*
 * Initialise the control transfer data stage
 */
ALWAYS_INLINE static void usb_control_transfer_init_data_stage(const usb_control_request_t* request, const usb_control_transfer_complete_t cb) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = NULL;
  control_transfer.feed.fed_count = 0;
  control_transfer.feed.total_count = 0;
  control_transfer.control_complete_cb = cb;  // Can be NULL
}

/*
 * Initialise the control transfer status stage
 */
ALWAYS_INLINE static bool usb_control_init_status_stage(const usb_control_request_t* request, const usb_control_transfer_complete_t cb) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = NULL;
  control_transfer.feed.fed_count = 0;
  control_transfer.feed.total_count = 0;
  control_transfer.control_complete_cb = cb;  // Can be NULL

  return usb_stage_control_status(request);
}

/*
 *
 */
static bool usb_stage_control_data() {
  // Calculate the remaining length of data to transfer
  const uint16_t len = feed_forward_remaining_count(&control_transfer.feed, USB_EP0_BUFFER_SIZE);

  // Address for EP0 (assume host OUT)
  uint8_t ep0_addr = (EP0_IDN | USB_DIR_DEVICE_IN_HOST_OUT);

  // Get direction from request
  const usb_request_direction_index_t request_direction = usb_request_direction(control_transfer.request.bmRequestType);

  // Is the direction IN?
  if (request_direction == USB_DIR_DEVICE_OUT_HOST_IN_IDX) {
    // Address for EP0 host IN
    ep0_addr = (EP0_IDN | USB_DIR_DEVICE_OUT_HOST_IN);

    if (len > 0) {
      if (len > USB_EP0_BUFFER_SIZE) {
        return false;
      }

      // Copy data to ep0_control_buffer
      memcpy(ep0_control_buffer, control_transfer.feed.buffer, len);
    }
  }

  return usb_ep_queue_transfer(ep0_addr, len > 0 ? ep0_control_buffer : NULL, len);
}

/*
 * Initiate a control response for a received control request
 */
bool usb_ep_initiate_control_response(const usb_control_request_t* request, const uint8_t* buffer, uint16_t len) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = buffer;
  control_transfer.feed.fed_count = 0;
  control_transfer.feed.total_count = (len < request->wLength) ? len : request->wLength;

  // If the request contains data then commence the data stage for the request
  if (request->wLength > 0U) {
    if (!usb_stage_control_data()) {
      return false;
    }

    return true;
  }

  // This is a status stage and we just need to respond with the response
  return usb_stage_control_status(request);
}

/*
 * Get a devices status: self_powered, remote_wakeup and stalled.
 */
static bool usb_device_get_status(const usb_control_request_t* request) {
  // Default status to zero
  uint16_t status = 0;

  // Get recipient
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  switch (request_recipient) {
    case USB_REQUEST_RECIPIENT_DEVICE:
      // Device status (USB 2.0 Spec, Table 9-4)
      // Bit 0: Self Powered (1 = self-powered, 0 = bus-powered)
      // Bit 1: Remote Wakeup (1 = enabled, 0 = disabled)
      status = (usb_device.self_powered ? 1u : 0u);
      status |= (usb_device.remote_wakeup ? 2u : 0u);
      break;

    case USB_REQUEST_RECIPIENT_INTERFACE:
      // Status always zero for interfaces, nothing to do
      break;

    case USB_REQUEST_RECIPIENT_ENDPOINT: {
      uint8_t ep_num = USB_EP_IDN(request->wIndex);
      bool stalled = usb_ep_stall_get_hal(ep_num);
      // Endpoint status (USB 2.0 Spec, Table 9-6)
      // Bit 0: Halt (1 = STALL, 0 = normal)
      status = stalled ? 1u : 0u;
      break;
    }

    default:
      // Invalid recipient → stall
      return false;
  }

  usb_ep_initiate_control_response(request, &status, sizeof(status));
}

static bool usb_device_set_address_complete(const usb_control_request_t* request) {
  usb_device.address = usb_device.address_pending;
  usb_device.addressed = (usb_device.address != 0);
  usb_device.address_pending = 0;
  usb_device_set_addr_hal(usb_device.address);
}

static bool usb_device_set_address(const usb_control_request_t* request) {
  // Get recipient
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  if (request_recipient != USB_REQUEST_RECIPIENT_DEVICE) {
    // Only valid for device, stall for invalid recipient
    return false;
  }

  // Extract new address (7-bit)
  const uint8_t device_address = (uint8_t)(request->wValue & 0x7F);

  // USB pending address until status complete with ACK
  usb_device.address_pending = device_address;

  // Per USB spec: apply address only after status stage completes
  usb_control_init_status_stage(request, usb_device_set_address_complete);

  // Queue status response for ep0
  usb_ep_queue_transfer_hal(EP0_IDN, USB_DIR_DEVICE_OUT_HOST_IN >> 7, NULL, 0);

  return true;
}

/*
 * All control requests are received on EP0.
 * They handle standard USB requests (enumeration, descriptors, addressing,
 * configuration, and status queries) as well as class- or vendor-specific
 * commands defined by the device.
 *
 * Every control transfer (on EP0) has three possible stages:
 *
 * 1. SETUP stage
 *
 *    Host sends an 8-byte SETUP packet (the request header).
 *    This defines what request is being made (e.g. SET_LINE_CODING).
 *    Based on this, the device prepares to either receive data (OUT),
 *    send data (IN), or skip directly to the status stage.
 *
 * 2. DATA stage (optional)
 *    May be host→device (OUT) or device→host (IN).
 *    For SET_LINE_CODING, the host sends 7 bytes of new line coding parameters in this stage.
 *    For GET_DESCRIPTOR, the device sends descriptor data back.
 *
 * 3. STATUS stage
 *    Always the opposite direction of the DATA stage (or IN if there was no data stage).
 *    A zero-length packet (ZLP) used by the host to acknowledge that the transfer completed successfully.
 */
bool usb_process_control_request(const usb_control_request_t* request) {
  // Can flag as connected as soon as first control request recieved
  usb_device.connected = 1;

  // Get request type
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);

  // Get request code
  const usb_request_code_t request_code = request->bRequest;

  // Get recipient
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  // USB_REQUEST_TYPE_RESERVED should not be used.
  // A host should never send this, but just in case...
  if (request_type == USB_REQUEST_TYPE_RESERVED) {
    return false;
  }

  // Vendor request
  if (request_type == USB_REQUEST_TYPE_VENDOR) {
    // This device driver has no vendor specific descriptors
    return false;
  }

  switch (request_code) {
    case USB_STD_GET_STATUS:
      return usb_device_get_status(request);

    case USB_STD_SET_ADDRESS:
      return usb_device_set_address(request);
  }

  switch (request_recipient) {
    case USB_REQUEST_RECIPIENT_DEVICE:
      if (request_type == USB_REQUEST_TYPE_CLASS) {
        return invoke_class_control(request);
      }

      if (request_type != USB_REQUEST_TYPE_STANDARD) {
        // Non-standard request is not supported
        return false;
      }

      // request->bRequest is of type usb_request_code_t
      switch (request_code) {
        case USB_STD_SET_ADDRESS:
          usb_control_transfer_init_data_stage(request);

          // Respond with status (ep0)
          usb_ep_queue_transfer_hal(EP0_IDN, USB_DIR_DEVICE_OUT_HOST_IN >> 7, NULL, 0);

          // USB has been addressed
          usb_device.address = request->wValue & 0x7F;  // Address 0 - 127 (7 bit)
          usb_device.addressed = 1;
          break;

        case USB_STD_GET_CONFIGURATION: {
          uint8_t config_num = usb_device.config_num;
          usb_ep_initiate_control_response(request, &config_num, 1);
        } break;

        case USB_STD_SET_CONFIGURATION: {
          const uint8_t config_num = (uint8_t)request->wValue;

          // Only process if new configure is different
          if (usb_device.config_num != config_num) {
            if (usb_device.config_num) {
              // disable SOF
              usb_sof_set_enable(false);

              // close all non-control endpoints, cancel all pending transfers if any
              usb_ep_close_all();

              // close all drivers and current configured state except bus speed
              usb_configuration_reset();
            }

            usb_device.config_num = config_num;

            // Handle the new configuration and execute the corresponding callback
            if (config_num) {
              // switch to new configuration if not zero
              if (!usb_set_configuration(config_num)) {
                usb_device.config_num = 0;
                return false;
              }

              // TODO: USB mount
            } else {
              // TODO: USB unmount
            }
          }

          usb_control_init_status_stage(request);
        } break;

        case USB_STD_GET_DESCRIPTOR:
          return process_get_descriptor(request);

        case USB_STD_SET_FEATURE:
          switch (request->wValue) {
            case USB_FEATURE_REMOTE_WAKEUP:
              // Host may enable remote wake up before suspending especially HID device
              usb_device.remote_wakeup = true;
              usb_control_init_status_stage(request);
              break;

            // Stall unsupported feature selector
            default:
              return false;
          }
          break;

        case USB_STD_CLEAR_FEATURE:
          // Only support remote wakeup for device feature
          if (request->wValue != USB_FEATURE_REMOTE_WAKEUP) {
            return false;
          }

          // Host may disable remote wake up after resuming
          usb_device.remote_wakeup = false;
          usb_control_init_status_stage(request);
          break;

        // Unknown/Unsupported request
        default:
          return false;
      }
      break;

    case USB_REQUEST_RECIPIENT_INTERFACE: {
      // all requests to Interface (STD or Class) is forwarded to class driver.
      // notable requests are: GET HID REPORT DESCRIPTOR, SET_INTERFACE, GET_INTERFACE
      if (!invoke_class_control(request)) {
        // For GET_INTERFACE and SET_INTERFACE, it is mandatory to respond even if the class
        // driver doesn't use alternate settings or implement this
        if (USB_REQUEST_TYPE_STANDARD != request_type) {
          return false;
        }

        switch (request->bRequest) {
          case USB_STD_GET_INTERFACE:
          case USB_STD_SET_INTERFACE:
            if (USB_STD_GET_INTERFACE == request->bRequest) {
              uint8_t alternate = 0;
              usb_ep_initiate_control_response(request, &alternate, 1);
            } else {
              usb_control_init_status_stage(request);
            }
            break;

          default:
            return false;
        }
      }
      break;
    }

    case USB_REQUEST_RECIPIENT_ENDPOINT: {
      const uint8_t ep_addr = (uint8_t)(request->wIndex & 0xFF);
      const uint8_t ep_idn = USB_EP_IDN(ep_addr);
      const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

      if (USB_REQUEST_TYPE_STANDARD != request_type) {
        // Forward class request to its driver
        return invoke_class_control(request);
      } else {
        // Handle STD request to endpoint
        switch (request->bRequest) {
          case USB_STD_CLEAR_FEATURE:
          case USB_STD_SET_FEATURE: {
            if (USB_FEATURE_ENDPOINT_HALT == request->wValue) {
              if (USB_STD_CLEAR_FEATURE == request->bRequest) {
                usb_ep_stall_clear(ep_addr);
              } else {
                usb_ep_stall_set(ep_addr);
              }
            }

            // Some classes such as USBTMC needs to clear/re-init its buffer when receiving CLEAR_FEATURE request
            // We will also forward std request targeted endpoint to class drivers as well

            // STD request must always be ACKed regardless of driver returned value
            // Also clear complete callback if driver set since it can also stall the request.
            invoke_class_control(request);
          } break;

          // Unknown/Unsupported request
          default:
            return false;
        }
      }
    } break;

    case USB_REQUEST_RECIPIENT_OTHER:
    default:
      // Unknown to this driver, so return false
      return false;
  }

  return true;
}

bool usb_control_transfer_complete(uint8_t ep_addr, uint32_t transferred_bytes) {
  const uint8_t request_direction = USB_EP_DIR(control_transfer.request.bmRequestType);

  // Status stage complete:
  // - Status direction is always opposite of the request/data stage
  // - Must be a zero-length packet (ZLP)
  if (USB_EP_DIR(ep_addr) != request_direction) {
    if (transferred_bytes != 0) {
      return false;  // Invalid status stage transfer
    }

    // invoke optional dcd hook if available
    usb_ep_control_status_complete(&control_transfer.request);

    if (control_transfer.control_complete_cb) {
      control_transfer.control_complete_cb(&control_transfer.request);
    }

    return true;
  }

  if (request_direction == USB_DIR_DEVICE_IN_HOST_OUT) {
    if (!control_transfer.feed.buffer) {
      return false;
    }

    memcpy(control_transfer.feed.buffer, ep0_control_buffer, transferred_bytes);
  }

  control_transfer.feed.fed_count += (uint16_t)transferred_bytes;
  control_transfer.feed.buffer += transferred_bytes;

  // Data Stage is complete when all request's length are transferred or
  // a short packet is sent including zero-length packet.
  if ((control_transfer.request.wLength == control_transfer.feed.fed_count) ||
      (transferred_bytes < USB_EP0_BUFFER_SIZE)) {
    // DATA stage is complete
    bool is_ok = true;

    // invoke complete callback if set
    // callback can still stall control in status phase e.g out data does not make sense
    if (control_transfer.control_complete_cb) {
      is_ok = control_transfer.control_complete_cb(&control_transfer.request);
    }

    if (is_ok) {
      if (!usb_stage_control_status(&control_transfer.request)) {
        return false;
      }
    } else {
      // Stall both IN and OUT control endpoint
      usb_ep_stall_set(USB_DIR_DEVICE_IN_HOST_OUT);
      usb_ep_stall_set(USB_DIR_DEVICE_OUT_HOST_IN);
    }
  } else {
    // More data to transfer
    if (!usb_stage_control_data()) {
      return false;
    }
  }

  return true;
}