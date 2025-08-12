#ifndef TUSB_DCD_H_
#define TUSB_DCD_H_

#include "usb.h"
#include "tusb_types.h"
#include "usbd.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Controller API
//--------------------------------------------------------------------+

// Initialize controller to device mode
void dcd_init();

// Deinitialize controller, unset device mode.
bool dcd_deinit();

// Receive Set Address request, mcu port must also include status IN response
void dcd_set_address(uint8_t dev_addr);

// Connect by enabling internal pull-up resistor on D+/D-
void dcd_connect();

// Enable/Disable Start-of-frame interrupt. Default is disabled
void dcd_sof_enable(bool en);

//--------------------------------------------------------------------+
// Endpoint API
//--------------------------------------------------------------------+

// Invoked when a control transfer's status stage is complete.
// May help DCD to prepare for next control transfer, this API is optional.
void dcd_edpt0_status_complete(tusb_control_request_t const* request);

// Configure endpoint's registers according to descriptor
bool dcd_edpt_open(tusb_desc_endpoint_t const* desc_ep);

// Close all non-control endpoints, cancel all pending transfers if any.
// Invoked when switching from a non-zero Configuration by SET_CONFIGURE therefore
// required for multiple configuration support.
void dcd_edpt_close_all();

// Submit a transfer
bool dcd_edpt_xfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes);

// Stall endpoint, any queuing transfer should be removed from endpoint
void dcd_edpt_stall(uint8_t ep_addr);

// clear stall, data toggle is also reset to DATA0
// This API never calls with control endpoints, since it is auto cleared when receiving setup packet
void dcd_edpt_clear_stall(uint8_t ep_addr);

#endif
