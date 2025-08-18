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

// Direction index
typedef enum {
  USB_DIR_DEVICE_IN_HOST_OUT_IDX = 0,  // Host-to-device
  USB_DIR_DEVICE_OUT_HOST_IN_IDX = 1,  // Device-to-host
} usb_request_direction_index_t;

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

typedef enum {
  // Direction (bit 7)
  USB_REQUEST_DIRECTION_HOST_TO_DEVICE = 0 << 7,
  USB_REQUEST_DIRECTION_DEVICE_TO_HOST = 1 << 7,
  USB_REQUEST_DIRECTION_MASK = 0x80  // Bit 7
} usb_direction_t;

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

/*
 * USB HAL API methods - these must be implmented by a HAL (typically the board code)
 */
void usb_device_start_hal();                                         // Start USB in device mode
bool process_control_request(const usb_control_request_t* request);  // Process a control request

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

#endif  // __USB_H__