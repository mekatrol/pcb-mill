#include "usb.h"

typedef struct {
  struct __attribute__((packed)) {
    volatile uint8_t connected : 1;  // USB is connected and ready for use
    volatile uint8_t addressed : 1;  // USB has received address
  };
  volatile uint8_t address;  // USB device address
} usbd_device_t;

usbd_device_t usb_device;

bool process_control_request(const usb_control_request_t* request) {
  // Can flag as connected as seoon as first control request recieved
  usb_device.connected = 1;

  // Get request type
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);

  // Evertything >= USB_REQUEST_TYPE_RESERVED is reserved in spec and should not be used
  if (request_type >= USB_REQUEST_TYPE_RESERVED) {
    return false;
  }

  // Vendor request
  if (request_type == USB_REQUEST_TYPE_VENDOR) {
    // This driver has no vendor specific descriptors
    usbd_control_set_complete_callback(NULL);
    return false;
  }

  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);
  switch (request_recipient) {
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
          usbd_control_set_request(request);

          // Respond with status (ep0)
          usb_ep_transfer_queue_hal(EP0_IDN, USB_DIR_DEVICE_OUT_HOST_IN >> 7, NULL, 0);

          // USB has been addressed
          usb_device.address = request->wValue & 0x7F;  // Address 0 - 127 (7 bit)
          usb_device.addressed = 1;

          diag_printf("USB device address: %d\r\n", usb_device.address);
          break;

        case USB_STD_GET_CONFIGURATION: {
          uint8_t config_num = usb_device.config_num;
          usb_ep_control_transfer(request, &config_num, 1);
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

          usb_control_status(request);
        } break;

        case USB_STD_GET_DESCRIPTOR:
          return process_get_descriptor(request);

        case USB_STD_SET_FEATURE:
          switch (request->wValue) {
            case USB_FEATURE_REMOTE_WAKEUP:
              // Host may enable remote wake up before suspending especially HID device
              usb_device.remote_wakeup_en = true;
              usb_control_status(request);
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
          usb_device.remote_wakeup_en = false;
          usb_control_status(request);
          break;

        case USB_STD_GET_STATUS: {
          // Device status bit mask
          // - Bit 0: Self Powered
          // - Bit 1: Remote Wakeup enabled
          uint16_t status = (uint16_t)((1u) | (usb_device.remote_wakeup_en ? 2u : 0u));
          usb_ep_control_transfer(request, &status, 2);
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
              usb_ep_control_transfer(request, &alternate, 1);
            } else {
              usb_control_status(request);
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
      const uint8_t ep_addr = (uint8_t)(request->wIndex & 0xFF);
      const uint8_t ep_idn = USB_EP_IDN(ep_addr);
      const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

      if (USB_REQUEST_TYPE_STANDARD != request_type) {
        // Forward class request to its driver
        return invoke_class_control(request);
      } else {
        // Handle STD request to endpoint
        switch (request->bRequest) {
          case USB_STD_GET_STATUS: {
            uint16_t status = usb_ep_stall_get_hal(ep_idn, ep_dir_idx) ? 0x0001 : 0x0000;
            usb_ep_control_transfer(request, &status, 2);
          } break;

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
            usbd_control_set_complete_callback(NULL);

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