#include "usb.h"

typedef enum {
  USB_BMATTR_RESERVED_D7 = 0x80,      ///< Bit 7: Reserved, must always be set to 1
  USB_BMATTR_SELF_POWERED = 0x40,     ///< Bit 6: 1 = Device is self-powered, 0 = bus-powered
  USB_BMATTR_REMOTE_WAKEUP = 0x20,    ///< Bit 5: 1 = Device can wake host from suspend
  USB_BMATTR_RESERVED_D4_TO_0 = 0x1F  ///< Bits 4..0: Reserved, must be 0
} usb_bm_attributes_mask_t;

typedef enum {
  USB_CONFIG_REMOTE_WAKEUP_MASK = 1U << 5,
  USB_CONFIG_SELF_POWERED_MASK = 1U << 6,
} usb_configuration_flags;

typedef struct {
  volatile usb_configuration_flags state_flags;  // Flags indicating the state of the device
  volatile uint8_t interface_count;              // The total number of interfaces the device has
} usb_device_t;

// We are a single USB device
// volatile usb_device_t usb_device;

// Will return next interfac descriptor
__attribute__((always_inline)) static inline const usb_interface_association_descriptor_t* next_interface(const usb_interface_association_descriptor_t* interface_assoc) {
  return (const usb_interface_association_descriptor_t*)(interface_assoc + interface_assoc->bLength);
}

#include "usb.h"
#include "cdc_device.h"
#include "stm32g0xx.h"

// Get high or low byte
#define U16_HIGH(_u16) ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16) ((uint8_t)((_u16) & 0x00ff))

//--------------------------------------------------------------------+
// Weak stubs: invoked if no strong implementation is available
//--------------------------------------------------------------------+
__attribute__((weak)) uint8_t const* usb_descriptor_other_speed_configuration(uint8_t index) {
  (void)index;
  return NULL;
}

//--------------------------------------------------------------------+
// Device Data
//--------------------------------------------------------------------+

usbd_device_t usb_device;

//--------------------------------------------------------------------+
// Prototypes
//--------------------------------------------------------------------+
static bool usb_set_configuration();
static bool process_get_descriptor(usb_control_request_t const* request);

// from usbd_control.c
void usbd_control_reset(void);
void usbd_control_set_request(usb_control_request_t const* request);
void usbd_control_set_complete_callback(usb_cdc_control_transfer_t fp);

//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+
bool tud_connected(void) {
  return usb_device.connected;
}

bool usb_init_driver() {
  memset(&usb_device, 0, sizeof(usbd_device_t));

  // Init class drivers
  usb_cdc_init();

  // Init device controller driver
  usb_device_init();
  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  return true;
}

void usb_configuration_reset() {
  usb_cdc_reset();

  memset(&usb_device, 0, sizeof(usbd_device_t));
  memset(usb_device.ep2drv, 0xFF, sizeof(usb_device.ep2drv));  // invalid mapping
}

//--------------------------------------------------------------------+
// Control Request Parser & Handling
//--------------------------------------------------------------------+

// Helper to invoke class driver control request handler
static bool invoke_class_control(usb_control_request_t const* request) {
  usbd_control_set_complete_callback(usb_cdc_control_xfer_cb);
  return usb_cdc_control_xfer_cb(CONTROL_STAGE_SETUP, request);
}

