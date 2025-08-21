#include "feed_forward_buffer.h"
#include "usb.h"
#include "usb_cdc.h"

// Get high or low byte
#define U16_HIGH(u16) ((uint8_t)(((u16) >> 8) & 0x00ff))
#define U16_LOW(u16) ((uint8_t)((u16) & 0x00ff))

enum {
  CONTROL_STAGE_IDLE = 0,  // No control transfer in progress (waiting for SETUP token)
  CONTROL_STAGE_SETUP,     // Setup stage (8-byte setup packet received)
  CONTROL_STAGE_DATA,      // Data stage (if wLength != 0)
                           //   - Host → Device: OUT transactions
                           //   - Device → Host: IN transactions
  CONTROL_STAGE_STATUS     // Status stage (always present, opposite direction of data stage)
                           //   - If Data OUT stage: device responds with IN (zero-length packet)
                           //   - If Data IN stage: host responds with OUT (zero-length packet)
                           //   - If no Data stage: default is IN (ZLP from device)
};

/// CDC ACM (Virtual COM Port) Class-Specific Request Codes
/// See USB CDC Spec 1.2, Table 3.1 (Abstract Control Model Requests)
typedef enum {
  CDC_REQUEST_SET_LINE_CODING = 0x20,         // Set serial line coding (baud rate, stop bits, parity, data bits) :contentReference[oaicite:0]{index=0}
  CDC_REQUEST_GET_LINE_CODING = 0x21,         // Get current serial line coding :contentReference[oaicite:1]{index=1}
  CDC_REQUEST_SET_CONTROL_LINE_STATE = 0x22,  // Control RTS/DTR tone (host signals presence) :contentReference[oaicite:2]{index=2}
  CDC_REQUEST_SEND_BREAK = 0x23               // Transmit break condition on the communication line :contentReference[oaicite:3]{index=3}
} cdc_acm_request_t;

typedef enum {
  USB_CONFIG_REMOTE_WAKEUP_MASK = 1U << 5,
  USB_CONFIG_SELF_POWERED_MASK = 1U << 6,
} usb_configuration_flags;

usbd_device_t usb_device = {
    .self_powered = 1,     // Set to 1 if device is self powered
    .connected = 0,        // Set to 1 if device is connected
    .addressed = 0,        // Set to 1 if device has recieved its address
    .remote_wakeup = 0,    // Set to 1 if remote wakeup is enabled
    .address_pending = 0,  // The pending device address (only set while pending status stage for USB_STD_SET_ADDRESS)
    .address = 0,          // The device address (only valid if addressed == 1)
    .config_num = 0        // 0 means unconfigured
};

typedef bool (*usb_control_transfer_complete_t)(const uint8_t control_stage, const usb_control_request_t* request);

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

ALWAYS_INLINE static bool usb_stage_control_status(const usb_control_request_t* request) {
  // Opposite to endpoint in Data Phase
  const usb_request_direction_index_t request_direction = usb_request_direction(request->bmRequestType);
  const uint8_t ep_addr = request_direction ? USB_DIR_DEVICE_IN_HOST_OUT : USB_DIR_DEVICE_OUT_HOST_IN;

  return usb_ep_queue_transfer(ep_addr, NULL, 0);
}

/*
 * Clear the control transfer request
 */
ALWAYS_INLINE static void usb_control_transfer_reset() {
  memset(&control_transfer, 0, sizeof(usb_control_transfer_t));
}

ALWAYS_INLINE static void usb_control_set_complete_callback(const usb_control_transfer_complete_t cb) {
  control_transfer.control_complete_cb = cb;
}

/*
 * Initialise the control transfer data stage
 */
ALWAYS_INLINE static void usb_control_transfer_init_data_stage(const usb_control_request_t* request) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = NULL;
  control_transfer.feed.fed_count = 0;
  control_transfer.feed.total_count = 0;
}

/*
 * Initialise the control transfer status stage
 */
ALWAYS_INLINE static bool usb_control_init_status_stage(const usb_control_request_t* request) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = NULL;
  control_transfer.feed.fed_count = 0;
  control_transfer.feed.total_count = 0;

  return usb_stage_control_status(request);
}

/*
 * Reset device configuration
 */
