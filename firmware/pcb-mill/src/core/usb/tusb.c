#include "dcd.h"
#include "tusb_private.h"
#include "usbd_pvt.h"

bool tu_edpt_claim(tu_edpt_state_t* ep_state) {
  // can only claim the endpoint if it is not busy and not claimed yet.
  bool const available = (ep_state->busy == 0) && (ep_state->claimed == 0);
  if (available) {
    ep_state->claimed = 1;
  }
  return available;
}

bool tu_edpt_release(tu_edpt_state_t* ep_state) {
  // can only release the endpoint if it is claimed and not busy
  bool const ret = (ep_state->claimed == 1) && (ep_state->busy == 0);
  if (ret) {
    ep_state->claimed = 0;
  }
  return ret;
}

bool tu_edpt_validate(tusb_desc_endpoint_t const* desc_ep, bool is_host) {
  // According to USB 2.0 Specification:
  //
  // Full-Speed Bulk Endpoint:
  //   Allowed max packet sizes: 8, 16, 32, or 64 bytes
  //   Reference: USB 2.0 Spec, Table 9-13 (p.262), Section 5.6.3 (Full-Speed endpoints)
  //
  // High-Speed Bulk Endpoint:
  //   Must have max packet size exactly 512 bytes
  //   Reference: USB 2.0 Spec, Table 9-13 (p.262), Section 5.8.3 (Bulk Transfers, p.120)
  uint16_t const max_packet_size = tu_edpt_packet_size(desc_ep);

  switch (desc_ep->bmAttributes.xfer) {
    case TUSB_XFER_BULK:
      // USB 2.0 Spec, Section 5.8.3, Table 9-13
      // High-speed bulk packet size must be exactly 512 bytes.
      // This is a hard requirement in the spec — you cannot pick smaller or larger values at high speed.
      // Full-speed bulk can only be 8, 16, 32, or 64 bytes.
      // These four values are the only legal options; the choice depends on the endpoint and device capability.
      if (is_host && max_packet_size == 512) {
        // HACK: while in host mode, some device incorrectly always report 512 regardless of link speed
        // overwrite descriptor to force 64
        tusb_desc_endpoint_t* hacked_ep = (tusb_desc_endpoint_t*)(uintptr_t)desc_ep;
        hacked_ep->wMaxPacketSize = 64;
      } else {
        if (max_packet_size != 8 && max_packet_size != 16 &&
            max_packet_size != 32 && max_packet_size != 64) {
          return false;
        }
      }
      break;

    case TUSB_XFER_INTERRUPT: {
      if (max_packet_size > 64) {
        return false;
      }
      break;
    }

    default:
      return false;
  }

  return true;
}

void tu_edpt_bind_driver(uint8_t ep2drv[][2], tusb_desc_interface_t const* desc_itf, uint16_t desc_len) {
  uint8_t const* p_desc = (uint8_t const*)desc_itf;
  uint8_t const* desc_end = p_desc + desc_len;

  while (p_desc < desc_end) {
    if (TUSB_DESC_ENDPOINT == tu_desc_type(p_desc)) {
      uint8_t const ep_addr = ((tusb_desc_endpoint_t const*)p_desc)->bEndpointAddress;
      ep2drv[tu_edpt_number(ep_addr)][usb_endpoint_direction(ep_addr)] = 0;
    }
    p_desc = tu_desc_next(p_desc);
  }
}

uint16_t tu_desc_get_interface_total_len(tusb_desc_interface_t const* desc_itf, uint8_t itf_count, uint16_t max_len) {
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
      if (tu_desc_type(p_desc) == TUSB_DESC_INTERFACE_ASSOCIATION) {
        return len;
      }
      if ((tu_desc_type(p_desc) == TUSB_DESC_INTERFACE) &&
          ((tusb_desc_interface_t const*)p_desc)->bAlternateSetting == 0) {
        break;
      }

      len += tu_desc_len(p_desc);
      p_desc = tu_desc_next(p_desc);
    }
  }

  return len;
}