// This handles the actual request and its response.
// Returns false if unable to complete the request, causing caller to stall control endpoints.
bool process_control_request(usb_control_request_t const* request) {
  usbd_control_set_complete_callback(NULL);

  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);

  // Evertything >= USB_REQUEST_TYPE_RESERVED is reserved in spac and should not be used
  if (request_type >= USB_REQUEST_TYPE_RESERVED) {
    return false;
  }

  // Vendor request
  if (request_type == USB_REQUEST_TYPE_VENDOR) {
    usbd_control_set_complete_callback(NULL);
    return false;
  }

  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);
  switch (request_recipient) {
    //------------- Device Requests e.g in enumeration -------------//
    case USB_REQUEST_RECIPIENT_DEVICE:
      if (USB_REQUEST_TYPE_CLASS == request_type) {
        // forward to class driver: "non-STD request to Interface"
        return invoke_class_control(request);
      }

      if (USB_REQUEST_TYPE_STANDARD != request_type) {
        // Non-standard request is not supported
        return false;
      }

      switch (request->bRequest) {
        case USB_STD_SET_ADDRESS:
          // Depending on mcu, status phase could be sent either before or after changing device address,
          // or even require stack to not response with status at all
          // Therefore DCD must take full responsibility to response and include zlp status packet if needed.
          usbd_control_set_request(request);  // set request since DCD has no access to tud_control_status() API

          // Respond with status (ep0)
          usb_endpoint_transfer_hal(0x00, USB_DIR_IN >> 7, NULL, 0);

          // skip tud_control_status()
          usb_device.addressed = 1;
          break;

        case USB_STD_GET_CONFIGURATION: {
          uint8_t cfg_num = usb_device.cfg_num;
          usb_endpoint_control_transfer(request, &cfg_num, 1);
        } break;

        case USB_STD_SET_CONFIGURATION: {
          uint8_t const cfg_num = (uint8_t)request->wValue;

          // Only process if new configure is different
          if (usb_device.cfg_num != cfg_num) {
            if (usb_device.cfg_num) {
              // disable SOF
              usb_sof_set_enable(false);

              // close all non-control endpoints, cancel all pending transfers if any
              usb_endpoint_close_all();

              // close all drivers and current configured state except bus speed
              usb_configuration_reset();
            }

            usb_device.cfg_num = cfg_num;

            // Handle the new configuration and execute the corresponding callback
            if (cfg_num) {
              // switch to new configuration if not zero
              if (!usb_set_configuration(cfg_num)) {
                usb_device.cfg_num = 0;
                return false;
              }

              // TODO: USB mount
            } else {
              // TODO: USB unmount
            }
          }

          tud_control_status(request);
        } break;

        case USB_STD_GET_DESCRIPTOR:
          return process_get_descriptor(request);

        case USB_STD_SET_FEATURE:
          switch (request->wValue) {
            case TUSB_REQ_FEATURE_REMOTE_WAKEUP:
              // Host may enable remote wake up before suspending especially HID device
              usb_device.remote_wakeup_en = true;
              tud_control_status(request);
              break;

            // Stall unsupported feature selector
            default:
              return false;
          }
          break;

        case USB_STD_CLEAR_FEATURE:
          // Only support remote wakeup for device feature
          if (request->wValue != TUSB_REQ_FEATURE_REMOTE_WAKEUP) {
            return false;
          }

          // Host may disable remote wake up after resuming
          usb_device.remote_wakeup_en = false;
          tud_control_status(request);
          break;

        case USB_STD_GET_STATUS: {
          // Device status bit mask
          // - Bit 0: Self Powered
          // - Bit 1: Remote Wakeup enabled
          uint16_t status = (uint16_t)((1u) | (usb_device.remote_wakeup_en ? 2u : 0u));
          usb_endpoint_control_transfer(request, &status, 2);
          break;
        }

        // Unknown/Unsupported request
        default:
          return false;
      }
      break;

    //------------- Class/Interface Specific Request -------------//
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
            // Clear complete callback if driver set since it can also stall the request.
            usbd_control_set_complete_callback(NULL);

            if (USB_STD_GET_INTERFACE == request->bRequest) {
              uint8_t alternate = 0;
              usb_endpoint_control_transfer(request, &alternate, 1);
            } else {
              tud_control_status(request);
            }
            break;

          default:
            return false;
        }
      }
      break;
    }

    //------------- Endpoint Request -------------//
    case USB_REQUEST_RECIPIENT_ENDPOINT: {
      uint8_t const ep_addr = (uint8_t)(request->wIndex & 0xFF);
      const uint8_t ep_idn = USB_EP_IDN(ep_addr);

      if (ep_idn >= sizeof(usb_device.ep2drv) / sizeof(usb_device.ep2drv[0])) {
        return false;
      }

      if (USB_REQUEST_TYPE_STANDARD != request_type) {
        // Forward class request to its driver
        return invoke_class_control(request);
      } else {
        // Handle STD request to endpoint
        switch (request->bRequest) {
          case USB_STD_GET_STATUS: {
            uint16_t status = usb_endpoint_is_stalled(ep_addr) ? 0x0001 : 0x0000;
            usb_endpoint_control_transfer(request, &status, 2);
          } break;

          case USB_STD_CLEAR_FEATURE:
          case USB_STD_SET_FEATURE: {
            if (TUSB_REQ_FEATURE_EDPT_HALT == request->wValue) {
              if (USB_STD_CLEAR_FEATURE == request->bRequest) {
                usb_endpoint_stall_clear(ep_addr);
              } else {
                usb_endpoint_stall_set(ep_addr);
              }
            }

            // Some classes such as USBTMC needs to clear/re-init its buffer when receiving CLEAR_FEATURE request
            // We will also forward std request targeted endpoint to class drivers as well

            // STD request must always be ACKed regardless of driver returned value
            // Also clear complete callback if driver set since it can also stall the request.
            invoke_class_control(request);
            usbd_control_set_complete_callback(NULL);

            // skip ZLP status if driver already did that
            if (!usb_device.ep_status[0][USB_EP_DIRECTION_IN_IDX].busy) {
              tud_control_status(request);
            }
          } break;

          // Unknown/Unsupported request
          default:
            return false;
        }
      }
    } break;

    // Unknown recipient
    default:
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
  usb_device.remote_wakeup_support = (descriptor_config->bmAttributes & USB_DESCRIPTOR_TYPE_CONFIG_ATT_REMOTE_WAKEUP) ? 1U : 0U;
  usb_device.self_powered = (descriptor_config->bmAttributes & USB_DESCRIPTOR_TYPE_CONFIG_ATT_SELF_POWERED) ? 1U : 0U;

  // Parse interface descriptor
  uint8_t const* discriptor = ((uint8_t const*)descriptor_config) + sizeof(usb_configuration_descriptor_t);
  uint8_t const* descriptor_end = ((uint8_t const*)descriptor_config) + descriptor_config->wTotalLength;

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
    uint16_t const remaining_len = (uint16_t)(descriptor_end - discriptor);
    uint16_t const drv_len = usb_cdc_open(descriptor_interface, remaining_len);

    if ((sizeof(usb_control_interface_descriptor_t) <= drv_len) && (drv_len <= remaining_len)) {
      if (assoc_itf_count == 1) {
        assoc_itf_count = 2;
      }

      // bind all endpoints to found driver
      tu_edpt_bind_driver(usb_device.ep2drv, descriptor_interface, drv_len);

      // next Interface
      discriptor += drv_len;

      break;  // exit driver find loop
    }
  }

  return true;
}

