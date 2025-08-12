#include "dcd.h"
#include "cdc_device.h"
#include "tusb_private.h"
#include "usbd.h"
#include "usbd_pvt.h"
#include "stm32g0xx.h"

//--------------------------------------------------------------------+
// USBD Configuration
//--------------------------------------------------------------------+
#ifndef CFG_TUD_TASK_QUEUE_SZ
#define CFG_TUD_TASK_QUEUE_SZ 16
#endif

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

__attribute__((weak)) bool tud_vendor_control_xfer_cb(uint8_t stage, tusb_control_request_t const* request) {
  (void)stage;
  (void)request;
  return false;
}

__attribute__((weak)) bool dcd_deinit() {
  return false;
}

__attribute__((weak)) void dcd_connect() {
}

//--------------------------------------------------------------------+
// Device Data
//--------------------------------------------------------------------+

usbd_device_t _usbd_dev;

//--------------------------------------------------------------------+
// Prototypes
//--------------------------------------------------------------------+
static bool process_set_config(uint8_t cfg_num);
static bool process_get_descriptor(tusb_control_request_t const* p_request);

// from usbd_control.c
void usbd_control_reset(void);
void usbd_control_set_request(tusb_control_request_t const* request);
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp);

//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+
bool tud_connected(void) {
  return _usbd_dev.connected;
}

bool tud_mounted(void) {
  return _usbd_dev.cfg_num ? true : false;
}

bool tud_disconnect(void) {
  USB->BCDR &= ~(USB_BCDR_DPPU);
  return true;
}

bool tud_connect(void) {
  dcd_connect();
  return true;
}

bool usb_init_driver() {
  memset(&_usbd_dev, 0, sizeof(usbd_device_t));

  // Init class drivers
  cdcd_init();

  // Init device controller driver
  dcd_init();
  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  return true;
}

void usb_configuration_reset() {
  cdcd_reset();

  memset(&_usbd_dev, 0, sizeof(usbd_device_t));
  memset(_usbd_dev.itf2drv, 0xFF, sizeof(_usbd_dev.itf2drv));  // invalid mapping
  memset(_usbd_dev.ep2drv, 0xFF, sizeof(_usbd_dev.ep2drv));    // invalid mapping
}

//--------------------------------------------------------------------+
// Control Request Parser & Handling
//--------------------------------------------------------------------+

// Helper to invoke class driver control request handler
static bool invoke_class_control(tusb_control_request_t const* request) {
  usbd_control_set_complete_callback(cdcd_control_xfer_cb);
  return cdcd_control_xfer_cb(CONTROL_STAGE_SETUP, request);
}

