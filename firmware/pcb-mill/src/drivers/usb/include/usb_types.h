#ifndef __USB_TYPES_H__
#define __USB_TYPES_H__

#include <stdint.h>

// USB Descriptor base (common to all descriptors)
typedef struct __attribute__((packed)) {
  uint8_t bLength;          // Size of this descriptor in bytes
  uint8_t bDescriptorType;  // Descriptor type
} usb_descriptor_base_t;

#endif  // __USB_TYPES_H__