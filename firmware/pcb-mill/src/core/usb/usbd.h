#ifndef _TUSB_USBD_H_
#define _TUSB_USBD_H_

#include "usb.h"
#include "tusb_option.h"
#include "tusb_types.h"

void tud_task_ext();

// Get current bus speed
tusb_speed_t tud_speed_get(void);

// Check if device is connected (may not mounted/configured yet)
// True if just got out of Bus Reset and received the very first data from host
bool tud_connected(void);

// Check if device is connected and configured
bool tud_mounted(void);

// Check if device is suspended
bool tud_suspended(void);

// Check if device is ready to transfer
__attribute__((always_inline)) static inline bool tud_ready(void) {
  return tud_mounted() && !tud_suspended();
}

// Remote wake up host, only if suspended and enabled by host
bool tud_remote_wakeup(void);

// Enable pull-up resistor on D+ D-
// Return false on unsupported MCUs
bool tud_disconnect(void);

// Disable pull-up resistor on D+ D-
// Return false on unsupported MCUs
bool tud_connect(void);

// Carry out Data and Status stage of control transfer
// - If len = 0, it is equivalent to sending status only
// - If len > wLength : it will be truncated
bool tud_control_xfer(tusb_control_request_t const* request, void* buffer, uint16_t len);

// Send STATUS (zero length) packet
bool tud_control_status(tusb_control_request_t const* request);

//--------------------------------------------------------------------+
// Application Callbacks
//--------------------------------------------------------------------+

// Invoked when received GET DEVICE DESCRIPTOR request
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void);

// Invoked when received GET CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index);

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid);

// Invoked when received GET BOS DESCRIPTOR request
// Application return pointer to descriptor
uint8_t const* tud_descriptor_bos_cb(void);

// Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
// device_qualifier descriptor describes information about a high-speed capable device that would
// change if the device were operating at the other speed. If not highspeed capable stall this request.
uint8_t const* tud_descriptor_device_qualifier_cb(void);

// Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
// Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index);

// Invoked when there is a new usb event
void tud_event_hook_cb(uint32_t eventid, bool in_isr);

// Invoked when received control request with VENDOR TYPE
bool tud_vendor_control_xfer_cb(uint8_t stage, tusb_control_request_t const* request);

#endif /* _TUSB_USBD_H_ */