// This handles the actual request and its response.
// Returns false if unable to complete the request, causing caller to stall control endpoints.
bool process_control_request(tusb_control_request_t const* p_request) {
  usbd_control_set_complete_callback(NULL);
  if (p_request->bmRequestType_bit.type >= TUSB_REQ_TYPE_INVALID) {
    return false;
  }

  // Vendor request
  if (p_request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR) {
    usbd_control_set_complete_callback(tud_vendor_control_xfer_cb);
    return tud_vendor_control_xfer_cb(CONTROL_STAGE_SETUP, p_request);
  }

  switch (p_request->bmRequestType_bit.recipient) {
    //------------- Device Requests e.g in enumeration -------------//
    case TUSB_REQ_RCPT_DEVICE:
      if (TUSB_REQ_TYPE_CLASS == p_request->bmRequestType_bit.type) {
        // forward to class driver: "non-STD request to Interface"
        return invoke_class_control(p_request);
      }

      if (TUSB_REQ_TYPE_STANDARD != p_request->bmRequestType_bit.type) {
        // Non-standard request is not supported
        return false;
      }

      switch (p_request->bRequest) {
        case TUSB_REQ_SET_ADDRESS:
          // Depending on mcu, status phase could be sent either before or after changing device address,
          // or even require stack to not response with status at all
          // Therefore DCD must take full responsibility to response and include zlp status packet if needed.
          usbd_control_set_request(p_request);  // set request since DCD has no access to tud_control_status() API
          dcd_set_address((uint8_t)p_request->wValue);
          // skip tud_control_status()
          _usbd_dev.addressed = 1;
          break;

        case TUSB_REQ_GET_CONFIGURATION: {
          uint8_t cfg_num = _usbd_dev.cfg_num;
          tud_control_xfer(p_request, &cfg_num, 1);
        } break;

        case TUSB_REQ_SET_CONFIGURATION: {
          uint8_t const cfg_num = (uint8_t)p_request->wValue;

          // Only process if new configure is different
          if (_usbd_dev.cfg_num != cfg_num) {
            if (_usbd_dev.cfg_num) {
              // disable SOF
              dcd_sof_enable(false);

              // close all non-control endpoints, cancel all pending transfers if any
              dcd_edpt_close_all();

              // close all drivers and current configured state except bus speed
              usb_configuration_reset();
            }

            _usbd_dev.cfg_num = cfg_num;

            // Handle the new configuration and execute the corresponding callback
            if (cfg_num) {
              // switch to new configuration if not zero
              if (!process_set_config(cfg_num)) {
                _usbd_dev.cfg_num = 0;
                return false;
              }

              // TODO: USB mount
            } else {
              // TODO: USB unmount
            }
          }

          tud_control_status(p_request);
        } break;

        case TUSB_REQ_GET_DESCRIPTOR:
          if (!process_get_descriptor(p_request)) {
            return false;
          }
          break;

        case TUSB_REQ_SET_FEATURE:
          switch (p_request->wValue) {
            case TUSB_REQ_FEATURE_REMOTE_WAKEUP:
              // Host may enable remote wake up before suspending especially HID device
              _usbd_dev.remote_wakeup_en = true;
              tud_control_status(p_request);
              break;

            // Stall unsupported feature selector
            default:
              return false;
          }
          break;

        case TUSB_REQ_CLEAR_FEATURE:
          // Only support remote wakeup for device feature
          if (p_request->wValue != TUSB_REQ_FEATURE_REMOTE_WAKEUP) {
            return false;
          }

          // Host may disable remote wake up after resuming
          _usbd_dev.remote_wakeup_en = false;
          tud_control_status(p_request);
          break;

        case TUSB_REQ_GET_STATUS: {
          // Device status bit mask
          // - Bit 0: Self Powered
          // - Bit 1: Remote Wakeup enabled
          uint16_t status = (uint16_t)((1u) | (_usbd_dev.remote_wakeup_en ? 2u : 0u));
          tud_control_xfer(p_request, &status, 2);
          break;
        }

        // Unknown/Unsupported request
        default:
          return false;
      }
      break;

    //------------- Class/Interface Specific Request -------------//
    case TUSB_REQ_RCPT_INTERFACE: {
      // all requests to Interface (STD or Class) is forwarded to class driver.
      // notable requests are: GET HID REPORT DESCRIPTOR, SET_INTERFACE, GET_INTERFACE
      if (!invoke_class_control(p_request)) {
        // For GET_INTERFACE and SET_INTERFACE, it is mandatory to respond even if the class
        // driver doesn't use alternate settings or implement this
        if (TUSB_REQ_TYPE_STANDARD != p_request->bmRequestType_bit.type) {
          return false;
        }

        switch (p_request->bRequest) {
          case TUSB_REQ_GET_INTERFACE:
          case TUSB_REQ_SET_INTERFACE:
            // Clear complete callback if driver set since it can also stall the request.
            usbd_control_set_complete_callback(NULL);

            if (TUSB_REQ_GET_INTERFACE == p_request->bRequest) {
              uint8_t alternate = 0;
              tud_control_xfer(p_request, &alternate, 1);
            } else {
              tud_control_status(p_request);
            }
            break;

          default:
            return false;
        }
      }
      break;
    }

    //------------- Endpoint Request -------------//
    case TUSB_REQ_RCPT_ENDPOINT: {
      uint8_t const ep_addr = U16_LOW(p_request->wIndex);
      uint8_t const ep_num = usb_endpoint_number(ep_addr);

      if (ep_num >= ARRAY_SIZE(_usbd_dev.ep2drv)) {
        return false;
      }

      if (TUSB_REQ_TYPE_STANDARD != p_request->bmRequestType_bit.type) {
        // Forward class request to its driver
        return invoke_class_control(p_request);
      } else {
        // Handle STD request to endpoint
        switch (p_request->bRequest) {
          case TUSB_REQ_GET_STATUS: {
            uint16_t status = usbd_edpt_stalled(ep_addr) ? 0x0001 : 0x0000;
            tud_control_xfer(p_request, &status, 2);
          } break;

          case TUSB_REQ_CLEAR_FEATURE:
          case TUSB_REQ_SET_FEATURE: {
            if (TUSB_REQ_FEATURE_EDPT_HALT == p_request->wValue) {
              if (TUSB_REQ_CLEAR_FEATURE == p_request->bRequest) {
                usbd_edpt_clear_stall(ep_addr);
              } else {
                usbd_edpt_stall(ep_addr);
              }
            }

            // Some classes such as USBTMC needs to clear/re-init its buffer when receiving CLEAR_FEATURE request
            // We will also forward std request targeted endpoint to class drivers as well

            // STD request must always be ACKed regardless of driver returned value
            // Also clear complete callback if driver set since it can also stall the request.
            invoke_class_control(p_request);
            usbd_control_set_complete_callback(NULL);

            // skip ZLP status if driver already did that
            if (!_usbd_dev.ep_status[0][USB_ENDPOINT_DIRECTION_IN].busy) tud_control_status(p_request);
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

// Process Set Configure Request
// This function parse configuration descriptor & open drivers accordingly
static bool process_set_config(uint8_t cfg_num) {
  // index is cfg_num-1
  tusb_desc_configuration_t const* desc_cfg = (tusb_desc_configuration_t const*)tud_descriptor_configuration_cb(cfg_num - 1);
  if (desc_cfg == NULL || desc_cfg->bDescriptorType != TUSB_DESC_CONFIGURATION) {
    return false;
  }

  // Parse configuration descriptor
  _usbd_dev.remote_wakeup_support = (desc_cfg->bmAttributes & TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP) ? 1u : 0u;
  _usbd_dev.self_powered = (desc_cfg->bmAttributes & TUSB_DESC_CONFIG_ATT_SELF_POWERED) ? 1u : 0u;

  // Parse interface descriptor
  uint8_t const* p_desc = ((uint8_t const*)desc_cfg) + sizeof(tusb_desc_configuration_t);
  uint8_t const* desc_end = ((uint8_t const*)desc_cfg) + desc_cfg->wTotalLength;

  while (p_desc < desc_end) {
    uint8_t assoc_itf_count = 1;

    // Class will always starts with Interface Association (if any) and then Interface descriptor
    if (TUSB_DESC_INTERFACE_ASSOCIATION == tu_desc_type(p_desc)) {
      tusb_desc_interface_assoc_t const* desc_iad = (tusb_desc_interface_assoc_t const*)p_desc;
      assoc_itf_count = desc_iad->bInterfaceCount;

      p_desc = tu_desc_next(p_desc);  // next to Interface
    }

    if (TUSB_DESC_INTERFACE != tu_desc_type(p_desc)) {
      return false;
    }
    tusb_desc_interface_t const* desc_itf = (tusb_desc_interface_t const*)p_desc;

    // Find driver for this interface
    uint16_t const remaining_len = (uint16_t)(desc_end - p_desc);
    uint16_t const drv_len = cdcd_open(desc_itf, remaining_len);

    if ((sizeof(tusb_desc_interface_t) <= drv_len) && (drv_len <= remaining_len)) {
      if (assoc_itf_count == 1) {
        assoc_itf_count = 2;
      }

      // bind (associated) interfaces to found driver
      for (uint8_t i = 0; i < assoc_itf_count; i++) {
        uint8_t const itf_num = desc_itf->bInterfaceNumber + i;

        // Interface number must not be used already
        if (_usbd_dev.itf2drv[itf_num] != 0xFF) {
          return false;
        }
        _usbd_dev.itf2drv[itf_num] = 0;
      }

      // bind all endpoints to found driver
      tu_edpt_bind_driver(_usbd_dev.ep2drv, desc_itf, drv_len);

      // next Interface
      p_desc += drv_len;

      break;  // exit driver find loop
    }
  }

  return true;
}

// return descriptor's buffer and update desc_len
static bool process_get_descriptor(tusb_control_request_t const* p_request) {
  tusb_desc_type_t const desc_type = (tusb_desc_type_t)U16_HIGH(p_request->wValue);
  uint8_t const desc_index = U16_LOW(p_request->wValue);

  switch (desc_type) {
    case TUSB_DESC_DEVICE: {
      void* desc_device = (void*)(uintptr_t)tud_descriptor_device_cb();
      if (!desc_device) {
        return false;
      }

      // Only response with exactly 1 Packet if: not addressed and host requested more data than device descriptor has.
      // This only happens with the very first get device descriptor and EP0 size = 8 or 16.
      if ((USB_EP0_BUFFER_SIZE < sizeof(usb_device_desc_t)) && !_usbd_dev.addressed &&
          ((tusb_control_request_t const*)p_request)->wLength > sizeof(usb_device_desc_t)) {
        // Hack here: we modify the request length to prevent usbd_control response with zlp
        // since we are responding with 1 packet & less data than wLength.
        tusb_control_request_t mod_request = *p_request;
        mod_request.wLength = USB_EP0_BUFFER_SIZE;

        return tud_control_xfer(&mod_request, desc_device, USB_EP0_BUFFER_SIZE);
      } else {
        return tud_control_xfer(p_request, desc_device, sizeof(usb_device_desc_t));
      }
    }
      // break; // unreachable

    case TUSB_DESC_BOS: {
      // requested by host if USB > 2.0 ( i.e 2.1 or 3.x )
      uintptr_t desc_bos = (uintptr_t)tud_descriptor_bos_cb();
      if (!desc_bos) {
        return false;
      }

      // Use offsetof to avoid pointer to the odd/misaligned address
      uint16_t const total_len = tu_unaligned_read16((const void*)(desc_bos + offsetof(tusb_desc_bos_t, wTotalLength)));

      return tud_control_xfer(p_request, (void*)desc_bos, total_len);
    }
      // break; // unreachable

    case TUSB_DESC_CONFIGURATION:
    case TUSB_DESC_OTHER_SPEED_CONFIG: {
      uintptr_t desc_config;

      if (desc_type == TUSB_DESC_CONFIGURATION) {
        desc_config = (uintptr_t)tud_descriptor_configuration_cb(desc_index);
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
      uint16_t const total_len = tu_unaligned_read16((const void*)(desc_config + offsetof(tusb_desc_configuration_t, wTotalLength)));

      return tud_control_xfer(p_request, (void*)desc_config, total_len);
    }
      // break; // unreachable

    case TUSB_DESC_STRING: {
      // String Descriptor always uses the desc set from user
      uint8_t const* desc_str = (uint8_t const*)tud_descriptor_string_cb(desc_index, p_request->wIndex);
      if (!desc_str) {
        return false;
      }

      // first byte of descriptor is its size
      return tud_control_xfer(p_request, (void*)(uintptr_t)desc_str, tu_desc_len(desc_str));
    }
      // break; // unreachable

    case TUSB_DESC_DEVICE_QUALIFIER: {
      uint8_t const* desc_qualifier = tud_descriptor_device_qualifier_cb();
      if (!desc_qualifier) {
        return false;
      }
      return tud_control_xfer(p_request, (void*)(uintptr_t)desc_qualifier, tu_desc_len(desc_qualifier));
    }
      // break; // unreachable

    default:
      return false;
  }
}

//--------------------------------------------------------------------+
// USBD API For Class Driver
//--------------------------------------------------------------------+

// Parse consecutive endpoint descriptors (IN & OUT)
bool usb_endpoint_open_set(uint8_t const* p_desc, uint8_t ep_count, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in) {
  for (int i = 0; i < ep_count; i++) {
    usb_endpoint_descriptor_t const* desc_ep = (usb_endpoint_descriptor_t const*)p_desc;

    if (desc_ep->bDescriptorType != TUSB_DESC_ENDPOINT || desc_ep->bmAttributes.type != xfer_type) {
      return false;
    }

    if (!usbd_edpt_open(desc_ep)) {
      return false;
    }

    if (usb_endpoint_direction(desc_ep->bEndpointAddress) == USB_ENDPOINT_DIRECTION_IN) {
      (*ep_in) = desc_ep->bEndpointAddress;
    } else {
      (*ep_out) = desc_ep->bEndpointAddress;
    }

    p_desc = tu_desc_next(p_desc);
  }

  return true;
}

//--------------------------------------------------------------------+
// USBD Endpoint API
//--------------------------------------------------------------------+

bool usbd_edpt_open(usb_endpoint_descriptor_t const* desc_ep) {
  if (usb_endpoint_number(desc_ep->bEndpointAddress) >= USB_ENDPOINT_MAX) {
    return false;
  }

  if (!tu_edpt_validate(desc_ep, false)) {
    return false;
  }

  return usb_endpoint_open(desc_ep);
}

bool usbd_edpt_claim(uint8_t ep_addr) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);
  endpoint_state_t* ep_state = &_usbd_dev.ep_status[epnum][dir];

  return tu_edpt_claim(ep_state);
}

bool usbd_edpt_release(uint8_t ep_addr) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);
  endpoint_state_t* ep_state = &_usbd_dev.ep_status[epnum][dir];

  return tu_edpt_release(ep_state);
}

bool usbd_edpt_xfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);

  // Attempt to transfer on a busy endpoint, sound like an race condition !
  if (_usbd_dev.ep_status[epnum][dir].busy != 0) {
    return false;
  }

  // Set busy first since the actual transfer can be complete before dcd_edpt_xfer()
  // could return and USBD task can preempt and clear the busy
  _usbd_dev.ep_status[epnum][dir].busy = 1;

  if (dcd_edpt_xfer(ep_addr, buffer, total_bytes)) {
    return true;
  } else {
    // DCD error, mark endpoint as ready to allow next transfer
    _usbd_dev.ep_status[epnum][dir].busy = 0;
    _usbd_dev.ep_status[epnum][dir].claimed = 0;
    return false;
  }
}

bool usbd_edpt_busy(uint8_t ep_addr) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);

  return _usbd_dev.ep_status[epnum][dir].busy;
}

void usbd_edpt_stall(uint8_t ep_addr) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);

  // only stalled if currently cleared
  usb_endpoint_stall(ep_addr);
  _usbd_dev.ep_status[epnum][dir].stalled = 1;
  _usbd_dev.ep_status[epnum][dir].busy = 1;
}

void usbd_edpt_clear_stall(uint8_t ep_addr) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);

  // only clear if currently stalled
  dcd_edpt_clear_stall(ep_addr);
  _usbd_dev.ep_status[epnum][dir].stalled = 0;
  _usbd_dev.ep_status[epnum][dir].busy = 0;
}

bool usbd_edpt_stalled(uint8_t ep_addr) {
  uint8_t const epnum = usb_endpoint_number(ep_addr);
  uint8_t const dir = usb_endpoint_direction(ep_addr);

  return _usbd_dev.ep_status[epnum][dir].stalled;
}