ALWAYS_INLINE static void usb_reset_config() {
  usb_cdc_reset();

  usb_device.self_powered = 1;     // Set to 1 if device is self powered
  usb_device.connected = 0;        // Set to 1 if device is connected
  usb_device.addressed = 0;        // Set to 1 if device has recieved its address
  usb_device.remote_wakeup = 0;    // Set to 1 if remote wakeup is enabled
  usb_device.address_pending = 0;  // The pending device address (only set while pending status stage for USB_STD_SET_ADDRESS)
  usb_device.address = 0;          // The device address (only valid if addressed == 1)
  usb_device.config_num = 0;       // 0 means unconfigured
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
      status = (usb_device.self_powered ? 1U : 0U);
      status |= (usb_device.remote_wakeup ? 2U : 0U);
      break;

    case USB_REQUEST_RECIPIENT_INTERFACE:
      // Status always zero for interfaces, nothing to do
      break;

    case USB_REQUEST_RECIPIENT_ENDPOINT: {
      const uint8_t ep_num = USB_EP_IDN(request->wIndex);
      const uint8_t ep_dir_idx = USB_EP_DIR_IDX(request->wIndex);
      bool stalled = usb_ep_stall_get_hal(ep_num, ep_dir_idx);
      // Endpoint status (USB 2.0 Spec, Table 9-6)
      // Bit 0: Halt (1 = STALL, 0 = normal)
      status = stalled ? 1U : 0U;
      break;
    }

    default:
      // Invalid recipient → stall
      return false;
  }

  return usb_ep_initiate_control_response(request, (const uint8_t*)&status, sizeof(status));
}

static bool usb_device_set_address_complete(const uint8_t control_stage, const usb_control_request_t* request) {
  if (control_stage == CONTROL_STAGE_STATUS) {
    usb_device.address = usb_device.address_pending;
    usb_device.addressed = (usb_device.address != 0);
    usb_device.address_pending = 0;
    usb_device_set_addr_hal(usb_device.address);
  }
  return true;
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
  usb_control_set_complete_callback(usb_device_set_address_complete);
  usb_control_init_status_stage(request);

  // Queue status response for ep0
  usb_ep_queue_transfer_hal(EP0_IDN, USB_DIR_DEVICE_OUT_HOST_IN >> 7, NULL, 0);

  return true;
}

static void usb_ep_stall_set(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // only stalled if currently cleared
  usb_ep_stall_set_hal(ep_idn, ep_dir_idx);
}

