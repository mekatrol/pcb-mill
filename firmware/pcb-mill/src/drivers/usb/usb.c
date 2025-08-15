#include "usb.h"

typedef enum {
  USB_BMATTR_RESERVED_D7 = 0x80,      ///< Bit 7: Reserved, must always be set to 1
  USB_BMATTR_SELF_POWERED = 0x40,     ///< Bit 6: 1 = Device is self-powered, 0 = bus-powered
  USB_BMATTR_REMOTE_WAKEUP = 0x20,    ///< Bit 5: 1 = Device can wake host from suspend
  USB_BMATTR_RESERVED_D4_TO_0 = 0x1F  ///< Bits 4..0: Reserved, must be 0
} usb_bm_attributes_mask_t;

typedef enum {
  USB_CONFIG_REMOTE_WAKEUP_MASK = 1U << 5,
  USB_CONFIG_SELF_POWERED_MASK = 1U << 6,
} usb_configuration_flags;

typedef struct {
  volatile usb_configuration_flags state_flags;  // Flags indicating the state of the device
  volatile uint8_t interface_count;              // The total number of interfaces the device has
} usb_device_t;

// We are a single USB device
// volatile usb_device_t usb_device;

// Will return next interfac descriptor
__attribute__((always_inline)) static inline const usb_interface_association_descriptor_t* next_interface(const usb_interface_association_descriptor_t* interface_assoc) {
  return (const usb_interface_association_descriptor_t*)(interface_assoc + interface_assoc->bLength);
}

// bool usb_set_configuration() {
//   // Get description configuration
//   usb_configuration_descriptor_t const* descriptor_config = (usb_configuration_descriptor_t const*)usb_descriptor_configuration();

//   // Validate type
//   if (descriptor_config->bDescriptorType != USB_DESCRIPTOR_TYPE_CONFIGURATION) {
//     return false;
//   }

//   // Set state flags
//   usb_device.state_flags |= (descriptor_config->bmAttributes & USB_BMATTR_REMOTE_WAKEUP) ? USB_CONFIG_REMOTE_WAKEUP_MASK : 0U;
//   usb_device.state_flags |= (descriptor_config->bmAttributes & USB_BMATTR_SELF_POWERED) ? USB_CONFIG_SELF_POWERED_MASK : 0U;

//   // Set total number of interfaces for USB device
//   usb_device.interface_count = descriptor_config->bNumInterfaces;

//   // First memory location after end of descriptors, allows termination
//   // once all descriptors have been enumerated
//   const uint8_t* descriptors_end = ((uint8_t const*)descriptor_config) + descriptor_config->wTotalLength;

//   // Walk through interface descriptors (starting at Interface Association Descriptor)
//   const usb_interface_association_descriptor_t* interface_association_descriptor = ((const usb_interface_association_descriptor_t*)descriptor_config) + sizeof(usb_configuration_descriptor_t);

//   // Validate type
//   if (descriptor_config->bDescriptorType != USB_DESCRIPTOR_TYPE_ASSOCIATION) {
//     return false;
//   }

//   // Move to next interface
//   interface_association_descriptor = (const usb_interface_association_descriptor_t*)(interface_association_descriptor + interface_association_descriptor->bLength);

//   if (interface_association_descriptor->bDescriptorType != USB_DESCRIPTOR_TYPE_INTERFACE) {
//     return false;
//   }

//   // Enumerate remaining descriptors in configuration
//   while (interface_association_descriptor < descriptors_end) {
//     const usb_control_interface_descriptor_t* control_interface_descriptor = (usb_control_interface_descriptor_t const*)p_desc;

//     // Find driver for this interface
//     uint16_t const remaining_len = (uint16_t)(descriptors_end - interface_association_descriptor);
//     uint16_t const drv_len = usb_cdc_open(control_interface_descriptor, remaining_len);

//     if ((sizeof(usb_control_interface_descriptor_t) <= drv_len) && (drv_len <= remaining_len)) {
//       if (interface_count == 1) {
//         interface_count = 2;
//       }

//       // bind (associated) interfaces to found driver
//       for (uint8_t i = 0; i < interface_count; i++) {
//         uint8_t const interface_num = control_interface_descriptor->bInterfaceNumber + i;

//         // Interface number must not be used already
//         if (usb_device.itf2drv[interface_num] != 0xFF) {
//           return false;
//         }
//         usb_device.itf2drv[interface_num] = 0;
//       }

//       // bind all endpoints to found driver
//       usb_endpoint_bind_driver(usb_device.ep2drv, control_interface_descriptor, drv_len);

//       // next Interface
//       interface_association_descriptor += drv_len;

//       break;  // exit driver find loop
//     }
//   }

//   return true;
// }