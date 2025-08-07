
#include <stdint.h>

#include "usbd.h"
#include "cdc_device.h"

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,

    .bDeviceClass = 0xEF,                    // Misc USB class
    .bDeviceSubClass = 2,                    // Common USB subclass
    .bDeviceProtocol = 1,                    // Priotocol IAD
    .bMaxPacketSize0 = USB_EP0_BUFFER_SIZE,  // Endpoint buffer size

    .idVendor = 0x0483,
    .idProduct = 0x5740,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 1};

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
    _desc_str[0] = (2 << 8) | TUSB_DESC_STRING;
    _desc_str[1] = 0x0409;
    return _desc_str;
  }

  if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

  const char* str = string_desc_arr[index];
  size_t len = strlen(str);

  if (len > 31) len = 31;
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
  for (size_t i = 0; i < len; ++i) {
    _desc_str[1 + i] = str[i];
  }

  return _desc_str;
}
