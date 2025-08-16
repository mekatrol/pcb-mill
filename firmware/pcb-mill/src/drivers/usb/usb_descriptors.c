
#include <stdint.h>

#include "usb.h"
#include "cdc_device.h"

// -----------------------------------------------------------------------------
// USB Device Descriptor (See USB 2.0 Spec, Section 9.6.1)
// This descriptor tells the host the overall characteristics of the USB device
// before it requests configuration/interface/endpoint descriptors.
// -----------------------------------------------------------------------------
static const usb_device_desc_t desc_device = {
    .bLength = sizeof(usb_device_desc_t),  // Size of this descriptor in bytes (should be 18 for a device descriptor)
    .bDescriptorType = USB_DESC_DEVICE,    // Descriptor Type: DEVICE (0x01)

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

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

// Configuration descriptor defined interfaces
enum {
  // CDC interface number
  INTERFACE_CDC_NUM = 0,

  // CDC data interface number
  INTERFACE_CDC_DATA_NUM = 1,

  // CDC total interface count
  INTERFACE_CDC_INTERFACE_COUNT = 2,

  // Total device interface count
  INTERFACE_TOTAL_COUNT = INTERFACE_CDC_INTERFACE_COUNT
};

uint8_t const usb_desc_configuration[] = {
    // Configuration Descriptor (usb_configuration_descriptor_t)
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
    8,                                // bLength: size of this descriptor in bytes (8)
    USB_DESCRIPTOR_TYPE_ASSOCIATION,  // bDescriptorType: INTERFACE_ASSOCIATION (0x0B)
    INTERFACE_CDC_NUM,                // bFirstInterface: first interface associated with this function
    INTERFACE_CDC_INTERFACE_COUNT,    // bInterfaceCount: number of interfaces associated (CDC control + data)
    0x02,                             // bFunctionClass: CDC class code
    0x02,                             // bFunctionSubClass: Abstract Control Model (ACM)
    0x00,                             // bFunctionProtocol: No protocol
    0,                                // iFunction: index of string descriptor for this function (0 = none)

    // CDC Control Interface Descriptor (usb_control_interface_descriptor_t)
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
    4,                                 // bFunctionLength: size of descriptor
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE
    0x02,                              // bDescriptorSubtype: Abstract Control Management (0x02)
    6,                                 // bmCapabilities: supports Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding

    // CDC Union Functional Descriptor
    5,                                 // bFunctionLength: size of descriptor
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,  // bDescriptorType: CS_INTERFACE
    0x06,                              // bDescriptorSubtype: Union functional descriptor (0x06)
    INTERFACE_CDC_NUM,                 // bMasterInterface: CDC control interface
    INTERFACE_CDC_DATA_NUM,            // bSlaveInterface0: CDC data interface

    // CDC Notification Endpoint (Interrupt IN)
    7,                             // bLength: size of endpoint descriptor
    USB_DESCRIPTOR_TYPE_ENDPOINT,  // bDescriptorType: ENDPOINT (0x05)
    USB_DIR_IN | 0x01,             // bEndpointAddress: IN endpoint 1 (0x80 | 1)
    0x03,                          // bmAttributes: interrupt type
    0x08,                          // wMaxPacketSize (low byte): 8 bytes
    0x00,                          // wMaxPacketSize (high byte)
    1,                             // bInterval: polling interval in ms

    // CDC Data Interface Descriptor
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
    7,                             // bLength: endpoint descriptor size
    USB_DESCRIPTOR_TYPE_ENDPOINT,  // bDescriptorType: ENDPOINT
    USB_DIR_OUT | 0x02,            // bEndpointAddress: OUT endpoint (0x00 | 2)
    0x02,                          // bmAttributes: bulk transfer
    0x40,                          // wMaxPacketSize low byte: 64 bytes
    0x00,                          // wMaxPacketSize high byte
    0,                             // bInterval: ignored for bulk

    // Data IN Endpoint (Bulk IN)
    7,                             // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,  // bDescriptorType: ENDPOINT
    USB_DIR_IN | 0x02,             // bEndpointAddress: IN endpoint 2 (0x80 | 2)
    0x02,                          // bmAttributes: bulk
    0x40,                          // wMaxPacketSize low byte: 64 bytes
    0x00,                          // wMaxPacketSize high byte
    0,                             // bInterval
};

const usb_configuration_descriptor_t* usb_descriptor_configuration() {
  // The start of usb_desc_configuration is the description configuration for the device
  // So just cast and return it
  return (usb_configuration_descriptor_t*)usb_desc_configuration;
}

// String descriptors
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},  // LANGID (English)
    "ST",                        // Manufacturer
    "PCB Mill",                  // Product
    "9876543210",                // Serial
    "PCB Mill",                  // CDC Interface
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  if (index == 0) {
    _desc_str[0] = (2 << 8) | USB_DESC_STRING;
    _desc_str[1] = 0x0409;
    return _desc_str;
  }

  if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

  const char* str = string_desc_arr[index];
  size_t len = strlen(str);

  if (len > 31) len = 31;
  _desc_str[0] = (USB_DESC_STRING << 8) | (2 * len + 2);
  for (size_t i = 0; i < len; ++i) {
    _desc_str[1 + i] = str[i];
  }

  return _desc_str;
}
