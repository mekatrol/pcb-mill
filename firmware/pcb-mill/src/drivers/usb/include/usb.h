#ifndef __USB_H__
#define __USB_H__

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "stm32g0xx.h"
#include "macros.h"

// EP0 identifier
#define EP0_IDN 0

// The size of endpoint 0 buffer
#define USB_EP0_BUFFER_SIZE 64UL

// USB registers strong type
#define USB ((USB_DRD_TypeDef*)USB_BASE)

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
  USB_REQUEST_TYPE_STANDARD = 0 << 5,  // 00b
  USB_REQUEST_TYPE_CLASS = 1 << 5,     // 01b
  USB_REQUEST_TYPE_VENDOR = 2 << 5,    // 10b
  USB_REQUEST_TYPE_RESERVED = 3 << 5,  // 11b (reserved in spec)
  USB_REQUEST_TYPE_MASK = 0x60,        // Bits 5..6
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

// Masks
typedef enum {
  USB_EP_NUM_MASK = 0x0F,             // Bits 0..3 = endpoint number
  USB_EP_DIR_MASK = 0x80,             // Bit 7 = direction
  USB_EP_PACKET_SIZE_MASK = 0x7FFUL,  // Bits 0–10 = Max packet size in bytes (0–1024)
} control_request_addr_mask_t;

enum {
  DESCRIPTOR_LEN_OFFSET = 0,
  DESCRIPTOR_TYPE_OFFSET = 1,
  DESCRIPTOR_SUBTYPE_OFFSET = 2
};

// Extract endpoint identifier (0..15)
#define USB_EP_IDN(addr) ((uint8_t)((addr) & USB_EP_NUM_MASK))

// Extract direction (USB_DIR_DEVICE_OUT_HOST_IN or USB_DIR_DEVICE_IN_HOST_OUT)
#define USB_EP_DIR(addr) ((uint8_t)((addr) & USB_EP_DIR_MASK))

// Convert direction bit (USB_DIR_DEVICE_OUT_HOST_IN or USB_DIR_DEVICE_IN_HOST_OUT) to direction index (0x01 or 0x00)
#define USB_EP_DIR_IDX(addr) ((uint8_t)(USB_EP_DIR((addr)) >> 7))

// Build endpoint address from number and direction
#define USB_EP_ADDR(num, dir) ((uint8_t)((num) & USB_EP_NUM_MASK) | ((dir) & USB_EP_DIR_MASK))

// Extract endpoint packet size
#define USB_EP_PACKET_SIZE(packet_size) (((uint32_t)(packet_size)) & USB_EP_PACKET_SIZE_MASK)

// USB Setup Packet (USB 2.0 Spec, Table 9-2: Standard Device Request)
typedef struct __attribute__((packed)) {
  uint8_t bmRequestType;  // Direction, type, recipient
  uint8_t bRequest;       // Request code
  uint16_t wValue;        // Request-specific parameter
  uint16_t wIndex;        // Index (e.g., interface, endpoint)
  uint16_t wLength;       // Number of bytes in data stage
} usb_control_request_t;

_Static_assert(sizeof(usb_control_request_t) == 8, "sizeof(usb_control_request_t) must be 8");

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
} usb_ep_descriptor_t;

_Static_assert(sizeof(usb_ep_descriptor_t) == 7, "size must be 7");

/*
 * The USB device state
 */
typedef struct {
  volatile uint8_t connected : 1;      // Device is connected
  volatile uint8_t addressed : 1;      // Device has been assigned an address
  volatile uint8_t remote_wakeup : 1;  // Remote wakeup enabled
  volatile uint8_t self_powered : 1;   // Device is self-powered
  uint8_t reserved : 4;                // Padding to make a full byte
  uint8_t address_pending;             // USB device address is pending status stage
  uint8_t address;                     // USB device address
  uint8_t config_num;                  // The current device configuration number
} usbd_device_t;

/*
 * USB HAL methods - these must be implmented by a HAL (typically the board code)
 */
