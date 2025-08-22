#include "usb.h"
#include "diagnostics.h"
#include "usb_cdc.h"
#include "feed_forward_buffer.h"
#include "stm32g0xx.h"

enum {
  USB_DESCRIPTOR_TYPE_CONFIG_ATT_REMOTE_WAKEUP = 1U << 5,
  USB_DESCRIPTOR_TYPE_CONFIG_ATT_SELF_POWERED = 1U << 6,
};

typedef enum {
  USB_BMATTR_RESERVED_D7 = 0x80,      ///< Bit 7: Reserved, must always be set to 1
  USB_BMATTR_SELF_POWERED = 0x40,     ///< Bit 6: 1 = Device is self-powered, 0 = bus-powered
  USB_BMATTR_REMOTE_WAKEUP = 0x20,    ///< Bit 5: 1 = Device can wake host from suspend
  USB_BMATTR_RESERVED_D4_TO_0 = 0x1F  ///< Bits 4..0: Reserved, must be 0
} usb_bm_attributes_mask_t;

typedef enum {
  USB_CONFIG_REMOTE_WAKEUP_MASK = 1 << 5U,
  USB_CONFIG_SELF_POWERED_MASK = 1 << 6U,
} usb_configuration_flags;

// We are a single USB device
usb_device_t usb_device;

typedef bool (*usb_control_transfer_complete_t)(const uint8_t control_stage, const usb_control_request_t* request);

typedef struct {
  feed_forward_buffer_t feed;                           // "Inherited" fields
  usb_control_request_t request;                        // The control request being transferred
  usb_control_transfer_complete_t control_complete_cb;  // Callback when transfer is complete
} usb_control_transfer_t;

static usb_control_transfer_t control_transfer;

static __attribute__((aligned(4))) uint8_t ep0_control_buffer[USB_EP0_BUFFER_SIZE];

// Will return next interfac descriptor
ALWAYS_INLINE static const usb_interface_association_descriptor_t* next_interface(const usb_interface_association_descriptor_t* interface_assoc) {
  return (const usb_interface_association_descriptor_t*)(interface_assoc + interface_assoc->bLength);
}

// Get high or low byte
#define U16_HIGH(_u16) ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16) ((uint8_t)((_u16) & 0x00ff))

static bool usb_set_configuration();
static bool process_get_descriptor(const usb_control_request_t* request);

// from usbd_control.c
void usb_control_transfer_reset(void);
void usb_control_set_complete_callback(usb_control_transfer_complete_t fp);

bool usb_connected(void) {
  return usb_device.connected;
}

