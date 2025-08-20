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
ALWAYS_INLINE static int32_t usb_cdc_read_char() {
  uint8_t ch;
  return usb_cdc_read(&ch, 1) ? (int32_t)ch : -1;
}

// Write bytes to TX buffer, data may remain in the buffer for a while
uint32_t usb_cdc_write(const uint8_t* buffer, uint32_t bufsize);

// Write a byte
ALWAYS_INLINE static uint32_t usb_cdc_write_char(uint8_t ch) {
  return usb_cdc_write(&ch, 1);
}

// Write a null-terminated string
ALWAYS_INLINE static uint32_t usb_cdc_write_str(const uint8_t* str) {
  return usb_cdc_write(str, strlen((char*)str));
}

// Force sending data if possible, return number of forced bytes
uint32_t usb_cdc_write_flush();

//--------------------------------------------------------------------+
// Application Callback API
//--------------------------------------------------------------------+

// Invoked when received new data
__attribute__((weak)) void usb_cdc_rx_cb();

// Invoked when a TX is complete and therefore space becomes available in TX buffer
__attribute__((weak)) void usb_cdc_tx_complete_cb();

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
__attribute__((weak)) void usb_cdc_handshake_cb(bool dtr, bool rts);

// Invoked when line coding is change via SET_LINE_CODING
__attribute__((weak)) void usb_cdc_line_coding_cb(const usb_cdc_line_coding_t* p_line_coding);

//--------------------------------------------------------------------+
// INTERNAL USBD-CLASS DRIVER API
//--------------------------------------------------------------------+
void usb_cdc_init();
void usb_cdc_reset();
uint16_t usb_cdc_open(const usb_control_interface_descriptor_t* itf_desc, uint16_t max_len);
bool usb_device_control_transfer(uint8_t stage, const usb_control_request_t* request);
bool usb_cdc_transfer(uint8_t ep_addr, uint32_t transferred_bytes);

#endif /* _TUSB_CDC_DEVICE_H_ */
