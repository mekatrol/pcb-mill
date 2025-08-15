#include "dcd.h"
#include "tusb_private.h"
#include "usbd_pvt.h"

bool tu_edpt_claim(endpoint_state_t* ep_state) {
  // can only claim the endpoint if it is not busy and not claimed yet.
  const bool ep_available = (ep_state->busy == 0) && (ep_state->claimed == 0);

  if (ep_available) {
    ep_state->claimed = 1;
  }

  return ep_available;
}

bool tu_edpt_release(endpoint_state_t* ep_state) {
  const bool released = (ep_state->claimed == 1) && (ep_state->busy == 0);
  if (released) {
    ep_state->claimed = 0;
  }
  return released;
}

bool tu_edpt_validate(usb_endpoint_descriptor_t const* endpoint_descriptor, bool is_host) {
  // According to USB 2.0 Specification:
  //
  // Full-Speed Bulk Endpoint:
  //   Allowed max packet sizes: 8, 16, 32, or 64 bytes
  //   Reference: USB 2.0 Spec, Table 9-13 (p.262), Section 5.6.3 (Full-Speed endpoints)
  //
  // High-Speed Bulk Endpoint:
  //   Must have max packet size exactly 512 bytes
  //   Reference: USB 2.0 Spec, Table 9-13 (p.262), Section 5.8.3 (Bulk Transfers, p.120)
  const uint32_t max_packet_size = usb_endpoint_packet_size(endpoint_descriptor);

  switch (endpoint_descriptor->bmAttributes.type) {
    case USB_ENDPOINT_TYPE_BULK:
      // USB 2.0 Spec, §5.8.3 & Table 9-13:
      // - High-speed bulk endpoints: wMaxPacketSize MUST be exactly 512 bytes.
      //   No other sizes are permitted at high speed.
      // - Full-speed bulk endpoints: wMaxPacketSize MUST be one of {8, 16, 32, 64}.
      //   These are the only legal values; choice depends on endpoint/device design.
      if (max_packet_size != 8 &&
          max_packet_size != 16 &&
          max_packet_size != 32 &&
          max_packet_size != 64) {
        return false;
      }
      break;

    case USB_ENDPOINT_TYPE_INTERRUPT:
      // USB 2.0 Spec, §5.7.3 & Table 9-13:
      // - Full-speed interrupt endpoints: wMaxPacketSize range is 1–64 bytes.
      //   This code enforces only the upper bound (64).
      // - High-speed interrupt endpoints: can be up to 1024 bytes (not handled here).
      if (max_packet_size > 64) {
        return false;
      }
      break;

    default:
      // Unsupported or invalid endpoint type
      return false;
  }

  return true;
}

void tu_edpt_bind_driver(uint8_t ep2drv[][2], usb_control_interface_descriptor_t const* desc_itf, uint16_t desc_len) {
  uint8_t const* p_desc = (uint8_t const*)desc_itf;
  uint8_t const* desc_end = p_desc + desc_len;

  while (p_desc < desc_end) {
    if (USB_DESC_ENDPOINT == tu_desc_type(p_desc)) {
      uint8_t const ep_addr = ((usb_endpoint_descriptor_t const*)p_desc)->bEndpointAddress;
      ep2drv[usb_endpoint_number(ep_addr)][usb_endpoint_direction(ep_addr)] = 0;
    }
    p_desc = tu_desc_next(p_desc);
  }
}

uint16_t tu_desc_get_interface_total_len(usb_control_interface_descriptor_t const* desc_itf, uint8_t itf_count, uint16_t max_len) {
  uint8_t const* p_desc = (uint8_t const*)desc_itf;
  uint16_t len = 0;

  while (itf_count--) {
    // Next on interface desc
    len += tu_desc_len(desc_itf);
    p_desc = tu_desc_next(p_desc);

    while (len < max_len) {
      if (tu_desc_len(p_desc) == 0) {
        // Escape infinite loop
        break;
      }
      // return on IAD regardless of itf count
      if (tu_desc_type(p_desc) == USB_DESC_INTERFACE_ASSOCIATION) {
        return len;
      }
      if ((tu_desc_type(p_desc) == USB_DESC_INTERFACE) &&
          ((usb_control_interface_descriptor_t const*)p_desc)->bAlternateSetting == 0) {
        break;
      }

      len += tu_desc_len(p_desc);
      p_desc = tu_desc_next(p_desc);
    }
  }

  return len;
}
