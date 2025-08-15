#ifndef _TUSB_CDC_H__
#define _TUSB_CDC_H__

#include "usb.h"

/// CDC ACM (Virtual COM Port) Class-Specific Request Codes
/// See USB CDC Spec 1.2, Table 3.1 (Abstract Control Model Requests)
typedef enum {
  CDC_REQUEST_SET_LINE_CODING = 0x20,         // Set serial line coding (baud rate, stop bits, parity, data bits) :contentReference[oaicite:0]{index=0}
  CDC_REQUEST_GET_LINE_CODING = 0x21,         // Get current serial line coding :contentReference[oaicite:1]{index=1}
  CDC_REQUEST_SET_CONTROL_LINE_STATE = 0x22,  // Control RTS/DTR tone (host signals presence) :contentReference[oaicite:2]{index=2}
  CDC_REQUEST_SEND_BREAK = 0x23               // Transmit break condition on the communication line :contentReference[oaicite:3]{index=3}
} cdc_acm_request_t;

// USB CDC Communication Interface Class Subclass Codes (CDC Spec 1.2 Table 4)
typedef enum {
  CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL = 0x02  // Abstract Control Model             [USB PSTN 1.2]
} cdc_comm_subclass_type_t;

// For a USB CDC Virtual COM Port, this struct represents the Line Coding object defined in USB CDC Specification 1.2, Section 6.2.13.
typedef struct __attribute__((packed)) {
  uint32_t dwDTERate;   // Data terminal rate in bits per second (baud rate)
  uint8_t bCharFormat;  // Stop bits: 0 = 1 stop bit, 1 = 1.5 stop bits, 2 = 2 stop bits
  uint8_t bParityType;  // Parity: 0 = None, 1 = Odd, 2 = Even, 3 = Mark, 4 = Space
  uint8_t bDataBits;    // Number of data bits: typically 5, 6, 7, 8, or 16
} usb_cdc_line_coding_t;

_Static_assert(sizeof(usb_cdc_line_coding_t) == 7, "size must be 7");

#endif
