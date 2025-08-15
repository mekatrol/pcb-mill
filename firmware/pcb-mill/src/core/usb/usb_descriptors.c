
#include <stdint.h>

#include "usbd.h"
#include "cdc_device.h"

// -----------------------------------------------------------------------------
// USB Device Descriptor (See USB 2.0 Spec, Section 9.6.1)
// This descriptor tells the host the overall characteristics of the USB device
// before it requests configuration/interface/endpoint descriptors.
// -----------------------------------------------------------------------------
usb_device_desc_t const desc_device = {
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

// Configuration descriptor
enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Configuration Descriptor
    9, 0x02, 0x4B, 0x00, ITF_NUM_TOTAL, 1, 0, 0x80, 50,

    // CDC Interface Association
    8, 0x0B, ITF_NUM_CDC, 2, 0x02, 0x02, 0x00, 0,

    // CDC Control Interface
    9, 0x04, ITF_NUM_CDC, 0, 1, 0x02, 0x02, 0x00, 4,

    // CDC Header
    5, 0x24, 0x00, 0x20, 0x01,

    // CDC Call Management
    5, 0x24, 0x01, 0, (ITF_NUM_CDC + 1),

    // CDC ACM
    4, 0x24, 0x02, 6,

    // CDC Union
    5, 0x24, 0x06, ITF_NUM_CDC, (ITF_NUM_CDC + 1),

    // CDC Notification Endpoint
    7, 0x05, 0x81, 0x03, 0x08, 0x00, 1,

    // CDC Data Interface
    9, 0x04, (ITF_NUM_CDC + 1), 0, 2, 0x0A, 0x00, 0x00, 0,

    // Data OUT Endpoint
    7, 0x05, 0x02, 0x02, 0x40, 0x00, 0,

    // Data IN Endpoint
    7, 0x05, 0x82, 0x02, 0x40, 0x00, 0};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
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
