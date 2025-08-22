#ifndef __USB_CDC_H__
#define __USB_CDC_H__

#include <stdint.h>
#include <stdio.h>

#include "circular_buffer.h"

// The size of CDC endpoint buffers
#define USB_CDC_EP_BUFFER_SIZE 64UL

typedef enum {
  // wValue bits for CDC_REQUEST_SET_CONTROL_LINE_STATE (CDC Spec §6.2.14)
  CDC_CONTROL_LINE_STATE_DTR = (1U << 0U),  // Data Terminal Ready (DTR) signal
  CDC_CONTROL_LINE_STATE_RTS = (1U << 1U)   // Request To Send (RTS) signal
} handshake_state_t;

// For a USB CDC Virtual COM Port, this struct represents the Line Coding object defined in USB CDC Specification 1.2, Section 6.2.13.
typedef struct __attribute__((packed)) {
  uint32_t dwDTERate;   // Data terminal rate in bits per second (baud rate)
  uint8_t bCharFormat;  // Stop bits: 0 = 1 stop bit, 1 = 1.5 stop bits, 2 = 2 stop bits
  uint8_t bParityType;  // Parity: 0 = None, 1 = Odd, 2 = Even, 3 = Mark, 4 = Space
  uint8_t bDataBits;    // Number of data bits: typically 5, 6, 7, 8, or 16
} usb_cdc_line_coding_t;

_Static_assert(sizeof(usb_cdc_line_coding_t) == 7, "size must be 7");

typedef struct {
  // The endpoint IN/OUT addresses
  uint8_t ep_addr_in;
  uint8_t ep_addr_out;

  // Meaning:
  // Bit 0 (DTR) — Host is ready (DCE can establish a connection).
  // Bit 1 (RTS) — Host requests the device to prepare for data transmission.
  // Other bits — Reserved, must be set to zero.
  handshake_state_t flow_control_state;

  // For a USB CDC Virtual COM Port, this struct represents the Line Coding object defined in USB CDC Specification 1.2, Section 6.2.13.
  usb_cdc_line_coding_t line_coding;

  // TX and RX circular buffer state
  circular_buffer_t rx_buffer;
  circular_buffer_t tx_buffer;

  // TX and RX circular buffer data
  uint8_t rx_buffer_data[USB_CDC_EP_BUFFER_SIZE];
  uint8_t tx_buffer_data[USB_CDC_EP_BUFFER_SIZE];
} usb_cdc_t;

extern usb_cdc_t usb_cdc;

// New data received on CDC
__attribute__((weak)) void usb_cdc_rx_cb();

// All data has been transmitted through CDC
__attribute__((weak)) void usb_cdc_tx_complete_cb();

// DTR/RTS changed from SET_CONTROL_LINE_STATE
__attribute__((weak)) void usb_cdc_handshake_cb(bool dtr, bool rts);

// Line coding change from SET_LINE_CODING
__attribute__((weak)) void usb_cdc_line_coding_cb(const usb_cdc_line_coding_t* p_line_coding);

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

void usb_cdc_init();

void usb_cdc_reset();

uint16_t usb_cdc_open(const usb_control_interface_descriptor_t* control_descriptor, uint16_t descriptor_end);

bool usb_cdc_control_transfer(uint8_t control_stage, const usb_control_request_t* request);

#endif  // __USB_CDC_H__