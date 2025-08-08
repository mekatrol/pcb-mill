#ifndef TUSB_USBD_PVT_H_
#define TUSB_USBD_PVT_H_

#include "osal.h"
#include "tusb_fifo.h"
#include "tusb_private.h"
#include "tusb_option.h"
#include "tusb_types.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

typedef enum {
  SOF_CONSUMER_USER = 0,
  SOF_CONSUMER_AUDIO,
} sof_consumer_t;

//--------------------------------------------------------------------+
// Class Driver API
//--------------------------------------------------------------------+

typedef struct {
  char const* name;
  void (*init)(void);
  bool (*deinit)(void);
  void (*reset)();
  uint16_t (*open)(tusb_desc_interface_t const* desc_intf, uint16_t max_len);
  bool (*control_xfer_cb)(uint8_t stage, tusb_control_request_t const* request);
  bool (*xfer_cb)(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
  bool (*xfer_isr)(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);  // optional, return false to defer to xfer_cb()
  void (*sof)(uint32_t frame_count);                                                // optional
} usbd_class_driver_t;

typedef bool (*usbd_control_xfer_cb_t)(uint8_t stage, tusb_control_request_t const* request);

void usbd_int_set(bool enabled);

// Open an endpoint
bool usbd_edpt_open(tusb_desc_endpoint_t const* desc_ep);

// Close an endpoint
void usbd_edpt_close(uint8_t ep_addr);

// Submit a usb transfer
bool usbd_edpt_xfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes);

// Submit a usb ISO transfer by use of a FIFO (ring buffer) - all bytes in FIFO get transmitted
bool usbd_edpt_xfer_fifo(uint8_t ep_addr, tu_fifo_t* ff, uint16_t total_bytes);

// Claim an endpoint before submitting a transfer.
// If caller does not make any transfer, it must release endpoint for others.
bool usbd_edpt_claim(uint8_t ep_addr);

// Release claimed endpoint without submitting a transfer
bool usbd_edpt_release(uint8_t ep_addr);

// Check if endpoint is busy transferring
bool usbd_edpt_busy(uint8_t ep_addr);

// Stall endpoint
void usbd_edpt_stall(uint8_t ep_addr);

// Clear stalled endpoint
void usbd_edpt_clear_stall(uint8_t ep_addr);

// Check if endpoint is stalled
bool usbd_edpt_stalled(uint8_t ep_addr);

// Check if endpoint is ready (not busy and not stalled)
__attribute__((always_inline)) static inline bool usbd_edpt_ready(uint8_t ep_addr) {
  return !usbd_edpt_busy(ep_addr) && !usbd_edpt_stalled(ep_addr);
}

/*------------------------------------------------------------------*/
/* Helper
 *------------------------------------------------------------------*/

bool usbd_open_edpt_pair(uint8_t const* p_desc, uint8_t ep_count, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in);

#endif
