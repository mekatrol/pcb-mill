
#include <stdint.h>

#include "usb.h"
#include "usb_cdc.h"

// -----------------------------------------------------------------------------
// USB Device Descriptor (See USB 2.0 Spec, Section 9.6.1)
// This descriptor tells the host the overall characteristics of the USB device
// before it requests configuration/interface/endpoint descriptors.
// -----------------------------------------------------------------------------
const static usb_device_descriptor_t descriptor_device = {
    .bLength = sizeof(usb_device_descriptor_t),     // Size of this descriptor in bytes (should be 18 for a device descriptor)
    .bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE,  // Descriptor Type: DEVICE (0x01)

    .bcdUSB = 0x0200,  // USB Specification version: 2.00 (BCD format)

    // Device class information:
    //
    // According to USB-IF assigned numbers:
    //   bDeviceClass    = 0xEF → Miscellaneous Device Class
    //   bDeviceSubClass = 0x02 → Common Class
    //   bDeviceProtocol = 0x01 → Interface Association Descriptor (IAD)
    //
    // This 0xEF / 0x02 / 0x01 triple is the standard for composite devices
    // that group multiple interfaces (e.g., CDC + HID) under a single device.
    .bDeviceClass = 0xEF,     // Device class code: Miscellaneous
    .bDeviceSubClass = 0x02,  // Subclass: Common
    .bDeviceProtocol = 0x01,  // Protocol: IAD

    .bMaxPacketSize0 = USB_EP0_BUFFER_SIZE,  // Max packet size for control endpoint 0 (8, 16, 32, or 64 bytes;
                                             // must be 64 for high-speed devices)

    // Vendor and product identification:
    .idVendor = 0x0483,   // Vendor ID (assigned by USB-IF) — 0x0483 = STMicroelectronics
    .idProduct = 0x5740,  // Product ID (assigned by manufacturer)
    .bcdDevice = 0x0100,  // Device release number in BCD (e.g., 0x0100 = version 1.00)

    // String descriptor indices (0 means no string):
    .iManufacturer = 0x01,  // Index of manufacturer string descriptor
    .iProduct = 0x02,       // Index of product string descriptor
    .iSerialNumber = 0x03,  // Index of serial number string descriptor

    .bNumConfigurations = 0x01,  // Number of configurations this device supports
};

const uint8_t* usb_get_device_descriptor(void) {
  return (const uint8_t*)&descriptor_device;
}

// Configuration descriptor defined interfaces
enum {
  // CDC interface number
  INTERFACE_CDC_NUM = 0,

  // CDC data interface number
  INTERFACE_CDC_DATA_NUM = 1,

  // CDC total interface count
  INTERFACE_CDC_INTERFACE_COUNT = 2,

  // Total device interface count (CDC control + CDC data)
  INTERFACE_TOTAL_COUNT = INTERFACE_CDC_INTERFACE_COUNT
};

