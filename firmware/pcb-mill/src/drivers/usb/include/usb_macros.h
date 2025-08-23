#ifndef __USB_MACROS_H__
#define __USB_MACROS_H__

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

#endif  // __USB_MACROS_H__