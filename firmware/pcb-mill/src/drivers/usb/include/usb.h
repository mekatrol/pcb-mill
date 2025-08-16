#ifndef __USB_H__
#define __USB_H__

// Standard Headers
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "stm32g0xx.h"

// USB registers strong type
#define USB ((USB_DRD_TypeDef*)USB_BASE)

// The number of endpoints to define, this should be limited to the
// number of physical endpoints available for the device, however it can
// be less if not all endpoints are needed. Reducing this value saves memory
// by not allocating unused endpoint memory.
// Endpoints are:
//   - ep0 - control endpoint (mandatory)
//   - ep1 - bulk (CDC virtual com)
//   - ep2 - interrupt
#define USB_EP_MAX 3

// The size of endpoint 0 buffer
#define USB_EP0_BUFFER_SIZE 64UL

// The size of endpoint 0 address
#define USB_EP0_ADDR 0

// The size of other endpoint buffers (e.g. CDC, MSC)
#define USB_EP_RX_BUFFER_SIZE 64UL
#define USB_EP_TX_BUFFER_SIZE 64UL

// Direction bit (from host perspective)
#define USB_DIR_OUT 0x00  // Host-to-device
#define USB_DIR_IN 0x80   // Device-to-host

// Masks
#define USB_EP_NUM_MASK 0x0F             // Bits 0..3 = endpoint number
#define USB_EP_DIR_MASK 0x80             // Bit 7 = direction
#define USB_EP_PACKET_SIZE_MASK 0x7FFUL  // Bits 0–10 = Max packet size in bytes (0–1024)

// Extract endpoint number (0..15)
#define USB_EP_NUM(addr) ((uint8_t)((addr) & USB_EP_NUM_MASK))

// Extract direction (USB_DIR_IN or USB_DIR_OUT)
#define USB_EP_DIR(addr) ((uint8_t)((addr) & USB_EP_DIR_MASK))

// Convert direction bit (USB_DIR_IN or USB_DIR_OUT) to direction index (0x01 or 0x00)
#define USB_EP_DIR_IDX(addr) ((uint8_t)(USB_EP_DIR((addr)) >> 7))

// Build endpoint address from number and direction
#define USB_EP_ADDR(num, dir) ((uint8_t)((num) & USB_EP_NUM_MASK) | ((dir) & USB_EP_DIR_MASK))

// Extract endpoint packet size
#define USB_EP_PACKET_SIZE(packet_size) (((uint32_t)(packet_size)) & USB_EP_PACKET_SIZE_MASK)

// See Table 237. Transmission status encoding in RM0444
typedef enum {
  USB_EP_STATE_DISABLED = 0b00,
  USB_EP_STATE_STALL = 0b01,
  USB_EP_STATE_NAK = 0b10,
  USB_EP_STATE_VALID = 0b11
} usb_endpoint_state_t;

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
} usb_endpoint_type_t;

// This is the same as USB_DRD_PMABuffDescTypeDef
typedef struct
{
  union {
    struct {
      volatile uint32_t count_addr;
    } buffer[2];

    volatile uint32_t TXBD;
    volatile uint32_t RXBD;
  };
} usb_buffer_description_table_t;

// Buffer Table is located in Packet Memory Area (PMA)
typedef struct {
  usb_buffer_description_table_t endpoint[USB_EP_MAX];
} usb_buffer_description_table_map_t;

// Buffers can be placed anywhere inside the packet memory because their location and size is specified in a buffer description table,
// which is also located in the packet memory
#define USB_BUFFER_DESC_TABLE ((volatile usb_buffer_description_table_map_t*)(USB_DRD_PMAADDR))

void usbd_control_reset();
void usb_configuration_reset();
void handle_bus_reset();

// Initialize USN in device mode
void usb_device_init();

__attribute__((always_inline)) static inline uint16_t min_u16(uint16_t x, uint16_t y) { return (x < y) ? x : y; }

__attribute__((always_inline)) static inline void usb_reset() {
  handle_bus_reset();
  usb_configuration_reset();
  usbd_control_reset();
}

bool usb_init_driver();

