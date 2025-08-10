#ifndef TUSB_CDC_DEVICE_H_
#define TUSB_CDC_DEVICE_H_

#include "cdc.h"

// Check if interface is ready
bool tud_cdc_n_ready();

// Check if terminal is connected to this port
bool tud_cdc_n_connected();

// Get current line state. Bit 0:  DTR (Data Terminal Ready), Bit 1: RTS (Request to Send)
uint8_t tud_cdc_n_get_line_state();

// Get current line encoding: bit rate, stop bits parity etc ..
void tud_cdc_n_get_line_coding(cdc_line_coding_t* coding);

// Set special character that will trigger tud_cdc_rx_wanted_cb() callback on receiving
void tud_cdc_n_set_wanted_char(char wanted);

// Get the number of bytes available for reading
uint32_t tud_cdc_n_available();

// Read received bytes
uint32_t tud_cdc_n_read(void* buffer, uint32_t bufsize);

// Read a byte, return -1 if there is none
__attribute__((always_inline)) static inline int32_t tud_cdc_n_read_char() {
  uint8_t ch;
  return tud_cdc_n_read(&ch, 1) ? (int32_t)ch : -1;
}

// Write bytes to TX FIFO, data may remain in the FIFO for a while
uint32_t tud_cdc_n_write(const uint8_t* buffer, uint32_t bufsize);

// Write a byte
__attribute__((always_inline)) static inline uint32_t tud_cdc_n_write_char(uint8_t ch) {
  return tud_cdc_n_write(&ch, 1);
}

// Write a null-terminated string
__attribute__((always_inline)) static inline uint32_t tud_cdc_n_write_str(uint8_t const* str) {
  return tud_cdc_n_write(str, strlen((char*)str));
}

// Force sending data if possible, return number of forced bytes
uint32_t tud_cdc_n_write_flush();

//--------------------------------------------------------------------+
// Application Callback API
//--------------------------------------------------------------------+

// Invoked when received new data
__attribute__((weak)) void tud_cdc_rx_cb();

// Invoked when received `wanted_char`
__attribute__((weak)) void tud_cdc_rx_wanted_cb(char wanted_char);

// Invoked when a TX is complete and therefore space becomes available in TX buffer
__attribute__((weak)) void tud_cdc_tx_complete_cb();

// Invoked when a notification is sent to host
__attribute__((weak)) void tud_cdc_notify_complete_cb();

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
__attribute__((weak)) void tud_cdc_line_state_cb(bool dtr, bool rts);

// Invoked when line coding is change via SET_LINE_CODING
__attribute__((weak)) void tud_cdc_line_coding_cb(cdc_line_coding_t const* p_line_coding);

// Invoked when received send break
// \param[in]  itf  interface for which send break was received.
// \param[in]  duration_ms  the length of time, in milliseconds, of the break signal. If a value of FFFFh, then the
//                          device will send a break until another SendBreak request is received with value 0000h.
__attribute__((weak)) void tud_cdc_send_break_cb(uint16_t duration_ms);

//--------------------------------------------------------------------+
// INTERNAL USBD-CLASS DRIVER API
//--------------------------------------------------------------------+
void cdcd_init();
bool cdcd_deinit();
void cdcd_reset();
uint16_t cdcd_open(tusb_desc_interface_t const* itf_desc, uint16_t max_len);
bool cdcd_control_xfer_cb(uint8_t stage, tusb_control_request_t const* request);
bool cdcd_xfer_cb(uint8_t ep_addr, uint32_t xferred_bytes);

#endif /* _TUSB_CDC_DEVICE_H_ */
