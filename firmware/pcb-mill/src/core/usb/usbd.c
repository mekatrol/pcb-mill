#include "dcd.h"
#include "cdc_device.h"
#include "tusb_private.h"
#include "usbd.h"
#include "usbd_pvt.h"
#include "stm32g0xx.h"

// Get high or low byte
#define U16_HIGH(_u16) ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16) ((uint8_t)((_u16) & 0x00ff))

//--------------------------------------------------------------------+
// Weak stubs: invoked if no strong implementation is available
//--------------------------------------------------------------------+
__attribute__((weak)) uint8_t const* tud_descriptor_bos_cb(void) {
  return NULL;
}

__attribute__((weak)) uint8_t const* tud_descriptor_device_qualifier_cb(void) {
  return NULL;
}

__attribute__((weak)) uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index) {
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
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp);

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
          dcd_set_address((uint8_t)request->wValue);

          // skip tud_control_status()
          usb_device.addressed = 1;
          break;

        case USB_STD_GET_CONFIGURATION: {
          uint8_t cfg_num = usb_device.cfg_num;
          tud_control_xfer(request, &cfg_num, 1);
        } break;

        case USB_STD_SET_CONFIGURATION: {
          uint8_t const cfg_num = (uint8_t)request->wValue;

          // Only process if new configure is different
          if (usb_device.cfg_num != cfg_num) {
            if (usb_device.cfg_num) {
              // disable SOF
              dcd_sof_enable(false);

              // close all non-control endpoints, cancel all pending transfers if any
              dcd_edpt_close_all();

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
          if (!process_get_descriptor(request)) {
            return false;
          }
          break;

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
          tud_control_xfer(request, &status, 2);
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
              tud_control_xfer(request, &alternate, 1);
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
      uint8_t const ep_num = USB_EP_NUM(ep_addr);

      if (ep_num >= sizeof(usb_device.ep2drv) / sizeof(usb_device.ep2drv[0])) {
        return false;
      }

      if (USB_REQUEST_TYPE_STANDARD != request_type) {
        // Forward class request to its driver
        return invoke_class_control(request);
      } else {
        // Handle STD request to endpoint
        switch (request->bRequest) {
          case USB_STD_GET_STATUS: {
            uint16_t status = usbd_edpt_stalled(ep_addr) ? 0x0001 : 0x0000;
            tud_control_xfer(request, &status, 2);
          } break;

          case USB_STD_CLEAR_FEATURE:
          case USB_STD_SET_FEATURE: {
            if (TUSB_REQ_FEATURE_EDPT_HALT == request->wValue) {
              if (USB_STD_CLEAR_FEATURE == request->bRequest) {
                usbd_edpt_clear_stall(ep_addr);
              } else {
                usbd_edpt_stall(ep_addr);
              }
            }

            // Some classes such as USBTMC needs to clear/re-init its buffer when receiving CLEAR_FEATURE request
            // We will also forward std request targeted endpoint to class drivers as well

            // STD request must always be ACKed regardless of driver returned value
            // Also clear complete callback if driver set since it can also stall the request.
            invoke_class_control(request);
            usbd_control_set_complete_callback(NULL);

            // skip ZLP status if driver already did that
            if (!usb_device.ep_status[0][USB_ENDPOINT_DIRECTION_IN].busy) {
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
  const usb_configuration_descriptor_t* desc_cfg = (const usb_configuration_descriptor_t*)usb_descriptor_configuration();

  if (desc_cfg == NULL || desc_cfg->bDescriptorType != USB_DESC_CONFIGURATION) {
    return false;
  }

  // Parse configuration descriptor
  usb_device.remote_wakeup_support = (desc_cfg->bmAttributes & USB_DESC_CONFIG_ATT_REMOTE_WAKEUP) ? 1u : 0u;
  usb_device.self_powered = (desc_cfg->bmAttributes & USB_DESC_CONFIG_ATT_SELF_POWERED) ? 1u : 0u;

  // Parse interface descriptor
  uint8_t const* p_desc = ((uint8_t const*)desc_cfg) + sizeof(usb_configuration_descriptor_t);
  uint8_t const* desc_end = ((uint8_t const*)desc_cfg) + desc_cfg->wTotalLength;

  while (p_desc < desc_end) {
    uint8_t assoc_itf_count = 1;

    // Class will always starts with Interface Association (if any) and then Interface descriptor
    if (USB_DESC_INTERFACE_ASSOCIATION == tu_desc_type(p_desc)) {
      const usb_interface_association_descriptor_t* desc_iad = (const usb_interface_association_descriptor_t*)p_desc;
      assoc_itf_count = desc_iad->bInterfaceCount;

      p_desc = tu_desc_next(p_desc);  // next to Interface
    }

    if (USB_DESC_INTERFACE != tu_desc_type(p_desc)) {
      return false;
    }

    const usb_control_interface_descriptor_t* desc_itf = (const usb_control_interface_descriptor_t*)p_desc;

    // Find driver for this interface
    uint16_t const remaining_len = (uint16_t)(desc_end - p_desc);
    uint16_t const drv_len = usb_cdc_open(desc_itf, remaining_len);

    if ((sizeof(usb_control_interface_descriptor_t) <= drv_len) && (drv_len <= remaining_len)) {
      if (assoc_itf_count == 1) {
        assoc_itf_count = 2;
      }

      // bind all endpoints to found driver
      tu_edpt_bind_driver(usb_device.ep2drv, desc_itf, drv_len);

      // next Interface
      p_desc += drv_len;

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

// return descriptor's buffer and update desc_len
static bool process_get_descriptor(usb_control_request_t const* request) {
  usb_desc_type_t const desc_type = (usb_desc_type_t)U16_HIGH(request->wValue);
  uint8_t const desc_index = U16_LOW(request->wValue);

  switch (desc_type) {
    case USB_DESC_DEVICE: {
      void* desc_device = (void*)(uintptr_t)tud_descriptor_device_cb();
      if (!desc_device) {
        return false;
      }

      // Only response with exactly 1 Packet if: not addressed and host requested more data than device descriptor has.
      // This only happens with the very first get device descriptor and EP0 size = 8 or 16.
      if ((USB_EP0_BUFFER_SIZE < sizeof(usb_device_desc_t)) && !usb_device.addressed &&
          ((usb_control_request_t const*)request)->wLength > sizeof(usb_device_desc_t)) {
        // Hack here: we modify the request length to prevent usbd_control response with zlp
        // since we are responding with 1 packet & less data than wLength.
        usb_control_request_t mod_request = *request;
        mod_request.wLength = USB_EP0_BUFFER_SIZE;

        return tud_control_xfer(&mod_request, desc_device, USB_EP0_BUFFER_SIZE);
      } else {
        return tud_control_xfer(request, desc_device, sizeof(usb_device_desc_t));
      }
    }
      // break; // unreachable

    case USB_DESC_BOS: {
      // requested by host if USB > 2.0 ( i.e 2.1 or 3.x )
      uintptr_t desc_bos = (uintptr_t)tud_descriptor_bos_cb();
      if (!desc_bos) {
        return false;
      }

      // Use offsetof to avoid pointer to the odd/misaligned address
      uint16_t const total_len = tu_unaligned_read16((const void*)(desc_bos + offsetof(tusb_desc_bos_t, wTotalLength)));

      return tud_control_xfer(request, (void*)desc_bos, total_len);
    }
      // break; // unreachable

    case USB_DESC_CONFIGURATION:
    case USB_DESC_OTHER_SPEED_CONFIG: {
      uintptr_t desc_config;

      if (desc_type == USB_DESC_CONFIGURATION) {
        desc_config = (uintptr_t)usb_descriptor_configuration(desc_index);
        if (!desc_config) {
          return false;
        }
      } else {
        // Host only request this after getting Device Qualifier descriptor
        desc_config = (uintptr_t)tud_descriptor_other_speed_configuration_cb(desc_index);
        if (!desc_config) {
          return false;
        }
      }

      // Use offsetof to avoid pointer to the odd/misaligned address
      uint16_t const total_len = tu_unaligned_read16((const void*)(desc_config + offsetof(usb_configuration_descriptor_t, wTotalLength)));

      return tud_control_xfer(request, (void*)desc_config, total_len);
    }
      // break; // unreachable

    case USB_DESC_STRING: {
      // String Descriptor always uses the desc set from user
      uint8_t const* desc_str = (uint8_t const*)tud_descriptor_string_cb(desc_index, request->wIndex);
      if (!desc_str) {
        return false;
      }

      // first byte of descriptor is its size
      return tud_control_xfer(request, (void*)(uintptr_t)desc_str, tu_desc_len(desc_str));
    }
      // break; // unreachable

    case USB_DESC_DEVICE_QUALIFIER: {
      uint8_t const* desc_qualifier = tud_descriptor_device_qualifier_cb();
      if (!desc_qualifier) {
        return false;
      }
      return tud_control_xfer(request, (void*)(uintptr_t)desc_qualifier, tu_desc_len(desc_qualifier));
    }
      // break; // unreachable

    default:
      return false;
  }
}

// Configure consecutive endpoint descriptors (IN & OUT)
bool usb_endpoint_open_set(const usb_endpoint_descriptor_t* desc_ep, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in) {
  for (int i = 0; i < 2; i++) {
    if (desc_ep->bDescriptorType != USB_DESC_ENDPOINT || desc_ep->bmAttributes.type != xfer_type) {
      return false;
    }

    if (!usb_endpoint_open(desc_ep)) {
      return false;
    }

    if (USB_EP_DIR(desc_ep->bEndpointAddress) == USB_DIR_IN) {
      (*ep_in) = desc_ep->bEndpointAddress;
    } else {
      (*ep_out) = desc_ep->bEndpointAddress;
    }

    desc_ep = (const usb_endpoint_descriptor_t*)tu_desc_next(desc_ep);
  }

  return true;
}

bool usbd_edpt_claim(uint8_t ep_addr) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_state_t* ep_state = &usb_device.ep_status[ep_num][ep_dir_idx];

  return tu_edpt_claim(ep_state);
}

bool usbd_edpt_release(uint8_t ep_addr) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  endpoint_state_t* ep_state = &usb_device.ep_status[ep_num][ep_dir_idx];

  return tu_edpt_release(ep_state);
}

bool usb_endpoint_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // Attempt to transfer on a busy endpoint, sound like an race condition !
  if (usb_device.ep_status[ep_num][ep_dir_idx].busy != 0) {
    return false;
  }

  // Set busy first since the actual transfer can be complete before dcd_edpt_xfer()
  // could return and USBD task can preempt and clear the busy
  usb_device.ep_status[ep_num][ep_dir_idx].busy = 1;

  if (dcd_edpt_xfer(ep_addr, buffer, total_bytes)) {
    return true;
  } else {
    // DCD error, mark endpoint as ready to allow next transfer
    usb_device.ep_status[ep_num][ep_dir_idx].busy = 0;
    usb_device.ep_status[ep_num][ep_dir_idx].claimed = 0;
    return false;
  }
}

bool usbd_edpt_busy(uint8_t ep_addr) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  return usb_device.ep_status[ep_num][ep_dir_idx].busy;
}

void usbd_edpt_stall(uint8_t ep_addr) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // only stalled if currently cleared
  usb_endpoint_stall(ep_addr);
  usb_device.ep_status[ep_num][ep_dir_idx].stalled = 1;
  usb_device.ep_status[ep_num][ep_dir_idx].busy = 1;
}

void usbd_edpt_clear_stall(uint8_t ep_addr) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  // only clear if currently stalled
  dcd_edpt_clear_stall(ep_addr);
  usb_device.ep_status[ep_num][ep_dir_idx].stalled = 0;
  usb_device.ep_status[ep_num][ep_dir_idx].busy = 0;
}

bool usbd_edpt_stalled(uint8_t ep_addr) {
  uint8_t const ep_num = USB_EP_NUM(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  return usb_device.ep_status[ep_num][ep_dir_idx].stalled;
}
