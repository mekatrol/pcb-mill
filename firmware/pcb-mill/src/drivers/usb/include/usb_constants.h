#ifndef __USB_CONSTANTS_H__
#define __USB_CONSTANTS_H__

// A pair of endpoints IN/OUT
#define EP_IN_OUT_PAIR 2

// EP0 identifier
#define EP0_IDN 0

// The size of endpoint 0 buffer
#define USB_EP0_BUFFER_SIZE 64UL

// USB registers strong type
#define USB ((USB_DRD_TypeDef*)USB_BASE)

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

// USB class codes (see USB-IF Assigned Numbers)
// These values may appear in device, interface, or function descriptors
// depending on how the class is defined.
typedef enum {
  USB_CLASS_CDC = 0x02,  // Communications Device Class (CDC):
                         // Typically used in the Interface Descriptor
                         // for control interfaces of USB CDC devices
                         // (e.g. Abstract Control Model for Virtual COM ports).

  USB_CLASS_CDC_DATA = 0x0A  // CDC Data Class:
                             // Used for the data interface in a CDC device.
                             // Always paired with a CDC control interface.
} usb_class_code_t;

// USB CDC Communication Interface Class Subclass Codes
// Source: USB CDC Specification 1.2, Table 4
// These go in bInterfaceSubClass of the Communication (control) interface descriptor
typedef enum {
  CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL = 0x02
  // Abstract Control Model (ACM):
  // - Used for devices that emulate serial ports (Virtual COM ports).
  // - Implements the PSTN (Public Switched Telephone Network) subclass.
  // - Must be paired with a CDC Data interface.
  // - Defined in "USB PSTN Subclass Specification 1.2".
} cdc_comm_subclass_type_t;

// The interrupt USB endpoint type is one of the four transfer types defined by the USB 2.0 specification:
//   USB Transfer Type	Typical Use	                          Key Properties
//   Control	          Device setup, configuration	          Guaranteed delivery, ordered
//   Isochronous	      Streaming audio/video	                Guaranteed timing, no retries
//   Bulk	              Large, non-time-critical data	        Best-effort delivery
//   Interrupt	        Small, time-sensitive updates	        Guaranteed max latency
typedef enum {
  USB_EP_TYPE_CONTROL = 0,
  USB_EP_TYPE_ISOCHRONOUS = 1,
  USB_EP_TYPE_BULK = 2,
  USB_EP_TYPE_INTERRUPT = 3
} usb_ep_type_t;

// Direction index
typedef enum {
  USB_DIR_DEVICE_IN_HOST_OUT_IDX = 0,  // Host-to-device
  USB_DIR_DEVICE_OUT_HOST_IN_IDX = 1,  // Device-to-host
} usb_request_direction_index_t;

// Direction bit
typedef enum {
  USB_DIR_DEVICE_IN_HOST_OUT = 0x00,  // Host-to-device
  USB_DIR_DEVICE_OUT_HOST_IN = 0x80,  // Device-to-host
} usb_direction_t;

typedef enum {
  // Type (bits 5..6)
  USB_REQUEST_TYPE_STANDARD = 0,  // 00b
  USB_REQUEST_TYPE_CLASS = 1,     // 01b
  USB_REQUEST_TYPE_VENDOR = 2,    // 10b
  USB_REQUEST_TYPE_RESERVED = 3,  // 11b (reserved in spec)
  USB_REQUEST_TYPE_MASK = 0x60,   // Bits 5..6
} usb_request_type_t;

typedef enum {
  // Recipient (bits 0..4)
  USB_REQUEST_RECIPIENT_DEVICE = 0,
  USB_REQUEST_RECIPIENT_INTERFACE = 1,
  USB_REQUEST_RECIPIENT_ENDPOINT = 2,
  USB_REQUEST_RECIPIENT_OTHER = 3,
  USB_REQUEST_RECIPIENT_MASK = 0x1F,  // Bits 0..4
} usb_request_recipient_t;

// Descriptor types recognised by this USB library
typedef enum {
  USB_DESCRIPTOR_TYPE_DEVICE = 0x01,
  USB_DESCRIPTOR_TYPE_CONFIGURATION = 0x02,
  USB_DESCRIPTOR_TYPE_STRING = 0x03,
  USB_DESCRIPTOR_TYPE_INTERFACE = 0x04,
  USB_DESCRIPTOR_TYPE_ENDPOINT = 0x05,
  USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 0x06,
  USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIG = 0x07,
  USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION = 0x0B,
  USB_DESCRIPTOR_TYPE_CS_INTERFACE = 0x24
} usb_descriptor_type_t;

typedef enum {
  // ----- USB 2.0 Standard Requests (bRequest only) -----
  USB_STD_GET_STATUS = 0x00,
  USB_STD_CLEAR_FEATURE = 0x01,
  USB_STD_RESERVED_2 = 0x02,
  USB_STD_SET_FEATURE = 0x03,
  USB_STD_RESERVED_4 = 0x04,
  USB_STD_SET_ADDRESS = 0x05,
  USB_STD_GET_DESCRIPTOR = 0x06,
  USB_STD_SET_DESCRIPTOR = 0x07,
  USB_STD_GET_CONFIGURATION = 0x08,
  USB_STD_SET_CONFIGURATION = 0x09,
  USB_STD_GET_INTERFACE = 0x0A,
  USB_STD_SET_INTERFACE = 0x0B,
  USB_STD_SYNCH_FRAME = 0x0C,

  // ----- USB CDC Class-Specific Requests (bRequest only) -----
  CDC_CLASS_SEND_ENCAPSULATED_COMMAND = 0x00,  // bmRequestType will differ
  CDC_CLASS_GET_ENCAPSULATED_RESPONSE = 0x01,
  CDC_CLASS_SET_COMM_FEATURE = 0x02,
  CDC_CLASS_GET_COMM_FEATURE = 0x03,
  CDC_CLASS_CLEAR_COMM_FEATURE = 0x04,
  CDC_CLASS_SET_LINE_CODING = 0x20,         // 32
  CDC_CLASS_GET_LINE_CODING = 0x21,         // 33
  CDC_CLASS_SET_CONTROL_LINE_STATE = 0x22,  // 34
  CDC_CLASS_SEND_BREAK = 0x23               // 35
} usb_request_code_t;

// USB Standard Feature Selectors (USB 2.0 Spec, Table 9-6)
typedef enum {
  USB_FEATURE_ENDPOINT_HALT = 0,  // Used with CLEAR_FEATURE/SET_FEATURE for endpoints
                                  // Stops endpoint from transmitting/receiving data (stall condition)

  USB_FEATURE_REMOTE_WAKEUP = 1,  // Used with CLEAR_FEATURE/SET_FEATURE for devices
                                  // Allows device to signal resume from suspend (if supported)

  USB_FEATURE_TEST_MODE = 2  // Used only in high-speed devices for test modes (Chapter 9, USB 2.0 spec)
} usb_request_feature_selector_t;

typedef enum {
  USB_EP_NUM_MASK = 0x0F,             // Bits 0..3 = endpoint number
  USB_EP_DIR_MASK = 0x80,             // Bit 7 = direction
  USB_EP_PACKET_SIZE_MASK = 0x7FFUL,  // Bits 0–10 = Max packet size in bytes (0–1024)
} control_request_addr_mask_t;

#endif  // __USB_CONSTANTS_H__