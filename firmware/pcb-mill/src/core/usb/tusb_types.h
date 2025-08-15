#ifndef TUSB_TYPES_H_
#define TUSB_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  USB_DESC_DEVICE = 0x01,
  USB_DESC_CONFIGURATION = 0x02,
  USB_DESC_STRING = 0x03,
  USB_DESC_INTERFACE = 0x04,
  USB_DESC_ENDPOINT = 0x05,
  USB_DESC_DEVICE_QUALIFIER = 0x06,
  USB_DESC_OTHER_SPEED_CONFIG = 0x07,
  USB_DESC_INTERFACE_ASSOCIATION = 0x0B,
  USB_DESC_BOS = 0x0F,
  USB_DESC_CS_INTERFACE = 0x24
} usb_desc_type_t;

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

typedef enum {
  TUSB_REQ_FEATURE_EDPT_HALT = 0,
  TUSB_REQ_FEATURE_REMOTE_WAKEUP = 1
} tusb_request_feature_selector_t;

// https://www.usb.org/defined-class-codes
typedef enum {
  TUSB_CLASS_CDC = 2,
  TUSB_CLASS_CDC_DATA = 10
} usb_class_code_t;

enum {
  USB_DESC_CONFIG_ATT_REMOTE_WAKEUP = 1U << 5,
  USB_DESC_CONFIG_ATT_SELF_POWERED = 1U << 6,
};

// TODO remove
enum {
  DESC_OFFSET_LEN = 0,
  DESC_OFFSET_TYPE = 1,
  DESC_OFFSET_SUBTYPE = 2
};

enum {
  INTERFACE_INVALID_NUMBER = 0xff
};

enum {
  CONTROL_STAGE_IDLE = 0,
  CONTROL_STAGE_SETUP,  // 1
  CONTROL_STAGE_DATA,   // 2
  CONTROL_STAGE_ACK     // 3
};

//--------------------------------------------------------------------+
// USB Descriptors
//--------------------------------------------------------------------+

// USB Binary Device Object Store (BOS) Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;          ///< Size of this descriptor in bytes
  uint8_t bDescriptorType;  ///< CONFIGURATION Descriptor Type
  uint16_t wTotalLength;    ///< Total length of data returned for this descriptor
  uint8_t bNumDeviceCaps;   ///< Number of device capability descriptors in the BOS
} tusb_desc_bos_t;

_Static_assert(sizeof(tusb_desc_bos_t) == 5, "size must be 5");

/// USB Endpoint Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;          // Size of this descriptor in bytes
  uint8_t bDescriptorType;  // ENDPOINT Descriptor Type

  uint8_t bEndpointAddress;  // The address of the endpoint

  struct __attribute__((packed)) {
    uint8_t type : 2;   // Control, Bulk, Interrupt
    uint8_t sync : 2;   // None, Asynchronous, Adaptive, Synchronous
    uint8_t usage : 2;  // Data, Feedback, Implicit feedback
    uint8_t : 2;
  } bmAttributes;

  uint16_t wMaxPacketSize;  // Bit 10..0 : max packet size, bit 12..11 additional transaction per highspeed micro-frame
  uint8_t bInterval;        // Polling interval, in frames or microframes depending on the operating speed
} usb_endpoint_descriptor_t;

_Static_assert(sizeof(usb_endpoint_descriptor_t) == 7, "size must be 7");

/// USB Other Speed Configuration Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;          ///< Size of descriptor
  uint8_t bDescriptorType;  ///< Other_speed_Configuration Type
  uint16_t wTotalLength;    ///< Total length of data returned

  uint8_t bNumInterfaces;       ///< Number of interfaces supported by this speed configuration
  uint8_t bConfigurationValue;  ///< Value to use to select configuration
  uint8_t iConfiguration;       ///< Index of string descriptor
  uint8_t bmAttributes;         ///< Same as Configuration descriptor
  uint8_t bMaxPower;            ///< Same as Configuration descriptor
} tusb_desc_other_speed_t;

/// USB Device Qualifier Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;          ///< Size of descriptor
  uint8_t bDescriptorType;  ///< Device Qualifier Type
  uint16_t bcdUSB;          ///< USB specification version number (e.g., 0200H for V2.00)

  uint8_t bDeviceClass;     ///< Class Code
  uint8_t bDeviceSubClass;  ///< SubClass Code
  uint8_t bDeviceProtocol;  ///< Protocol Code

  uint8_t bMaxPacketSize0;     ///< Maximum packet size for other speed
  uint8_t bNumConfigurations;  ///< Number of Other-speed Configurations
  uint8_t bReserved;           ///< Reserved for future use, must be zero
} tusb_desc_device_qualifier_t;

_Static_assert(sizeof(tusb_desc_device_qualifier_t) == 10, "size must be 10");

// USB String Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;          ///< Size of this descriptor in bytes
  uint8_t bDescriptorType;  ///< Descriptor Type
  uint16_t utf16le[];
} tusb_desc_string_t;

// USB Binary Device Object Store (BOS)
typedef struct __attribute__((packed)) {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDevCapabilityType;
  uint8_t bReserved;
  uint8_t PlatformCapabilityUUID[16];
  uint8_t CapabilityData[];
} tusb_desc_bos_platform_t;

// USB WebUSB URL Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bScheme;
  char url[];
} tusb_desc_webusb_url_t;

// DFU Functional Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;
  uint8_t bDescriptorType;

  union {
    struct __attribute__((packed)) {
      uint8_t bitCanDnload : 1;
      uint8_t bitCanUpload : 1;
      uint8_t bitManifestationTolerant : 1;
      uint8_t bitWillDetach : 1;
      uint8_t reserved : 4;
    } bmAttributes;

    uint8_t bAttributes;
  };

  uint16_t wDetachTimeOut;
  uint16_t wTransferSize;
  uint16_t bcdDFUVersion;
} tusb_desc_dfu_functional_t;

