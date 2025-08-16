#include "usb.h"

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

void tu_edpt_bind_driver(uint8_t ep2drv[][2], usb_control_interface_descriptor_t const* desc_itf, uint16_t desc_len) {
  uint8_t const* p_desc = (uint8_t const*)desc_itf;
  uint8_t const* desc_end = p_desc + desc_len;

  while (p_desc < desc_end) {
    if (USB_DESC_ENDPOINT == tu_desc_type(p_desc)) {
      uint8_t const ep_addr = ((usb_endpoint_descriptor_t const*)p_desc)->bEndpointAddress;
      ep2drv[USB_EP_NUM(ep_addr)][USB_EP_DIR_IDX(ep_addr)] = 0;
    }
    p_desc = tu_desc_next(p_desc);
  }
}
