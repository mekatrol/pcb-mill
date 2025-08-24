#ifndef __USB_TYPES_H__
#define __USB_TYPES_H__

#include <stdint.h>

// USB Descriptor base (common to all descriptors)
typedef struct __attribute__((packed)) {
  uint8_t bLength;          // Size of this descriptor in bytes
  uint8_t bDescriptorType;  // Descriptor type
} usb_descriptor_base_t;

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
  uint8_t bLength;             // Size of this descriptor in bytes (always 18 for device descriptor)
  uint8_t bDescriptorType;     // Descriptor type (DEVICE = 0x01)
  uint16_t bcdUSB;             // USB specification release number in BCD (e.g., 0x0200 = USB 2.0)
  uint8_t bDeviceClass;        // Device class code (assigned by USB-IF; 0 = defined at interface level, 0xFF = vendor-specific)
  uint8_t bDeviceSubClass;     // Device subclass code (qualified by bDeviceClass)
  uint8_t bDeviceProtocol;     // Device protocol code (qualified by bDeviceClass and bDeviceSubClass)
  uint8_t bMaxPacketSize0;     // Maximum packet size for endpoint zero (8, 16, 32, or 64 bytes)
  uint16_t idVendor;           // Vendor ID (assigned by USB-IF)
  uint16_t idProduct;          // Product ID (assigned by manufacturer)
  uint16_t bcdDevice;          // Device release number in BCD (manufacturer-defined)
  uint8_t iManufacturer;       // Index of string descriptor describing manufacturer (0 = none)
  uint8_t iProduct;            // Index of string descriptor describing product (0 = none)
  uint8_t iSerialNumber;       // Index of string descriptor describing the device’s serial number (0 = none)
  uint8_t bNumConfigurations;  // Number of possible configurations supported by the device
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
  volatile uint8_t suspended : 1;      // Set to 1 if device is suspended
  uint8_t reserved : 3;                // Padding to make a full byte
  volatile uint8_t address_pending;    // USB device address is pending status stage
  volatile uint8_t address;            // USB device address
  volatile uint8_t config_num;         // The current device configuration number
} usb_device_t;

#endif  // __USB_TYPES_H__