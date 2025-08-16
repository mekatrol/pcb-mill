#ifndef TUSB_CDC_DEVICE_H_
#define TUSB_CDC_DEVICE_H_

#include "usb.h"

// Get current line encoding: bit rate, stop bits parity etc ..
void usb_cdc_get_line_coding(usb_cdc_line_coding_t* coding);

// Get the number of bytes available for reading
uint32_t usb_cdc_available();

// Read received bytes
uint32_t usb_cdc_read(void* buffer, uint32_t bufsize);

// Read a byte, return -1 if there is none
__attribute__((always_inline)) static inline int32_t usb_cdc_read_char() {
  uint8_t ch;
  return usb_cdc_read(&ch, 1) ? (int32_t)ch : -1;
}

// Write bytes to TX buffer, data may remain in the buffer for a while
uint32_t usb_cdc_write(const uint8_t* buffer, uint32_t bufsize);

// Write a byte
__attribute__((always_inline)) static inline uint32_t usb_cdc_write_char(uint8_t ch) {
  return usb_cdc_write(&ch, 1);
}

// Write a null-terminated string
__attribute__((always_inline)) static inline uint32_t usb_cdc_write_str(uint8_t const* str) {
  return usb_cdc_write(str, strlen((char*)str));
}

// Force sending data if possible, return number of forced bytes
uint32_t usb_cdc_write_flush();

//--------------------------------------------------------------------+
// Application Callback API
//--------------------------------------------------------------------+

// Invoked when received new data
__attribute__((weak)) void tud_cdc_rx_cb();

// Invoked when a TX is complete and therefore space becomes available in TX buffer
__attribute__((weak)) void tud_cdc_tx_complete_cb();

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
__attribute__((weak)) void tud_cdc_line_state_cb(bool dtr, bool rts);

// Invoked when line coding is change via SET_LINE_CODING
__attribute__((weak)) void tud_cdc_line_coding_cb(usb_cdc_line_coding_t const* p_line_coding);

// Invoked when received send break
// \param[in]  itf  interface for which send break was received.
// \param[in]  duration_ms  the length of time, in milliseconds, of the break signal. If a value of FFFFh, then the
//                          device will send a break until another SendBreak request is received with value 0000h.
__attribute__((weak)) void tud_cdc_send_break_cb(uint16_t duration_ms);

//--------------------------------------------------------------------+
// INTERNAL USBD-CLASS DRIVER API
//--------------------------------------------------------------------+
void usb_cdc_init();
void usb_cdc_reset();
uint16_t usb_cdc_open(usb_control_interface_descriptor_t const* itf_desc, uint16_t max_len);
bool usb_cdc_control_xfer_cb(uint8_t stage, usb_control_request_t const* request);
bool usb_cdc_transfer_cb(uint8_t ep_addr, uint32_t transferred_bytes);

#endif /* _TUSB_CDC_DEVICE_H_ */