// Get Endpoint number from address
__attribute__((always_inline)) static inline uint8_t usb_endpoint_number(uint8_t addr) {
  return (uint8_t)(addr & (~USB_ENDPOINT_DIRECTION_IN_MASK));
}

__attribute__((always_inline)) static inline uint8_t tu_edpt_addr(uint8_t num, uint8_t dir) {
  return (uint8_t)(num | (dir ? USB_ENDPOINT_DIRECTION_IN_MASK : 0));
}

__attribute__((always_inline)) static inline uint32_t usb_endpoint_packet_size(usb_endpoint_descriptor_t const* desc_ep) {
  return (uint32_t)desc_ep->wMaxPacketSize & 0x7FF;
}

//--------------------------------------------------------------------+
// Descriptor helper
//--------------------------------------------------------------------+

// return next descriptor
__attribute__((always_inline)) static inline uint8_t const* tu_desc_next(void const* desc) {
  uint8_t const* desc8 = (uint8_t const*)desc;
  return desc8 + desc8[DESC_OFFSET_LEN];
}

// get descriptor length
__attribute__((always_inline)) static inline uint8_t tu_desc_len(void const* desc) {
  return ((uint8_t const*)desc)[DESC_OFFSET_LEN];
}

// get descriptor type
__attribute__((always_inline)) static inline uint8_t tu_desc_type(void const* desc) {
  return ((uint8_t const*)desc)[DESC_OFFSET_TYPE];
}

// get descriptor subtype
__attribute__((always_inline)) static inline uint8_t tu_desc_subtype(void const* desc) {
  return ((uint8_t const*)desc)[DESC_OFFSET_SUBTYPE];
}

__attribute__((always_inline)) static inline uint8_t tu_desc_is_valid(void const* desc, uint8_t const* desc_end) {
  const uint8_t* desc8 = (uint8_t const*)desc;
  return (desc8 < desc_end) && (tu_desc_next(desc) <= desc_end);
}

__attribute__((always_inline)) static inline uint16_t usb_get_request_type_code(const usb_control_request_t* request) {
  uint16_t req_code = ((request->bmRequestType & 0x60) << 3) | request->bRequest;
  return req_code;
}

__attribute__((always_inline)) static inline const char* get_usb_request_code_name(const usb_control_request_t* request) {
  uint8_t type = request->bmRequestType & 0x60;  // 0x00=Standard, 0x20=Class, 0x40=Vendor
  uint8_t bRequest = (uint16_t)request->bRequest;

  switch (type) {
    case 0x00:  // ----- Standard -----
      switch (bRequest) {
        case USB_STD_GET_STATUS:
          return "USB_STD_GET_STATUS";
        case USB_STD_CLEAR_FEATURE:
          return "USB_STD_CLEAR_FEATURE";
        case USB_STD_RESERVED_2:
          return "USB_STD_RESERVED_2";
        case USB_STD_SET_FEATURE:
          return "USB_STD_SET_FEATURE";
        case USB_STD_RESERVED_4:
          return "USB_STD_RESERVED_4";
        case USB_STD_SET_ADDRESS:
          return "USB_STD_SET_ADDRESS";
        case USB_STD_GET_DESCRIPTOR:
          return "USB_STD_GET_DESCRIPTOR";
        case USB_STD_SET_DESCRIPTOR:
          return "USB_STD_SET_DESCRIPTOR";
        case USB_STD_GET_CONFIGURATION:
          return "USB_STD_GET_CONFIGURATION";
        case USB_STD_SET_CONFIGURATION:
          return "USB_STD_SET_CONFIGURATION";
        case USB_STD_GET_INTERFACE:
          return "USB_STD_GET_INTERFACE";
        case USB_STD_SET_INTERFACE:
          return "USB_STD_SET_INTERFACE";
        case USB_STD_SYNCH_FRAME:
          return "USB_STD_SYNCH_FRAME";
        default:
          return "UNKNOWN_STD_REQUEST";
      }

    case 0x20:  // ----- Class -----
      switch (bRequest) {
        case CDC_CLASS_SEND_ENCAPSULATED_COMMAND:
          return "CDC_CLASS_SEND_ENCAPSULATED_COMMAND";
        case CDC_CLASS_GET_ENCAPSULATED_RESPONSE:
          return "CDC_CLASS_GET_ENCAPSULATED_RESPONSE";
        case CDC_CLASS_SET_COMM_FEATURE:
          return "CDC_CLASS_SET_COMM_FEATURE";
        case CDC_CLASS_GET_COMM_FEATURE:
          return "CDC_CLASS_GET_COMM_FEATURE";
        case CDC_CLASS_CLEAR_COMM_FEATURE:
          return "CDC_CLASS_CLEAR_COMM_FEATURE";
        case CDC_CLASS_SET_LINE_CODING:
          return "CDC_CLASS_SET_LINE_CODING";
        case CDC_CLASS_GET_LINE_CODING:
          return "CDC_CLASS_GET_LINE_CODING";
        case CDC_CLASS_SET_CONTROL_LINE_STATE:
          return "CDC_CLASS_SET_CONTROL_LINE_STATE";
        case CDC_CLASS_SEND_BREAK:
          return "CDC_CLASS_SEND_BREAK";
        default:
          return "UNKNOWN_CLASS_REQUEST";
      }

    case 0x40:  // ----- Vendor -----
      // Add your vendor-specific requests here
      return "VENDOR_SPECIFIC_REQUEST";

    default:
      return "UNKNOWN_USB_REQUEST_TYPE";
  }
}

#endif  // TUSB_TYPES_H_