const uint8_t usb_configuration_descriptor[] = {
    // Configuration Descriptor (usb_configuration_descriptor_t)
    // Purpose: Declares one complete configuration.
    9,                                  // bLength: Size of this descriptor in bytes (always 9)
    USB_DESCRIPTOR_TYPE_CONFIGURATION,  // bDescriptorType: CONFIGURATION descriptor (0x02)
    0x4B,                               // wTotalLength (low byte): total length of all descriptors for this configuration
    0x00,                               // wTotalLength (high byte): total length high byte
    INTERFACE_TOTAL_COUNT,              // bNumInterfaces: total number of interfaces in this configuration
    1,                                  // bConfigurationValue: value used in SetConfiguration request
    0,                                  // iConfiguration: index of string descriptor describing this configuration (0 = none)
    0x80,                               // bmAttributes: D7 = 1 (reserved), D6 = 0 (bus-powered), D5 = 0 (no remote wakeup)
    50,                                 // bMaxPower: maximum power in 2 mA units (50 = 100 mA)

    // Interface Association Descriptor (IAD) for CDC (usb_interface_association_descriptor_t)
    // Purpose: Groups the CDC control interface and data interface as one "function" so the OS knows they belong together.
    8,                                          // bLength: size of this descriptor in bytes (8)
    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,  // bDescriptorType: INTERFACE_ASSOCIATION (0x0B)
    INTERFACE_CDC_NUM,                          // bFirstInterface: first interface associated with this function
    INTERFACE_CDC_INTERFACE_COUNT,              // bInterfaceCount: number of interfaces associated (CDC control + data)
    0x02,                                       // bFunctionClass: CDC class code
    0x02,                                       // bFunctionSubClass: Abstract Control Model (ACM)
    0x00,                                       // bFunctionProtocol: No protocol
    0,                                          // iFunction: index of string descriptor for this function (0 = none)

    // CDC Control Interface Descriptor (usb_control_interface_descriptor_t)
    // Purpose: Declares the control interface (the "management" side of the CDC)
    9,                              // bLength: size of interface descriptor
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType: INTERFACE (0x04)
    INTERFACE_CDC_NUM,              // bInterfaceNumber: CDC control interface number
    0,                              // bAlternateSetting: alternate setting number
    1,                              // bNumEndpoints: number of endpoints used by this interface (excluding EP0)
    0x02,                           // bInterfaceClass: CDC (Communications Device Class)
    0x02,                           // bInterfaceSubClass: ACM
    0x00,                           // bInterfaceProtocol: No protocol
    4,                              // iInterface: index of string descriptor describing this interface

    // CDC Header Functional Descriptor
    // Purpose: Tells the host "this interface follows the CDC functional descriptor set."
    5,                                 // bFunctionLength: size of this descriptor
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE (0x24)
    0x00,                              // bDescriptorSubtype: Header functional descriptor (0x00)
    0x20,                              // bcdCDC (low byte): CDC spec release 2.0
    0x01,                              // bcdCDC (high byte)

    // CDC Call Management Functional Descriptor
    5,                                 // bFunctionLength: size of descriptor
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE
    0x01,                              // bDescriptorSubtype: Call Management (0x01)
    0,                                 // bmCapabilities: no call management over data interface
    INTERFACE_CDC_DATA_NUM,            // bDataInterface: interface number of data class interface

    // CDC ACM Functional Descriptor
    // Purpose: Needed for COM port drivers (e.g. Windows uses this to load usbser.sys)
    4,                                 // bFunctionLength: size of descriptor
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE
    0x02,                              // bDescriptorSubtype: Abstract Control Management (0x02)
    6,                                 // bmCapabilities: supports Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding

    // CDC Union Functional Descriptor
    // Purpose: This ties the two interfaces into one logical device
    5,                                 // bFunctionLength: size of descriptor
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE
    0x06,                              // bDescriptorSubtype: Union functional descriptor (0x06)
    INTERFACE_CDC_NUM,                 // bMasterInterface: CDC control interface
    INTERFACE_CDC_DATA_NUM,            // bSlaveInterface0: CDC data interface

    // CDC Notification Endpoint (Interrupt IN)
    // Purpose: Used for sending notifications (e.g. line state change)
    7,                                  // bLength: size of endpoint descriptor
    USB_DESCRIPTOR_TYPE_ENDPOINT,       // bDescriptorType: ENDPOINT (0x05)
    USB_DIR_DEVICE_OUT_HOST_IN | 0x01,  // bEndpointAddress: IN endpoint 1 (0x80 | 1)
    0x03,                               // bmAttributes: interrupt type
    0x08,                               // wMaxPacketSize (low byte): 8 bytes
    0x00,                               // wMaxPacketSize (high byte)
    1,                                  // bInterval: polling interval in ms

    // CDC Data Interface Descriptor
    // Purpose: Declares the data interface
    9,                              // bLength: size of interface descriptor
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType: INTERFACE
    INTERFACE_CDC_DATA_NUM,         // bInterfaceNumber: data interface number
    0,                              // bAlternateSetting
    2,                              // bNumEndpoints: two endpoints (IN + OUT)
    0x0A,                           // bInterfaceClass: Data interface class
    0x00,                           // bInterfaceSubClass: none
    0x00,                           // bInterfaceProtocol: none
    0,                              // iInterface: string index

    // Data OUT Endpoint (Bulk OUT)
    // Purpose: Used for host → device data
    7,                                  // bLength: endpoint descriptor size
    USB_DESCRIPTOR_TYPE_ENDPOINT,       // bDescriptorType: ENDPOINT
    USB_DIR_DEVICE_IN_HOST_OUT | 0x02,  // bEndpointAddress: OUT endpoint (0x00 | 2)
    0x02,                               // bmAttributes: bulk transfer
    USB_CDC_EP_BUFFER_SIZE,             // wMaxPacketSize low byte: 64 bytes
    0x00,                               // wMaxPacketSize high byte
    0,                                  // bInterval: ignored for bulk

    // Data IN Endpoint (Bulk IN)
    // Purpose: Used for device → host data
    7,                                  // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,       // bDescriptorType: ENDPOINT
    USB_DIR_DEVICE_OUT_HOST_IN | 0x02,  // bEndpointAddress: IN endpoint 2 (0x80 | 2)
    0x02,                               // bmAttributes: bulk
    USB_CDC_EP_BUFFER_SIZE,             // wMaxPacketSize low byte: 64 bytes
    0x00,                               // wMaxPacketSize high byte
    0,                                  // bInterval
};