typedef struct {
  uint16_t val;
} __attribute__((packed)) tu_unaligned_uint16_t;

__attribute__((always_inline)) static inline uint16_t tu_unaligned_read16(const void* mem) {
  tu_unaligned_uint16_t const* ua16 = (tu_unaligned_uint16_t const*)mem;
  return ua16->val;
}

// return descriptor's buffer and update descriptor_len
static bool process_get_descriptor(usb_control_request_t const* request) {
  usb_descriptor_type_t const descriptor_type = (usb_descriptor_type_t)U16_HIGH(request->wValue);
  uint8_t const descriptor_index = U16_LOW(request->wValue);

  switch (descriptor_type) {
    case USB_DESCRIPTOR_TYPE_DEVICE: {
      void* descriptor_device = (void*)(uintptr_t)get_device_descriptor();
      return usb_endpoint_control_transfer(request, descriptor_device, sizeof(usb_device_descriptor_t));
    }

    case USB_DESCRIPTOR_TYPE_CONFIGURATION:
    case USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIG: {
      uintptr_t descriptor_config = (uintptr_t)NULL;

      if (descriptor_type == USB_DESCRIPTOR_TYPE_CONFIGURATION) {
        descriptor_config = (uintptr_t)usb_descriptor_configuration(descriptor_index);
      } else {
        // Host only request this after getting Device Qualifier descriptor
        descriptor_config = (uintptr_t)usb_descriptor_other_speed_configuration(descriptor_index);
      }

      if (!descriptor_config) {
        return false;
      }

      // Use offsetof to avoid pointer to the odd/misaligned address
      uint16_t const total_len = tu_unaligned_read16((const void*)(descriptor_config + offsetof(usb_configuration_descriptor_t, wTotalLength)));

      return usb_endpoint_control_transfer(request, (void*)descriptor_config, total_len);
    }

    case USB_DESCRIPTOR_TYPE_STRING: {
      // String Descriptor always uses the desc set from user
      uint8_t const* descriptor_str = (uint8_t const*)usb_descriptor_string(descriptor_index, request->wIndex);
      if (!descriptor_str) {
        return false;
      }

      // first byte of descriptor is its size
      return usb_endpoint_control_transfer(request, (void*)(uintptr_t)descriptor_str, usb_descriptor_len(descriptor_str));
    }

    case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER: {
      const uint8_t* descriptor_qualifier = usb_descriptor_device_qualifier();
      if (!descriptor_qualifier) {
        return false;
      }
      return usb_endpoint_control_transfer(request, (void*)(uintptr_t)descriptor_qualifier, usb_descriptor_len(descriptor_qualifier));
    }

    default:
      return false;
  }
}