typedef enum {
  USB_REQUEST_RECIPIENT_DEVICE = 0,
  USB_REQUEST_RECIPIENT_INTERFACE,
  USB_REQUEST_RECIPIENT_ENDPOINT,

  USB_REQUEST_RECIPIENT_MASK = 0x1F
} usb_request_recipient_t;

typedef enum {
  USB_REQUEST_TYPE_STANDARD = 0,  // 00
  USB_REQUEST_TYPE_CLASS = 1,     // 01
  USB_REQUEST_TYPE_VENDOR = 2,    // 10
  USB_REQUEST_TYPE_RESERVED = 3,  // 11 -> reserved in spec

  USB_REQUEST_TYPE_MASK = 0x60
} usb_request_type_t;

typedef enum {
  USB_EP_DIRECTION_OUT_IDX = 0,
  USB_EP_DIRECTION_IN_IDX = 1,
} usb_endpoint_direction_index_t;

/// Standard USB control request (USB 2.0 Spec, Table 9-2)
typedef struct __attribute__((packed)) {
  uint8_t bmRequestType;  // Request characteristics: direction, type, and recipient (see USB 2.0 §9.3, Table 9-2)
  uint8_t bRequest;       // Specific request code (standard, class, or vendor-specific)
  uint16_t wValue;        // Request-specific parameter (meaning depends on bRequest)
  uint16_t wIndex;        // Typically an index or offset; often used for interface or endpoint number
  uint16_t wLength;       // Number of bytes to transfer in the data stage (host → device or device → host)
} usb_control_request_t;

_Static_assert(sizeof(usb_control_request_t) == 8, "sizeof(usb_control_request_t) must be 8");

__attribute__((always_inline)) static inline usb_request_recipient_t usb_request_recipient(uint8_t bm) { return (bm)&USB_REQUEST_RECIPIENT_MASK; }
__attribute__((always_inline)) static inline usb_request_type_t usb_request_type(uint8_t bm) { return ((bm)&USB_REQUEST_TYPE_MASK) >> 5; }
__attribute__((always_inline)) static inline usb_endpoint_direction_index_t usb_request_direction(uint8_t bm) { return USB_EP_DIR((bm)) >> 7; }

// USB Device Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;             ///< Size of this descriptor in bytes (always 18 for device descriptor)
  uint8_t bDescriptorType;     ///< Descriptor type (DEVICE = 0x01)
  uint16_t bcdUSB;             ///< USB specification release number in BCD (e.g., 0x0200 = USB 2.0)
  uint8_t bDeviceClass;        ///< Device class code (assigned by USB-IF; 0 = defined at interface level, 0xFF = vendor-specific)
  uint8_t bDeviceSubClass;     ///< Device subclass code (qualified by bDeviceClass)
  uint8_t bDeviceProtocol;     ///< Device protocol code (qualified by bDeviceClass and bDeviceSubClass)
  uint8_t bMaxPacketSize0;     ///< Maximum packet size for endpoint zero (8, 16, 32, or 64 bytes)
  uint16_t idVendor;           ///< Vendor ID (assigned by USB-IF)
  uint16_t idProduct;          ///< Product ID (assigned by manufacturer)
  uint16_t bcdDevice;          ///< Device release number in BCD (manufacturer-defined)
  uint8_t iManufacturer;       ///< Index of string descriptor describing manufacturer (0 = none)
  uint8_t iProduct;            ///< Index of string descriptor describing product (0 = none)
  uint8_t iSerialNumber;       ///< Index of string descriptor describing the device’s serial number (0 = none)
  uint8_t bNumConfigurations;  ///< Number of possible configurations supported by the device
} usb_device_descriptor_t;

_Static_assert(sizeof(usb_device_descriptor_t) == 18, "sizeof(usb_device_descriptor_t) must be 18");

// Configuration Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;              // Size of this descriptor in bytes (always 9 for a configuration descriptor)
  uint8_t bDescriptorType;      // Descriptor type (CONFIGURATION = 0x02)
  uint16_t wTotalLength;        // Total length of all descriptors returned for this configuration, including all interface, endpoint, and class/vendor descriptors
  uint8_t bNumInterfaces;       // Number of interfaces supported by this configuration
  uint8_t bConfigurationValue;  // Value used in the SetConfiguration request to select this configuration
  uint8_t iConfiguration;       // Index of string descriptor describing this configuration (0 if none)
  uint8_t bmAttributes;         // Configuration characteristics:
                                //   Bit 7: Reserved, must be 1
                                //   Bit 6: Self-powered (1 = device is self-powered, 0 = bus-powered)
                                //   Bit 5: Remote wakeup (1 = device can wake host from suspend)
                                //   Bits 4..0: Reserved, must be 0
  uint8_t bMaxPower;            // Maximum power consumption from the USB bus in this configuration, in 2 mA units
                                //   Example: 50 = 100 mA
} usb_configuration_descriptor_t;

