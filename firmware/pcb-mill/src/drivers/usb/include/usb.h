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
#define USB ((USB_DRD_TypeDef *)USB_BASE)

// The number of endpoints to define, this should be limited to the
// number of physical endpoints available for the device, however it can
// be less if not all endpoints are needed. Reducing this value saves memory
// by not allocating unused endpoint memory.
// Endpoints are:
//   - ep0 - control endpoint (mandatory)
//   - ep1 - bulk (CDC virtual com)
//   - ep2 - interrupt
#define USB_ENDPOINT_MAX 3

// The size of endpoint 0 buffer
#define USB_EP0_BUFFER_SIZE 64UL

// The size of endpoint 0 address
#define USB_EP0_ADDR 0

// The size of other endpoint buffers (e.g. CDC, MSC)
#define USB_ENDPOINT_RX_BUFFER_SIZE 64UL
#define USB_ENDPOINT_TX_BUFFER_SIZE 64UL

// See Table 237. Transmission status encoding in RM0444
typedef enum {
  USB_ENDPOINT_STATE_DISABLED = 0b00,
  USB_ENDPOINT_STATE_STALL = 0b01,
  USB_ENDPOINT_STATE_NAK = 0b10,
  USB_ENDPOINT_STATE_VALID = 0b11
} usb_endpoint_state_t;

// The interrupt USB endpoint type is one of the four transfer types defined by the USB 2.0 specification:
//   USB Transfer Type	Typical Use	                          Key Properties
//   Control	          Device setup, configuration	          Guaranteed delivery, ordered
//   Isochronous	      Streaming audio/video	                Guaranteed timing, no retries
//   Bulk	              Large, non-time-critical data	        Best-effort delivery
//   Interrupt	        Small, time-sensitive updates	        Guaranteed max latency
typedef enum {
  USB_ENDPOINT_TYPE_CONTROL = 0,
  USB_ENDPOINT_TYPE_ISOCHRONOUS = 1,
  USB_ENDPOINT_TYPE_BULK = 2,
  USB_ENDPOINT_TYPE_INTERRUPT = 3
} usb_endpoint_type_t;

// The maximum number of USB interfaces across all configurations
// Each USB interface is as defined in the USB 2.0 spec (see Section 9.6.5 Interface Descriptor).
// An interface is a logical grouping of endpoints and functions.
// Two interfaces supported:
//  * CDC (Communications Device Class ) ACM (Abstract Control Model) [Virtual COM port]
//  * MSC (Mass Storage Class)  [SD card]
#define USB_MAX_INTERFACES 1

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
  usb_buffer_description_table_t endpoint[USB_ENDPOINT_MAX];
} usb_buffer_description_table_map_t;

// Buffers can be placed anywhere inside the packet memory because their location and size is specified in a buffer description table,
// which is also located in the packet memory
#define USB_BUFFER_DESC_TABLE ((volatile usb_buffer_description_table_map_t *)(USB_DRD_PMAADDR))

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
  USB_DESCRIPTOR_TYPE_CONFIGURATION = 0x02,
  USB_DESCRIPTOR_TYPE_INTERFACE = 0x04,
  USB_DESCRIPTOR_TYPE_ENDPOINT = 0x05,
  USB_DESCRIPTOR_TYPE_ASSOCIATION = 0x0B,
  USB_DESCRIPTOR_TYPE_CS_INTERFACE = 0x24
} usb_descriptor_type_t;

typedef enum {
  USB_REQUEST_TYPE_STANDARD = 0,  // 00
  USB_REQUEST_TYPE_CLASS = 1,     // 01
  USB_REQUEST_TYPE_VENDOR = 2,    // 10
  USB_REQUEST_TYPE_RESERVED = 3,  // 11 -> reserved in spec

  USB_REQUEST_TYPE_MASK = 0x60
} usb_request_type_t;

// See: Bit 4 DIR: Direction of transaction USB interrupt status register (USB_ISTR) in RM0444
typedef enum {
  USB_ENDPOINT_DIRECTION_OUT = 0,  // VTTX bit is set in the USB_CHEPnR register
  USB_ENDPOINT_DIRECTION_IN = 1,   // VTRX bit or both VTTX/VTRX are set in the USB_CHEPnR

  USB_ENDPOINT_DIRECTION_IN_MASK = 0x80
} usb_endpoint_direction_t;

/// Standard USB control request (USB 2.0 Spec, Table 9-2)
typedef struct __attribute__((packed)) {
  uint8_t bmRequestType;  // Request characteristics: direction, type, and recipient (see USB 2.0 §9.3, Table 9-2)
  uint8_t bRequest;       // Specific request code (standard, class, or vendor-specific)
  uint16_t wValue;        // Request-specific parameter (meaning depends on bRequest)
  uint16_t wIndex;        // Typically an index or offset; often used for interface or endpoint number
  uint16_t wLength;       // Number of bytes to transfer in the data stage (host → device or device → host)
} usb_control_request_t;

_Static_assert(sizeof(usb_control_request_t) == 8, "sizeof(usb_control_request_t) must be 8");

__attribute__((always_inline)) static inline usb_request_recipient_t usb_request_recipient(uint8_t bm) { return bm & USB_REQUEST_RECIPIENT_MASK; }
__attribute__((always_inline)) static inline usb_request_type_t usb_request_type(uint8_t bm) { return (bm & USB_REQUEST_TYPE_MASK) >> 5; }
__attribute__((always_inline)) static inline usb_endpoint_direction_t usb_request_direction(uint8_t bm) { return (bm & USB_ENDPOINT_DIRECTION_IN_MASK) >> 7; }
__attribute__((always_inline)) static inline usb_endpoint_direction_t usb_endpoint_direction(uint8_t addr) { return (addr & USB_ENDPOINT_DIRECTION_IN_MASK) >> 7; }

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
} usb_device_desc_t;

_Static_assert(sizeof(usb_device_desc_t) == 18, "sizeof(usb_device_desc_t) must be 18");

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

const usb_configuration_descriptor_t *usb_descriptor_configuration();

#endif  // __USB_H__