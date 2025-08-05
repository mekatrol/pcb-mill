#include "tusb.h"

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,

    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

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
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x81, 8, 0x02, 0x82, 64),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

// String descriptors
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},  // 0: LANGID (English)
    "OpenAI Labs",               // 1: Manufacturer
    "TinyUSB CDC Device",        // 2: Product
    "12345678",                  // 3: Serial
    "TinyUSB CDC Interface",     // 4: CDC Interface
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