_Static_assert(sizeof(usb_configuration_descriptor_t) == 9, "sizeof(usb_configuration_descriptor_t) must be 9");

// Interface Association Descriptor (IAD) for CDC
typedef struct __attribute__((packed)) {
  uint8_t bLength;            // Size of this descriptor in bytes (8)
  uint8_t bDescriptorType;    // INTERFACE_ASSOCIATION (0x0B)
  uint8_t bFirstInterface;    // First interface associated with this function
  uint8_t bInterfaceCount;    // Number of interfaces associated (CDC control + data)
  uint8_t bFunctionClass;     // CDC class code
  uint8_t bFunctionSubClass;  // Abstract Control Model (ACM)
  uint8_t bFunctionProtocol;  // No protocol
  uint8_t iFunction;          // Index of string descriptor for this function (0 = none)
} usb_interface_association_descriptor_t;

_Static_assert(sizeof(usb_interface_association_descriptor_t) == 8, "sizeof(usb_interface_association_descriptor_t) must be 8");

// CDC Control Interface Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;             // Size of this descriptor in bytes (always 9 for an interface descriptor)
  uint8_t bDescriptorType;     // Descriptor type (INTERFACE = 0x04)
  uint8_t bInterfaceNumber;    // Interface index (zero-based) identifying this interface within the configuration
  uint8_t bAlternateSetting;   // Alternate setting number for this interface (0 if not using alternate settings)
  uint8_t bNumEndpoints;       // Number of endpoints used by this interface (excluding endpoint 0). 0 means this interface uses only the default control endpoint.
  uint8_t bInterfaceClass;     // Class code assigned by USB-IF. Examples: 0x02 = CDC, 0x0A = Data, 0xFF = Vendor-specific
  uint8_t bInterfaceSubClass;  // Subclass code assigned by USB-IF, qualified by bInterfaceClass. For CDC ACM, this is 0x02 (Abstract Control Model)
  uint8_t bInterfaceProtocol;  // Protocol code assigned by USB-IF, qualified by bInterfaceClass and bInterfaceSubClass. 0 = no specific protocol, 0xFF = vendor-specific
  uint8_t iInterface;          // Index of string descriptor describing this interface (0 = no string)
} usb_control_interface_descriptor_t;

_Static_assert(sizeof(usb_control_interface_descriptor_t) == 9, "size must be 9");

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

  uint16_t wMaxPacketSize;  // Bits 0–10  = Max packet size in bytes (0–1024)
                            // Bits 11–12 = Additional transactions per microframe (high-speed only)
                            // Bits 13–15 = Reserved (must be zero)
  uint8_t bInterval;        // Polling interval, in frames or microframes depending on the operating speed
} usb_endpoint_descriptor_t;

_Static_assert(sizeof(usb_endpoint_descriptor_t) == 7, "size must be 7");

typedef struct __attribute__((packed)) {
  volatile uint8_t busy : 1;
  volatile uint8_t stalled : 1;
  volatile uint8_t claimed : 1;
} endpoint_state_t;

typedef struct {
  struct __attribute__((packed)) {
    volatile uint8_t connected : 1;
    volatile uint8_t addressed : 1;

    uint8_t remote_wakeup_en : 1;       // enable/disable by host
    uint8_t remote_wakeup_support : 1;  // configuration descriptor's attribute
    uint8_t self_powered : 1;           // configuration descriptor's attribute
  };
  volatile uint8_t cfg_num;  // current active configuration (0x00 is not configured)

  uint8_t ep2drv[USB_EP_MAX][2];  // map endpoint to driver ( 0xff is invalid ), can use only 4-bit each

  endpoint_state_t ep_status[USB_EP_MAX][2];

} usbd_device_t;