// Device Qualifier Descriptor for a High-Speed capable device
__attribute__((aligned(4))) static const uint8_t device_qualifier_desc[] = {
    0x0A,                                  // bLength = 10 bytes
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,  // bDescriptorType = 0x06
    0x00, 0x02,                            // bcdUSB = USB 2.00
    0x02,                                  // bInterfaceClass: CDC (Communications Device Class)
    0x02,                                  // bInterfaceSubClass: ACM
    0x00,                                  // bInterfaceProtocol: No protocol
    USB_EP0_BUFFER_SIZE,                   // bMaxPacketSize0 = 64 bytes for EP0
    0x01,                                  // bNumConfigurations (how many at this speed)
    0x00                                   // bReserved = 0
};

const uint8_t* usb_descriptor_device_qualifier() {
  return (uint8_t*)device_qualifier_desc;
}

const usb_configuration_descriptor_t* usb_get_configuration_descriptor() {
  // The start of usb_get_configuration_descriptor is the description configuration for the device
  // So just cast and return it
  return (usb_configuration_descriptor_t*)usb_configuration_descriptor;
}

// -----------------------------------------------------------------------------
// USB String Descriptors
// -----------------------------------------------------------------------------
//
// String descriptor index mapping:
//   0 : Supported language IDs (special case, LANGID descriptor)
//   1 : Manufacturer
//   2 : Product
//   3 : Serial number
//   4 : CDC Interface description
//
// NOTE: Strings must be UTF-16LE encoded per USB spec. Here we generate them
//       dynamically from ASCII strings at runtime.
// -----------------------------------------------------------------------------

// String descriptor source table.
// Index 0 is special: LANGID (0x0409 = English - United States)
static const char* string_descriptor_arr[] = {
    (const char[]){0x09, 0x04},  // LANGID descriptor (en-US)
    "ST",                        // Manufacturer
    "PCB Mill",                  // Product name
    "9876543210",                // Serial number
    "PCB Mill",                  // CDC interface string
};

// Temporary buffer for building string descriptors.
// Size = 32 UTF-16 characters max.
static uint16_t descriptor_str[32];

/**
 * @brief Retrieve a USB string descriptor.
 *
 * @param index String descriptor index (0 = LANGID, others = from table)
 * @return Pointer to UTF-16LE string descriptor (length-prefixed), or NULL if invalid.
 *
 * The returned buffer is reused across calls, so it must be consumed immediately.
 */
const uint16_t* usb_descriptor_string(uint8_t index) {
  // Special case: index 0 = LANGID descriptor
  if (index == 0) {
    // Descriptor header: bLength (2), bDescriptorType (STRING = 0x03)
    descriptor_str[0] = (2 << 8) | USB_DESCRIPTOR_TYPE_STRING;
    // First (and only) LANGID: English (0x0409)
    descriptor_str[1] = 0x0409;
    return descriptor_str;
  }

  // Bounds check: must be within string descriptor array
  if (index >= (sizeof(string_descriptor_arr) / sizeof(string_descriptor_arr[0]))) {
    return NULL;
  }

  const char* str = string_descriptor_arr[index];
  size_t len = strlen(str);

  // Limit length to 31 characters (fits into descriptor_str[32], with header)
  if (len > 31) len = 31;

  // Descriptor header: bLength, bDescriptorType
  // bLength = 2 (header) + 2*len (UTF-16 chars)
  descriptor_str[0] = (USB_DESCRIPTOR_TYPE_STRING << 8) | (2 * len + 2);

  // Convert ASCII to UTF-16LE
  for (size_t i = 0; i < len; i++) {
    descriptor_str[1 + i] = (uint8_t)str[i];  // high byte = 0
  }

  return descriptor_str;
}