static void usb_ep_stall_clear(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // only clear if currently stalled
  usb_ep_stall_clear_hal(ep_idn, ep_dir_idx);
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool usb_device_control_transfer(uint8_t control_stage, const usb_control_request_t* request) {
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

//
static bool usb_class_control_staging_init(const usb_control_request_t* request) {
  usb_control_set_complete_callback(usb_device_control_transfer);
  return usb_device_control_transfer(CONTROL_STAGE_SETUP, request);
}

static bool usb_set_configuration() {
  const usb_configuration_descriptor_t* descriptor_config = (const usb_configuration_descriptor_t*)usb_descriptor_configuration();

  if (descriptor_config == NULL || descriptor_config->bDescriptorType != USB_DESCRIPTOR_TYPE_CONFIGURATION) {
    return false;
  }

  // Configuration descriptor
  usb_device.remote_wakeup = (descriptor_config->bmAttributes & USB_CONFIG_REMOTE_WAKEUP_MASK) ? 1U : 0U;
  usb_device.self_powered = (descriptor_config->bmAttributes & USB_CONFIG_SELF_POWERED_MASK) ? 1U : 0U;

  // Interface descriptor
  const uint8_t* discriptor = ((const uint8_t*)descriptor_config) + sizeof(usb_configuration_descriptor_t);
  const uint8_t* descriptor_end = ((const uint8_t*)descriptor_config) + descriptor_config->wTotalLength;

  while (discriptor < descriptor_end) {
    uint8_t assoc_itf_count = 1;

    // Class will always starts with Interface Association (if any) and then Interface descriptor
    if (usb_descriptor_type(discriptor) == USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION) {
      const usb_interface_association_descriptor_t* descriptor_iad = (const usb_interface_association_descriptor_t*)discriptor;
      assoc_itf_count = descriptor_iad->bInterfaceCount;

      discriptor = usb_next_descriptor(discriptor);  // next to Interface
    }

    if (usb_descriptor_type(discriptor) != USB_DESCRIPTOR_TYPE_INTERFACE) {
      return false;
    }

    const usb_control_interface_descriptor_t* descriptor_interface = (const usb_control_interface_descriptor_t*)discriptor;

    // Find driver for this interface
    const uint16_t remaining_len = (uint16_t)(descriptor_end - discriptor);
    const uint16_t drv_len = usb_cdc_open(descriptor_interface, remaining_len);

    if ((sizeof(usb_control_interface_descriptor_t) <= drv_len) && (drv_len <= remaining_len)) {
      if (assoc_itf_count == 1) {
        assoc_itf_count = 2;
      }

      // next Interface
      discriptor += drv_len;

      break;  // exit driver find loop
    }
  }

  return true;
}

// Configure consecutive endpoint descriptors (IN & OUT)
bool usb_ep_open_in_out(const usb_ep_descriptor_t* descriptor_ep, uint8_t transfer_type, uint8_t* ep_addr_out, uint8_t* ep_addr_in) {
  for (int i = 0; i < 2; i++) {
    if (descriptor_ep->bDescriptorType != USB_DESCRIPTOR_TYPE_ENDPOINT || descriptor_ep->bmAttributes.type != transfer_type) {
      return false;
    }

    if (!usb_ep_open_hal(descriptor_ep)) {
      return false;
    }

    if (USB_EP_DIR(descriptor_ep->bEndpointAddress) == USB_DIR_DEVICE_OUT_HOST_IN) {
      (*ep_addr_in) = descriptor_ep->bEndpointAddress;
    } else {
      (*ep_addr_out) = descriptor_ep->bEndpointAddress;
    }

    descriptor_ep = (const usb_ep_descriptor_t*)usb_next_descriptor(descriptor_ep);
  }

  return true;
}

// return descriptor's buffer and update descriptor_len
static bool process_get_descriptor(const usb_control_request_t* request) {
  const usb_descriptor_type_t descriptor_type = (usb_descriptor_type_t)U16_HIGH(request->wValue);
  const uint8_t descriptor_index = U16_LOW(request->wValue);

  switch (descriptor_type) {
    case USB_DESCRIPTOR_TYPE_DEVICE: {
      void* descriptor_device = (void*)(uintptr_t)get_device_descriptor();
      return usb_ep_initiate_control_response(request, descriptor_device, sizeof(usb_device_descriptor_t));
    }

    case USB_DESCRIPTOR_TYPE_CONFIGURATION: {
      usb_configuration_descriptor_t* descriptor_config = (usb_configuration_descriptor_t*)usb_descriptor_configuration(descriptor_index);
      return usb_ep_initiate_control_response(request, (void*)descriptor_config, descriptor_config->wTotalLength);
    }

    case USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIG:
      return false;

    case USB_DESCRIPTOR_TYPE_STRING: {
      const uint8_t* descriptor_str = (const uint8_t*)usb_descriptor_string(descriptor_index);

      // No string matching the descriptor index
      if (!descriptor_str) {
        return false;
      }

      return usb_ep_initiate_control_response(request, (void*)(uintptr_t)descriptor_str, usb_descriptor_len(descriptor_str));
    }

    case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER: {
      const uint8_t* descriptor_qualifier = usb_descriptor_device_qualifier();
      if (!descriptor_qualifier) {
        return false;
      }
      return usb_ep_initiate_control_response(request, (void*)(uintptr_t)descriptor_qualifier, usb_descriptor_len(descriptor_qualifier));
    }

    default:
      return false;
  }
}

/*
 * Full USB reset
 */
void usb_reset() {
  usb_init_function_hal();
  usb_reset_config();
  usb_control_transfer_reset();
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
    usb_control_set_complete_callback(NULL);
    return false;
  }

  switch (request_recipient) {
    case USB_REQUEST_RECIPIENT_DEVICE:
      if (request_type == USB_REQUEST_TYPE_CLASS) {
        return usb_class_control_staging_init(request);
      }

      if (request_type != USB_REQUEST_TYPE_STANDARD) {
        // Non-standard request is not supported
        return false;
      }

      // request->bRequest is of type usb_request_code_t
      switch (request_code) {
        case USB_STD_SET_ADDRESS:
          return usb_device_set_address(request);

        case USB_STD_GET_CONFIGURATION: {
          uint8_t config_num = usb_device.config_num;
          usb_ep_initiate_control_response(request, &config_num, 1);
        } break;

        case USB_STD_SET_CONFIGURATION: {
          const uint8_t config_num = (uint8_t)request->wValue;

          // SET_CONFIGURATION is idempotent — sending the same configuration number twice
          // should not tear down and rebuild everything unnecessarily.
          // If the device is already in configuration N, and host sends SET_CONFIGURATION N again,
          // nothing should change.
          // If the host sends a different configuration number, then the device must tear down
          // all interfaces/endpoints from the old configuration and set up the new ones.

          // config_num changes only when the host explicitly issues SET_CONFIGURATION with a different nonzero value (or 0 to unconfigure).
          // After reset: usb_device.config_num = 0 (unconfigured).
          // When host sets configuration: e.g. SET_CONFIGURATION 1 → now config_num = 1.
          // If host later sets SET_CONFIGURATION 0 → return to unconfigured state (config_num = 0).
          // If host switches to another configuration (e.g. 2) → config_num = 2.
          // Most USB devices only expose one valid configuration descriptor. In that case, the only meaningful transitions are:
          //    0 → 1 (host configures device)
          //    1 → 0 (host unconfigures device)
          // If the device advertised multiple configurations in its device descriptor (bNumConfigurations > 1), then the host could select between them, and config_num could flip between 1, 2, etc.

          if (usb_device.config_num != config_num) {
            // Close any existing configured endpoints
            usb_ep_close_all();

            // Reset all current configuration
            usb_reset_config();

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

        case USB_STD_GET_STATUS:
          return usb_device_get_status(request);

        // Unknown/Unsupported request
        default:
          return false;
      }
      break;

    case USB_REQUEST_RECIPIENT_INTERFACE: {
      // all requests to Interface (STD or Class) is forwarded to class driver.
      // notable requests are: GET HID REPORT DESCRIPTOR, SET_INTERFACE, GET_INTERFACE
      if (!usb_class_control_staging_init(request)) {
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

      if (USB_REQUEST_TYPE_STANDARD != request_type) {
        // Forward class request to its driver
        return usb_class_control_staging_init(request);
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
            usb_class_control_staging_init(request);
            usb_control_set_complete_callback(NULL);
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

  // Status stage complete inidicated by status direction is always
  // opposite of the request/data stage
  if (request_direction != USB_EP_DIR(ep_addr)) {
    // Must be a zero-length packet (ZLP)
    if (transferred_bytes != 0) {
      return false;
    }

    // If the initiator provided a control completion callback then we should call it
    if (control_transfer.control_complete_cb) {
      return control_transfer.control_complete_cb(CONTROL_STAGE_STATUS, &control_transfer.request);
    }

    // Transfer was successful
    return true;
  }

  // If the host is sending the device control data then we need to transfer it
  // from the ep0_control_buffer to the feed buffer
  if (request_direction == USB_DIR_DEVICE_IN_HOST_OUT) {
    // If the originator did not specify a buffer to place the data into then that is an issue
    if (!control_transfer.feed.buffer) {
      return false;
    }

    // Copy the received data to the feed buffer
    memcpy((void*)control_transfer.feed.buffer, ep0_control_buffer, transferred_bytes);
  }

  // Update feed buffer info
  control_transfer.feed.fed_count += (uint16_t)transferred_bytes;
  control_transfer.feed.buffer += transferred_bytes;

  // Data Stage is complete when all request's length are transferred or
  // a short packet is sent including zero-length packet.
  if ((control_transfer.request.wLength == control_transfer.feed.fed_count) ||
      (transferred_bytes <= USB_EP0_BUFFER_SIZE)) {
    // DATA stage is complete
    bool data_stage_complete_ok = true;

    // If the initiator provided a control completion callback then we should call it,
    // the callback can return false to indicate that the device should be stalled
    if (control_transfer.control_complete_cb) {
      data_stage_complete_ok = control_transfer.control_complete_cb(CONTROL_STAGE_DATA, &control_transfer.request);
    }

    if (data_stage_complete_ok) {
      // Data stage complete so send control status
      return usb_stage_control_status(&control_transfer.request);
    }

    // Stall both IN and OUT control endpoint if not OK
    usb_ep_stall_set(USB_DIR_DEVICE_IN_HOST_OUT);
    usb_ep_stall_set(USB_DIR_DEVICE_OUT_HOST_IN);

    return true;
  }

  // More data to transfer, so queue next batch of bytes to send
  return usb_stage_control_data();
}

void usb_device_init() {
  // Reset config to 'unconfigured'
  usb_reset_config();

  // Initialise CDC (Virtual COM) state
  usb_cdc_init();

  // Start USB in device mode
  usb_device_start_hal();
}