/// CDC ACM (Virtual COM Port) Class-Specific Request Codes
/// See USB CDC Spec 1.2, Table 3.1 (Abstract Control Model Requests)
typedef enum {
  CDC_REQUEST_SET_LINE_CODING = 0x20,         // Set serial line coding (baud rate, stop bits, parity, data bits) :contentReference[oaicite:0]{index=0}
  CDC_REQUEST_GET_LINE_CODING = 0x21,         // Get current serial line coding :contentReference[oaicite:1]{index=1}
  CDC_REQUEST_SET_CONTROL_LINE_STATE = 0x22,  // Control RTS/DTR tone (host signals presence) :contentReference[oaicite:2]{index=2}
  CDC_REQUEST_SEND_BREAK = 0x23               // Transmit break condition on the communication line :contentReference[oaicite:3]{index=3}
} cdc_acm_request_t;

// USB CDC Communication Interface Class Subclass Codes (CDC Spec 1.2 Table 4)
typedef enum {
  CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL = 0x02  // Abstract Control Model             [USB PSTN 1.2]
} cdc_comm_subclass_type_t;

// For a USB CDC Virtual COM Port, this struct represents the Line Coding object defined in USB CDC Specification 1.2, Section 6.2.13.
typedef struct __attribute__((packed)) {
  uint32_t dwDTERate;   // Data terminal rate in bits per second (baud rate)
  uint8_t bCharFormat;  // Stop bits: 0 = 1 stop bit, 1 = 1.5 stop bits, 2 = 2 stop bits
  uint8_t bParityType;  // Parity: 0 = None, 1 = Odd, 2 = Even, 3 = Mark, 4 = Space
  uint8_t bDataBits;    // Number of data bits: typically 5, 6, 7, 8, or 16
} usb_cdc_line_coding_t;

_Static_assert(sizeof(usb_cdc_line_coding_t) == 7, "size must be 7");

const usb_configuration_descriptor_t* usb_descriptor_configuration();

typedef bool (*usbd_control_xfer_cb_t)(uint8_t stage, usb_control_request_t const* request);

// Submit a usb transfer
bool usb_endpoint_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes);

// Claim an endpoint before submitting a transfer.
// If caller does not make any transfer, it must release endpoint for others.
bool usb_endpoint_claim(uint8_t ep_addr);

// Release claimed endpoint without submitting a transfer
bool usb_endpoint_release(uint8_t ep_addr);

// Set endpoint stalled
void usb_endpoint_stall_set(uint8_t ep_addr);

// Clear endpoint stalled
void usb_endpoint_stall_clear(uint8_t ep_addr);

// Check if endpoint is stalled
bool usb_endpoint_is_stalled(uint8_t ep_addr);

// Open a set of output and input endpoints
bool usb_endpoint_open_in_out(const usb_endpoint_descriptor_t* p_desc, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in);

// Bind all endpoint of a interface descriptor to class driver
void tu_edpt_bind_driver(uint8_t ep2drv[][2], usb_control_interface_descriptor_t const* p_desc, uint16_t desc_len);

// Claim an endpoint
bool tu_edpt_claim(endpoint_state_t* ep_state);

// Release an endpoint
bool tu_edpt_release(endpoint_state_t* ep_state);

extern usbd_device_t usb_device;

bool process_control_request(usb_control_request_t const* request);

// Check if device is connected (may not mounted/configured yet)
// True if just got out of Bus Reset and received the very first data from host
bool tud_connected(void);

// True if device configured
__attribute__((always_inline)) static inline bool tud_ready(void) {
  return usb_device.cfg_num ? true : false;
}

// Carry out Data and Status stage of control transfer
// - If len = 0, it is equivalent to sending status only
// - If len > wLength : it will be truncated
bool usb_endpoint_control_transfer(const usb_control_request_t* request, void* buffer, uint16_t len);

// Send STATUS (zero length) packet
bool tud_control_status(usb_control_request_t const* request);

