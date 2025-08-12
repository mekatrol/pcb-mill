#ifndef TUSB_USBD_PVT_H_
#define TUSB_USBD_PVT_H_

#include "tusb_private.h"
#include "tusb_types.h"

//--------------------------------------------------------------------+
// Class Driver API
//--------------------------------------------------------------------+

typedef bool (*usbd_control_xfer_cb_t)(uint8_t stage, tusb_control_request_t const* request);

// Open an endpoint
bool usbd_edpt_open(usb_endpoint_descriptor_t const* desc_ep);

// Submit a usb transfer
bool usbd_edpt_xfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes);

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

bool usb_endpoint_open_set(uint8_t const* p_desc, uint8_t ep_count, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in);

#endif
