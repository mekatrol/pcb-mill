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
#define USB_ENDPOINT_MAX 3

// The size of endpoint 0 buffer
#define USB_EP0_BUFFER_SIZE 64

// The size of other endpoint buffers (e.g. CDC, MSC)
#define USB_ENDPOINT_RX_BUFFER_SIZE 64
#define USB_ENDPOINT_TX_BUFFER_SIZE 64

// The maximum number of USB interfaces across all configurations
// Each USB interface is as defined in the USB 2.0 spec (see Section 9.6.5 Interface Descriptor).
// An interface is a logical grouping of endpoints and functions.
// Two interfaces supported:
//  * CDC (Communications Device Class ) ACM (Abstract Control Model) [Virtual COM port]
//  * MSC (Mass Storage Class)  [SD card]
#define USB_MAX_INTERFACES 2

// Size of array based on total memory size dived by size of single element
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// Ceiling division
//  DIV_CEIL(5, 2) => ((5 + 2 - 1) / 2) => (6 / 2) => 3
//  DIV_CEIL(9, 4) => ((9 + 4 - 1) / 4) => (12 / 4) => 3
//  DIV_CEIL(8, 4) => ((8 + 4 - 1) / 4) => (11 / 4) => 2
#define DIV_CEIL(n, d) (((n) + (d) - 1) / (d))

// Bit mask based on bit position
#define BIT_MASK(n) (1UL << (n))

#define ENDPOINT_TX_BUFFER 0
#define ENDPOINT_RX_BUFFER 1

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
} usbram_register_t;

// Buffer Table is located in Packet Memory Area (PMA)
typedef struct {
  usbram_register_t endpoint[USB_ENDPOINT_MAX];
} usbram_register_map_t;

#define USBRAM_REGSITER ((volatile usbram_register_map_t *)(USB_DRD_PMAADDR))

/// USB Device Descriptor
typedef struct __attribute__((packed)) {
  uint8_t bLength;             // Descriptor size in bytes
  uint8_t bDescriptorType;     // Descriptor type (DEVICE)
  uint16_t bcdUSB;             // USB specification version in BCD
  uint8_t bDeviceClass;        // Device class code
  uint8_t bDeviceSubClass;     // Device subclass code
  uint8_t bDeviceProtocol;     // Device protocol code
  uint8_t bMaxPacketSize0;     // Max packet size for endpoint zero
  uint16_t idVendor;           // Vendor ID (USB-IF assigned)
  uint16_t idProduct;          // Product ID (manufacturer assigned)
  uint16_t bcdDevice;          // Device release number in BCD
  uint8_t iManufacturer;       // Index of manufacturer string descriptor
  uint8_t iProduct;            // Index of product string descriptor
  uint8_t iSerialNumber;       // Index of serial number string descriptor
  uint8_t bNumConfigurations;  // Number of configurations supported
} usb_device_desc_t;

void usbd_control_reset();
void usb_configuration_reset();
void handle_bus_reset();
void handle_ctr_rx(uint32_t endpoint_id);
void handle_ctr_tx(uint32_t endpoint_id);
void handle_ctr_setup(uint32_t endpoint_id);

__attribute__((always_inline)) static inline bool bit_set_test(uint32_t value, uint32_t pos) { return (value & BIT_MASK(pos)) ? true : false; }
__attribute__((always_inline)) static inline uint16_t min_u16(uint16_t x, uint16_t y) { return (x < y) ? x : y; }

__attribute__((always_inline)) static inline void usb_reset() {
  handle_bus_reset();
  usb_configuration_reset();
  usbd_control_reset();
}

// Get high or low byte
#define U16_HIGH(_u16) ((uint8_t)(((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16) ((uint8_t)((_u16) & 0x00ff))

typedef struct {
  uint16_t val;
} __attribute__((packed)) tu_unaligned_uint16_t;

typedef struct {
  uint32_t val;
} __attribute__((packed)) tu_unaligned_uint32_t;

__attribute__((always_inline)) static inline uint32_t tu_unaligned_read32(const void *mem) {
  tu_unaligned_uint32_t const *ua32 = (tu_unaligned_uint32_t const *)mem;
  return ua32->val;
}

__attribute__((always_inline)) static inline void tu_unaligned_write32(void *mem, uint32_t value) {
  tu_unaligned_uint32_t *ua32 = (tu_unaligned_uint32_t *)mem;
  ua32->val = value;
}

__attribute__((always_inline)) static inline uint16_t tu_unaligned_read16(const void *mem) {
  tu_unaligned_uint16_t const *ua16 = (tu_unaligned_uint16_t const *)mem;
  return ua16->val;
}

bool usb_init_driver();

#endif  // __USB_H__