uint8_t const* get_device_descriptor(void);
uint16_t const* usb_descriptor_string(uint8_t index, uint16_t langid);
uint8_t const* usb_descriptor_device_qualifier(void);
uint8_t const* usb_descriptor_other_speed_configuration(uint8_t index);

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
  USB_DESCRIPTOR_TYPE_CONFIG_ATT_REMOTE_WAKEUP = 1U << 5,
  USB_DESCRIPTOR_TYPE_CONFIG_ATT_SELF_POWERED = 1U << 6,
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

/// USB Other Speed Configuration Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;              // Size of descriptor
  uint8_t bDescriptorType;      // Other_speed_Configuration Type
  uint16_t wTotalLength;        // Total length of data returned
  uint8_t bNumInterfaces;       // Number of interfaces supported by this speed configuration
  uint8_t bConfigurationValue;  // Value to use to select configuration
  uint8_t iConfiguration;       // Index of string descriptor
  uint8_t bmAttributes;         // Same as Configuration descriptor
  uint8_t bMaxPower;            // Same as Configuration descriptor
} usb_descriptor_other_speed_t;

/// USB Device Qualifier Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;             // Size of descriptor
  uint8_t bDescriptorType;     // Device Qualifier Type
  uint16_t bcdUSB;             // USB specification version number (e.g., 0200H for V2.00
  uint8_t bDeviceClass;        // Class Code
  uint8_t bDeviceSubClass;     // SubClass Code
  uint8_t bDeviceProtocol;     // Protocol Code
  uint8_t bMaxPacketSize0;     // Maximum packet size for other speed
  uint8_t bNumConfigurations;  // Number of Other-speed Configurations
  uint8_t bReserved;           // Reserved for future use, must be zero
} tusb_descriptor_device_qualifier_t;

_Static_assert(sizeof(tusb_descriptor_device_qualifier_t) == 10, "size must be 10");

// USB String Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;          ///< Size of this descriptor in bytes
  uint8_t bDescriptorType;  ///< Descriptor Type
  uint16_t utf16le[];
} tusb_descriptor_string_t;

// return next descriptor
__attribute__((always_inline)) static inline uint8_t const* usb_next_descriptor(void const* desc) {
  uint8_t const* desc8 = (uint8_t const*)desc;
  return desc8 + desc8[DESC_OFFSET_LEN];
}

// get descriptor length
__attribute__((always_inline)) static inline uint8_t usb_descriptor_len(void const* desc) {
  return ((uint8_t const*)desc)[DESC_OFFSET_LEN];
}

// get descriptor type
__attribute__((always_inline)) static inline uint8_t usb_descriptor_type(void const* desc) {
  return ((uint8_t const*)desc)[DESC_OFFSET_TYPE];
}

// get descriptor subtype
__attribute__((always_inline)) static inline uint8_t tu_desc_subtype(void const* desc) {
  return ((uint8_t const*)desc)[DESC_OFFSET_SUBTYPE];
}

__attribute__((always_inline)) static inline uint8_t tu_desc_is_valid(void const* desc, uint8_t const* desc_end) {
  const uint8_t* desc8 = (uint8_t const*)desc;
  return (desc8 < desc_end) && (usb_next_descriptor(desc) <= desc_end);
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

// Enable/Disable Start-of-frame interrupt. Default is disabled
void usb_sof_set_enable(bool en);

// Invoked when a control transfer's status stage is complete.
// May help DCD to prepare for next control transfer, this API is optional.
void usb_endpoint_control_status_complete(usb_control_request_t const* request);

// Configure endpoint's registers according to descriptor
bool usb_endpoint_open(usb_endpoint_descriptor_t const* endpoint_descriptor);

// Close all endpoints
void usb_endpoint_close_all();

// Submit a transfer
bool usb_endpoint_transfer_hal(uint8_t ep_num, uint8_t ep_dir_idx, uint8_t* buffer, uint16_t total_bytes);

// Stall endpoint, any queuing transfer should be removed from endpoint
void usb_endpoint_stall_set(uint8_t ep_addr);
void usb_endpoint_stall_set_hal(uint8_t ep_num, uint8_t ep_dir_idx);

// clear stall, data toggle is also reset to DATA0
// This API never calls with control endpoints, since it is auto cleared when receiving setup packet
void usb_endpoint_stall_clear(uint8_t ep_addr);
void usb_endpoint_stall_clear_hal(uint8_t ep_num, uint8_t ep_dir_idx);

#endif  // __USB_H__