// Configure consecutive endpoint descriptors (IN & OUT)
bool usb_endpoint_open_in_out(const usb_endpoint_descriptor_t* descriptor_ep, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in) {
  for (int i = 0; i < 2; i++) {
    if (descriptor_ep->bDescriptorType != USB_DESCRIPTOR_TYPE_ENDPOINT || descriptor_ep->bmAttributes.type != xfer_type) {
      return false;
    }

    if (!usb_endpoint_open(descriptor_ep)) {
      return false;
    }

    if (USB_EP_DIR(descriptor_ep->bEndpointAddress) == USB_DIR_IN) {
      (*ep_in) = descriptor_ep->bEndpointAddress;
    } else {
      (*ep_out) = descriptor_ep->bEndpointAddress;
    }

    descriptor_ep = (const usb_endpoint_descriptor_t*)usb_next_descriptor(descriptor_ep);
  }

  return true;
}

bool usb_endpoint_claim(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_state_t* ep_state = &usb_device.ep_status[ep_idn][ep_dir_idx];

  return tu_edpt_claim(ep_state);
}

bool usb_endpoint_release(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_state_t* ep_state = &usb_device.ep_status[ep_idn][ep_dir_idx];

  return tu_edpt_release(ep_state);
}

bool usb_endpoint_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // Attempt to transfer on a busy endpoint, sound like an race condition !
  if (usb_device.ep_status[ep_idn][ep_dir_idx].busy != 0) {
    return false;
  }

  // Set busy first since the actual transfer can be complete before usb_endpoint_transfer_hal()
  // could return and USBD task can preempt and clear the busy
  usb_device.ep_status[ep_idn][ep_dir_idx].busy = 1;

  if (usb_endpoint_transfer_hal(ep_idn, ep_dir_idx, buffer, total_bytes)) {
    return true;
  } else {
    // DCD error, mark endpoint as ready to allow next transfer
    usb_device.ep_status[ep_idn][ep_dir_idx].busy = 0;
    usb_device.ep_status[ep_idn][ep_dir_idx].claimed = 0;
    return false;
  }
}

void usb_endpoint_stall_set(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // only stalled if currently cleared
  usb_endpoint_stall_set_hal(ep_idn, ep_dir_idx);
  usb_device.ep_status[ep_idn][ep_dir_idx].stalled = 1;
  usb_device.ep_status[ep_idn][ep_dir_idx].busy = 1;
}

void usb_endpoint_stall_clear(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // only clear if currently stalled
  usb_endpoint_stall_clear_hal(ep_idn, ep_dir_idx);
  usb_device.ep_status[ep_idn][ep_dir_idx].stalled = 0;
  usb_device.ep_status[ep_idn][ep_dir_idx].busy = 0;
}

bool usb_endpoint_is_stalled(uint8_t ep_addr) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  return usb_device.ep_status[ep_idn][ep_dir_idx].stalled;
}

typedef struct {
  usb_control_request_t request;
  uint8_t* buffer;
  uint16_t data_len;
  uint16_t total_xferred;
  usb_cdc_control_transfer_t complete_cb;
} usbd_control_xfer_t;

static usbd_control_xfer_t control_transfer;

static __attribute__((aligned(4))) uint8_t ep0_control_buffer[USB_EP0_BUFFER_SIZE];

static inline bool status_stage_xact(const usb_control_request_t* request) {
  // Opposite to endpoint in Data Phase
  const usb_endpoint_direction_index_t request_direction = usb_request_direction(request->bmRequestType);
  const uint8_t ep_addr = request_direction ? USB_DIR_OUT : USB_DIR_IN;

  return usb_endpoint_transfer(ep_addr, NULL, 0);
}

// Status phase
bool tud_control_status(const usb_control_request_t* request) {
  control_transfer.request = (*request);
  control_transfer.buffer = NULL;
  control_transfer.total_xferred = 0;
  control_transfer.data_len = 0;

  return status_stage_xact(request);
}

// Queue a transaction in Data Stage
// Each transaction has up to Endpoint0's max packet size.
// This function can also transfer an zero-length packet
static bool data_stage_xact() {
  const uint16_t xact_len = min_u16(control_transfer.data_len - control_transfer.total_xferred, USB_EP0_BUFFER_SIZE);
  uint8_t ep_addr = USB_DIR_OUT;

  const usb_endpoint_direction_index_t request_direction = usb_request_direction(control_transfer.request.bmRequestType);
  if (request_direction == USB_EP_DIRECTION_IN_IDX) {
    ep_addr = USB_DIR_IN;
    if (xact_len) {
      if (xact_len > USB_EP0_BUFFER_SIZE) {
        return false;
      }

      // Copy data to ep0_control_buffer
      memcpy(ep0_control_buffer, control_transfer.buffer, xact_len);
    }
  }

  return usb_endpoint_transfer(ep_addr, xact_len ? ep0_control_buffer : NULL, xact_len);
}

