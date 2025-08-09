#ifndef TUSB_CDC_DEVICE_H_
#define TUSB_CDC_DEVICE_H_

#include "cdc.h"

//--------------------------------------------------------------------+
// Class Driver Configuration
//--------------------------------------------------------------------+
#define CFG_TUD_CDC_NOTIFY 0

//--------------------------------------------------------------------+
// Driver Configuration
//--------------------------------------------------------------------+
typedef struct __attribute__((packed)) {
  uint8_t rx_persistent : 1;                    // keep rx fifo data even with bus reset or disconnect
  uint8_t tx_persistent : 1;                    // keep tx fifo data even with reset or disconnect
  uint8_t tx_overwritabe_if_not_connected : 1;  // if not connected, tx fifo can be overwritten
} tud_cdc_configure_t;

#define TUD_CDC_CONFIGURE_DEFAULT() {     \
    .rx_persistent = 0,                   \
    .tx_persistent = 0,                   \
    .tx_overwritabe_if_not_connected = 1, \
}

// Configure CDC driver behavior
bool tud_cdc_configure(const tud_cdc_configure_t* driver_cfg);

// Backward compatible
#define tud_cdc_configure_fifo_t tud_cdc_configure_t
#define tud_cdc_configure_fifo tud_cdc_configure

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
uint32_t tud_cdc_n_write(void const* buffer, uint32_t bufsize);

// Write a byte
__attribute__((always_inline)) static inline uint32_t tud_cdc_n_write_char(char ch) {
  return tud_cdc_n_write(&ch, 1);
}

// Write a null-terminated string
__attribute__((always_inline)) static inline uint32_t tud_cdc_n_write_str(char const* str) {
  return tud_cdc_n_write(str, strlen(str));
}

// Force sending data if possible, return number of forced bytes
uint32_t tud_cdc_n_write_flush();

// Return the number of bytes (characters) available for writing to TX FIFO buffer in a single n_write operation.
uint32_t tud_cdc_n_write_available();

// Clear the transmit FIFO
bool tud_cdc_n_write_clear();

//--------------------------------------------------------------------+
// Application API (Single Port)
//--------------------------------------------------------------------+

__attribute__((always_inline)) static inline bool tud_cdc_ready(void) {
  return tud_cdc_n_ready(0);
}

__attribute__((always_inline)) static inline bool tud_cdc_connected(void) {
  return tud_cdc_n_connected(0);
}

__attribute__((always_inline)) static inline uint8_t tud_cdc_get_line_state(void) {
  return tud_cdc_n_get_line_state(0);
}

__attribute__((always_inline)) static inline void tud_cdc_get_line_coding(cdc_line_coding_t* coding) {
  tud_cdc_n_get_line_coding(coding);
}

__attribute__((always_inline)) static inline void tud_cdc_set_wanted_char(char wanted) {
  tud_cdc_n_set_wanted_char(wanted);
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_available(void) {
  return tud_cdc_n_available();
}

__attribute__((always_inline)) static inline int32_t tud_cdc_read_char(void) {
  return tud_cdc_n_read_char();
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_read(void* buffer, uint32_t bufsize) {
  return tud_cdc_n_read(buffer, bufsize);
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_write_char(char ch) {
  return tud_cdc_n_write_char(ch);
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_write(void const* buffer, uint32_t bufsize) {
  return tud_cdc_n_write(buffer, bufsize);
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_write_str(char const* str) {
  return tud_cdc_n_write_str(str);
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_write_flush(void) {
  return tud_cdc_n_write_flush();
}

__attribute__((always_inline)) static inline uint32_t tud_cdc_write_available(void) {
  return tud_cdc_n_write_available(0);
}

__attribute__((always_inline)) static inline bool tud_cdc_write_clear(void) {
  return tud_cdc_n_write_clear(0);
}

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
