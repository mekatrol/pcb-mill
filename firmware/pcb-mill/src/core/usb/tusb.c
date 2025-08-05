#include "dcd.h"
#include "tusb_option.h"
#include "tusb.h"
#include "tusb_private.h"
#include "usbd_pvt.h"

tusb_role_t _tusb_rhport_role[TUP_USBIP_CONTROLLER_NUM] = {TUSB_ROLE_INVALID};

//--------------------------------------------------------------------+
// Public API
//--------------------------------------------------------------------+
bool tusb_rhport_init(uint8_t rhport, const tusb_rhport_init_t* rh_init) {
  //  backward compatible called with tusb_init(void)
  if (rh_init == NULL) {
    // init device stack CFG_TUSB_RHPORTx_MODE must be defined
    const tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL};
    TU_ASSERT(tud_rhport_init(TUD_OPT_RHPORT, &dev_init));
    _tusb_rhport_role[TUD_OPT_RHPORT] = TUSB_ROLE_DEVICE;

    return true;
  }

  // new API with explicit rhport and role
  TU_ASSERT(rhport < TUP_USBIP_CONTROLLER_NUM && rh_init->role != TUSB_ROLE_INVALID);
  _tusb_rhport_role[rhport] = rh_init->role;

  if (rh_init->role == TUSB_ROLE_DEVICE) {
    TU_ASSERT(tud_rhport_init(rhport, rh_init));
  }

  return true;
}

bool tusb_inited(void) {
  bool ret = false;

  ret = ret || tud_inited();

  return ret;
}

void tusb_int_handler(uint8_t rhport, bool in_isr) {
  TU_VERIFY(rhport < TUP_USBIP_CONTROLLER_NUM, );

  if (_tusb_rhport_role[rhport] == TUSB_ROLE_DEVICE) {
    (void)in_isr;
    dcd_int_handler(rhport);
  }
}

bool tusb_deinit(uint8_t rhport) {
  TU_VERIFY(rhport < TUP_USBIP_CONTROLLER_NUM);
  bool ret = false;

  if (_tusb_rhport_role[rhport] == TUSB_ROLE_DEVICE) {
    TU_ASSERT(tud_deinit(rhport));
    _tusb_rhport_role[rhport] = TUSB_ROLE_INVALID;
    ret = true;
  }

  return ret;
}

//--------------------------------------------------------------------+
// Descriptor helper
//--------------------------------------------------------------------+

uint8_t const* tu_desc_find(uint8_t const* desc, uint8_t const* end, uint8_t byte1) {
  while (desc + 1 < end) {
    if (desc[1] == byte1) {
      return desc;
    }
    desc += desc[DESC_OFFSET_LEN];
  }
  return NULL;
}

uint8_t const* tu_desc_find2(uint8_t const* desc, uint8_t const* end, uint8_t byte1, uint8_t byte2) {
  while (desc + 2 < end) {
    if (desc[1] == byte1 && desc[2] == byte2) {
      return desc;
    }
    desc += desc[DESC_OFFSET_LEN];
  }
  return NULL;
}

uint8_t const* tu_desc_find3(uint8_t const* desc, uint8_t const* end, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  while (desc + 3 < end) {
    if (desc[1] == byte1 && desc[2] == byte2 && desc[3] == byte3) {
      return desc;
    }
    desc += desc[DESC_OFFSET_LEN];
  }
  return NULL;
}

//--------------------------------------------------------------------+
// Endpoint Helper for both Host and Device stack
//--------------------------------------------------------------------+

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

bool tu_edpt_validate(tusb_desc_endpoint_t const* desc_ep, tusb_speed_t speed, bool is_host) {
  uint16_t const max_packet_size = tu_edpt_packet_size(desc_ep);

  switch (desc_ep->bmAttributes.xfer) {
    case TUSB_XFER_ISOCHRONOUS: {
      uint16_t const spec_size = (speed == TUSB_SPEED_HIGH ? 1024 : 1023);
      TU_ASSERT(max_packet_size <= spec_size);
      break;
    }

    case TUSB_XFER_BULK:
      if (speed == TUSB_SPEED_HIGH) {
        // Bulk highspeed must be EXACTLY 512
        TU_ASSERT(max_packet_size == 512);
      } else {
        // Bulk fullspeed can only be 8, 16, 32, 64
        if (is_host && max_packet_size == 512) {
          // HACK: while in host mode, some device incorrectly always report 512 regardless of link speed
          // overwrite descriptor to force 64
          tusb_desc_endpoint_t* hacked_ep = (tusb_desc_endpoint_t*)(uintptr_t)desc_ep;
          hacked_ep->wMaxPacketSize = tu_htole16(64);
        } else {
          TU_ASSERT(max_packet_size == 8 || max_packet_size == 16 ||
                    max_packet_size == 32 || max_packet_size == 64);
        }
      }
      break;

    case TUSB_XFER_INTERRUPT: {
      uint16_t const spec_size = (speed == TUSB_SPEED_HIGH ? 1024 : 64);
      TU_ASSERT(max_packet_size <= spec_size);
      break;
    }

    default:
      return false;
  }

  return true;
}

void tu_edpt_bind_driver(uint8_t ep2drv[][2], tusb_desc_interface_t const* desc_itf, uint16_t desc_len,
                         uint8_t driver_id) {
  uint8_t const* p_desc = (uint8_t const*)desc_itf;
  uint8_t const* desc_end = p_desc + desc_len;

  while (p_desc < desc_end) {
    if (TUSB_DESC_ENDPOINT == tu_desc_type(p_desc)) {
      uint8_t const ep_addr = ((tusb_desc_endpoint_t const*)p_desc)->bEndpointAddress;
      ep2drv[tu_edpt_number(ep_addr)][tu_edpt_dir(ep_addr)] = driver_id;
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