bool usb_endpoint_control_transfer(const usb_control_request_t* request, void* buffer, uint16_t len) {
  control_transfer.request = (*request);
  control_transfer.buffer = (uint8_t*)buffer;
  control_transfer.total_xferred = 0U;
  control_transfer.data_len = min_u16(len, request->wLength);

  if (request->wLength > 0U) {
    if (control_transfer.data_len > 0U) {
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

void usbd_control_set_request(const usb_control_request_t* request);
void usbd_control_set_complete_callback(usb_cdc_control_transfer_t fp);

void usbd_control_reset(void) {
  memset(&control_transfer, 0, sizeof(usbd_control_xfer_t));
}

// Set complete callback
void usbd_control_set_complete_callback(usb_cdc_control_transfer_t fp) {
  control_transfer.complete_cb = fp;
}

void usbd_control_set_request(const usb_control_request_t* request) {
  control_transfer.request = (*request);
  control_transfer.buffer = NULL;
  control_transfer.total_xferred = 0;
  control_transfer.data_len = 0;
}

bool usb_control_transfer(uint8_t ep_addr, uint32_t transferred_bytes) {
  const uint8_t request_direction = USB_EP_DIR(control_transfer.request.bmRequestType);

  // Endpoint Address is opposite to direction bit, this is Status Stage complete event
  if (USB_EP_DIR(ep_addr) != request_direction) {
    if (transferred_bytes != 0) {
      return false;
    }

    // invoke optional dcd hook if available
    usb_endpoint_control_status_complete(&control_transfer.request);

    if (control_transfer.complete_cb) {
      control_transfer.complete_cb(CONTROL_STAGE_ACK, &control_transfer.request);
    }

    return true;
  }

  if (request_direction == USB_DIR_OUT) {
    if (!control_transfer.buffer) {
      return false;
    }
    memcpy(control_transfer.buffer, ep0_control_buffer, transferred_bytes);
  }

  control_transfer.total_xferred += (uint16_t)transferred_bytes;
  control_transfer.buffer += transferred_bytes;

  // Data Stage is complete when all request's length are transferred or
  // a short packet is sent including zero-length packet.
  if ((control_transfer.request.wLength == control_transfer.total_xferred) ||
      (transferred_bytes < USB_EP0_BUFFER_SIZE)) {
    // DATA stage is complete
    bool is_ok = true;

    // invoke complete callback if set
    // callback can still stall control in status phase e.g out data does not make sense
    if (control_transfer.complete_cb) {
      is_ok = control_transfer.complete_cb(CONTROL_STAGE_DATA, &control_transfer.request);
    }

    if (is_ok) {
      if (!status_stage_xact(&control_transfer.request)) {
        return false;
      }
    } else {
      // Stall both IN and OUT control endpoint
      usb_endpoint_stall_set(USB_DIR_OUT);
      usb_endpoint_stall_set(USB_DIR_IN);
    }
  } else {
    // More data to transfer
    if (!data_stage_xact()) {
      return false;
    }
  }

  return true;
}

bool tu_edpt_claim(endpoint_state_t* ep_state) {
  // can only claim the endpoint if it is not busy and not claimed yet.
  const bool ep_available = (ep_state->busy == 0) && (ep_state->claimed == 0);

  if (ep_available) {
    ep_state->claimed = 1;
  }

  return ep_available;
}

bool tu_edpt_release(endpoint_state_t* ep_state) {
  const bool released = (ep_state->claimed == 1) && (ep_state->busy == 0);
  if (released) {
    ep_state->claimed = 0;
  }
  return released;
}

void tu_edpt_bind_driver(uint8_t ep2drv[][EP_IN_OUT_PAIR], const usb_control_interface_descriptor_t* descriptor_interface, uint16_t descriptor_len) {
  uint8_t const* discriptor = (uint8_t const*)descriptor_interface;
  uint8_t const* descriptor_end = discriptor + descriptor_len;

  while (discriptor < descriptor_end) {
    if (USB_DESCRIPTOR_TYPE_ENDPOINT == usb_descriptor_type(discriptor)) {
      uint8_t const ep_addr = ((usb_endpoint_descriptor_t const*)discriptor)->bEndpointAddress;
      ep2drv[USB_EP_IDN(ep_addr)][USB_EP_DIR_IDX(ep_addr)] = 0;
    }
    discriptor = usb_next_descriptor(discriptor);
  }
}
