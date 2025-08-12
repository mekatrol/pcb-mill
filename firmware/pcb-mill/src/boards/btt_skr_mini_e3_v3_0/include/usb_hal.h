#ifndef __USB_HAL_H__
#define __USB_HAL_H__

#include "board_hal.h"
#include "usb.h"

__attribute__((always_inline)) static inline void usb_endpoint_status(uint32_t* endpoint_reg, usb_endpoint_direction_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == USB_ENDPOINT_DIRECTION_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline void usb_endpoint_data_toggle(uint32_t* endpoint_reg, usb_endpoint_direction_t dir, usb_endpoint_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in endpoint_reg
  *endpoint_reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (dir == USB_ENDPOINT_DIRECTION_IN ? 0 : 8)));
}

#endif