void usb_device_start_hal();                                                                                // Start USB in device mode
bool usb_ep_queue_transfer_hal(uint8_t ep_idn, uint8_t ep_dir_idx, uint8_t* buffer, uint16_t total_bytes);  // Queue endpoint transfer in HAL
void usb_device_set_addr_hal(const uint8_t device_addr);
bool usb_ep_stall_get_hal(uint8_t ep_idn, uint8_t ep_dir_idx);
void usb_ep_stall_set_hal(uint8_t ep_idn, uint8_t ep_dir_idx);
void usb_ep_stall_clear_hal(uint8_t ep_idn, uint8_t ep_dir_idx);
bool usb_ep_open_hal(const usb_ep_descriptor_t* ep_descriptor);

/*
 * USB Device methods
 */
bool usb_process_control_request(const usb_control_request_t* request);           // Process a control request
bool usb_control_transfer_complete(uint8_t ep_addr, uint32_t transferred_bytes);  // An EP0 control transfer has completed
bool usb_ep_open_in_out(                                                          // Configure consecutive endpoint descriptors (IN & OUT)
    const usb_ep_descriptor_t* descriptor,
    uint8_t transfer_type,
    uint8_t* ep_addr_out,
    uint8_t* ep_addr_in);

/*
 * USB descriptor methods
 */
const usb_configuration_descriptor_t* usb_descriptor_configuration();
const uint8_t* get_device_descriptor();
const uint16_t* usb_descriptor_string(uint8_t index);
const uint8_t* usb_descriptor_device_qualifier();

extern usbd_device_t usb_device;

/*
 * True if device configured
 */
ALWAYS_INLINE static bool usb_configured(void) {
  return usb_device.config_num != 0;
}

/*
 * Full USB reset
 */
ALWAYS_INLINE static void usb_reset() {
  usb_device_start_hal();
}

/*
 * Request type, recipient and direction helpers
 */
ALWAYS_INLINE static usb_request_recipient_t usb_request_recipient(uint8_t bmRequestType) { return (bmRequestType)&USB_REQUEST_RECIPIENT_MASK; }
ALWAYS_INLINE static usb_request_type_t usb_request_type(uint8_t bmRequestType) { return ((bmRequestType)&USB_REQUEST_TYPE_MASK) >> 5; }
ALWAYS_INLINE static usb_request_direction_index_t usb_request_direction(uint8_t bmRequestType) { return USB_EP_DIR((bmRequestType)) >> 7; }

/*
 * Descriptor helpers
 */

// Return next descriptor
ALWAYS_INLINE static const uint8_t* usb_next_descriptor(const void* desc) {
  const uint8_t* desc8 = (const uint8_t*)desc;
  return desc8 + desc8[DESCRIPTOR_LEN_OFFSET];
}

// Get descriptor length
ALWAYS_INLINE static uint8_t usb_descriptor_len(const void* desc) {
  return ((const uint8_t*)desc)[DESCRIPTOR_LEN_OFFSET];
}

// Get descriptor type
ALWAYS_INLINE static uint8_t usb_descriptor_type(const void* desc) {
  return ((const uint8_t*)desc)[DESCRIPTOR_TYPE_OFFSET];
}

// Get descriptor subtype
ALWAYS_INLINE static uint8_t usb_descriptor_subtype(const void* desc) {
  return ((const uint8_t*)desc)[DESCRIPTOR_SUBTYPE_OFFSET];
}

ALWAYS_INLINE static uint8_t usb_descriptor_is_valid(const void* desc, const uint8_t* desc_end) {
  const uint8_t* desc8 = (const uint8_t*)desc;
  return (desc8 < desc_end) && (usb_next_descriptor(desc) <= desc_end);
}

/*
 * Queue a transfer in HAL
 */
ALWAYS_INLINE static bool usb_ep_queue_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  return usb_ep_queue_transfer_hal(ep_idn, ep_dir_idx, buffer, total_bytes);
}

#endif  // __USB_H__