void usb_device_init() {
  // Reset device configuration
  memset(&usb_device, 0, sizeof(usb_device_t));

  // Init class drivers
  usb_cdc_init();

  // Init device controller driver
  usb_device_start_hal();

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

void usb_configuration_reset() {
  // Reset CDC
  usb_cdc_reset();

  // Reset all members
  memset(&usb_device, 0, sizeof(usb_device_t));
}

static bool usb_class_control_staging_init(const usb_control_request_t* request) {
  usb_control_set_complete_callback(usb_cdc_control_transfer);
  return usb_cdc_control_transfer(CONTROL_STAGE_SETUP, request);
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

/*
 * Full USB reset
 */
void usb_reset() {
  usb_init_enable_hal();
  usb_configuration_reset();
  usb_control_transfer_reset();
}

void usb_ep_stall_set(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  usb_ep_stall_set_hal(ep_idn, ep_dir_idx);
}

void usb_ep_stall_clear(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  usb_ep_stall_clear_hal(ep_idn, ep_dir_idx);
}

bool usb_process_control_request(const usb_control_request_t* request) {
  // Make sure control callback cleared
  usb_control_set_complete_callback(NULL);

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
            usb_configuration_reset();

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
          // Device status bit mask
          // - Bit 0: Self Powered
          // - Bit 1: Remote Wakeup enabled
          uint16_t status = (uint16_t)((1u) | (usb_device.remote_wakeup ? 2U : 0U));
          usb_ep_initiate_control_response(request, (const uint8_t*)&status, 2);
          break;

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
            // Make sure control callback cleared
            usb_control_set_complete_callback(NULL);

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

static bool usb_set_configuration() {
  const usb_configuration_descriptor_t* descriptor_config = (const usb_configuration_descriptor_t*)usb_descriptor_configuration();

  if (descriptor_config == NULL || descriptor_config->bDescriptorType != USB_DESCRIPTOR_TYPE_CONFIGURATION) {
    return false;
  }

  // Parse configuration descriptor
  usb_device.remote_wakeup = (descriptor_config->bmAttributes & USB_DESCRIPTOR_TYPE_CONFIG_ATT_REMOTE_WAKEUP) ? 1U : 0U;
  usb_device.self_powered = (descriptor_config->bmAttributes & USB_DESCRIPTOR_TYPE_CONFIG_ATT_SELF_POWERED) ? 1U : 0U;

  // Parse interface descriptor
  const uint8_t* discriptor = ((const uint8_t*)descriptor_config) + sizeof(usb_configuration_descriptor_t);
  const uint8_t* descriptor_end = ((const uint8_t*)descriptor_config) + descriptor_config->wTotalLength;

  while (discriptor < descriptor_end) {
    uint8_t assoc_itf_count = 1;

    // Class will always starts with Interface Association (if any) and then Interface descriptor
    if (USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION == usb_descriptor_type(discriptor)) {
      const usb_interface_association_descriptor_t* descriptor_iad = (const usb_interface_association_descriptor_t*)discriptor;
      assoc_itf_count = descriptor_iad->bInterfaceCount;

      discriptor = usb_next_descriptor(discriptor);  // next to Interface
    }

    if (USB_DESCRIPTOR_TYPE_INTERFACE != usb_descriptor_type(discriptor)) {
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

// return descriptor's buffer and update descriptor_len
static bool process_get_descriptor(const usb_control_request_t* request) {
  const usb_descriptor_type_t descriptor_type = (usb_descriptor_type_t)U16_HIGH(request->wValue);
  const uint8_t descriptor_index = U16_LOW(request->wValue);

  switch (descriptor_type) {
    case USB_DESCRIPTOR_TYPE_DEVICE: {
      const uint8_t* descriptor_device = (const uint8_t*)get_device_descriptor();
      return usb_ep_initiate_control_response(request, descriptor_device, sizeof(usb_device_descriptor_t));
    }

    case USB_DESCRIPTOR_TYPE_CONFIGURATION: {
      usb_configuration_descriptor_t* descriptor_config = (usb_configuration_descriptor_t*)usb_descriptor_configuration(descriptor_index);
      return usb_ep_initiate_control_response(request, (const uint8_t*)descriptor_config, descriptor_config->wTotalLength);
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

// Configure consecutive endpoint descriptors (IN & OUT)
bool usb_ep_open_in_out(const usb_ep_descriptor_t* descriptor_ep, uint8_t xfer_type, uint8_t* ep_addr_out, uint8_t* ep_addr_in) {
  for (int i = 0; i < 2; i++) {
    if (descriptor_ep->bDescriptorType != USB_DESCRIPTOR_TYPE_ENDPOINT || descriptor_ep->bmAttributes.type != xfer_type) {
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

static inline bool usb_control_status_stage(const usb_control_request_t* request) {
  // Opposite to endpoint in Data Phase
  const usb_request_direction_index_t request_direction = usb_request_direction(request->bmRequestType);
  const uint8_t ep_addr = request_direction ? USB_DIR_DEVICE_IN_HOST_OUT : USB_DIR_DEVICE_OUT_HOST_IN;

  return usb_ep_queue_transfer(ep_addr, NULL, 0);
}

// Status phase
bool usb_control_init_status_stage(const usb_control_request_t* request) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = NULL;
  control_transfer.feed.fed_count = 0;
  control_transfer.feed.total_count = 0;

  return usb_control_status_stage(request);
}

static bool usb_control_data_stage() {
  // Calculate the remaining length of data to transfer
  const uint16_t len = feed_forward_remaining_count(&control_transfer.feed, USB_EP0_BUFFER_SIZE);

  // Address for EP0 host OUT (assume OUT)
  uint8_t ep0_addr = USB_DIR_DEVICE_IN_HOST_OUT;

  // Get direction from request
  const usb_request_direction_index_t request_direction = usb_request_direction(control_transfer.request.bmRequestType);

  // Is the direction IN?
  if (request_direction == USB_DIR_DEVICE_OUT_HOST_IN_IDX) {
    // Address for EP0 host IN
    ep0_addr = USB_DIR_DEVICE_OUT_HOST_IN;

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

bool usb_ep_initiate_control_response(const usb_control_request_t* request, const uint8_t* buffer, uint16_t len) {
  control_transfer.request = (*request);
  control_transfer.feed.buffer = (uint8_t*)buffer;
  control_transfer.feed.fed_count = 0U;
  control_transfer.feed.total_count = (len < request->wLength) ? len : request->wLength;

  // If the request contains data then commence the data stage for the request
  if (request->wLength > 0U) {
    // This is the control data stage
    return usb_control_data_stage();
  }

  // This is the control status stage and we just need to respond with the response
  return usb_control_status_stage(request);
}

void usb_control_transfer_reset(void) {
  memset(&control_transfer, 0, sizeof(usb_control_transfer_t));
}

// Set complete callback
void usb_control_set_complete_callback(usb_control_transfer_complete_t fp) {
  control_transfer.control_complete_cb = fp;
}

bool usb_control_transfer(uint8_t ep_addr, uint32_t transferred_bytes) {
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
      (transferred_bytes < USB_EP0_BUFFER_SIZE)) {
    // DATA stage is complete
    bool data_stage_complete_ok = true;

    // If the initiator provided a control completion callback then we should call it,
    // the callback can return false to indicate that the device should be stalled
    if (control_transfer.control_complete_cb) {
      data_stage_complete_ok = control_transfer.control_complete_cb(CONTROL_STAGE_DATA, &control_transfer.request);
    }

    if (data_stage_complete_ok) {
      // Data stage complete so send control status
      return usb_control_status_stage(&control_transfer.request);
    }

    // Stall both IN and OUT control endpoint if not OK
    usb_ep_stall_set(USB_DIR_DEVICE_IN_HOST_OUT);
    usb_ep_stall_set(USB_DIR_DEVICE_OUT_HOST_IN);

    return true;
  }

  // More data to transfer, so queue next batch of bytes to send
  return usb_control_data_stage();
}