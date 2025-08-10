#ifndef _TUSB_USBD_H_
#define _TUSB_USBD_H_

#include "usb.h"
#include "tusb_types.h"

typedef struct __attribute__((packed)) {
  volatile uint8_t busy : 1;
  volatile uint8_t stalled : 1;
  volatile uint8_t claimed : 1;
} tu_edpt_state_t;

typedef struct {
  struct __attribute__((packed)) {
    volatile uint8_t connected : 1;
    volatile uint8_t addressed : 1;

    uint8_t remote_wakeup_en : 1;       // enable/disable by host
    uint8_t remote_wakeup_support : 1;  // configuration descriptor's attribute
    uint8_t self_powered : 1;           // configuration descriptor's attribute
  };
  volatile uint8_t cfg_num;  // current active configuration (0x00 is not configured)

  uint8_t itf2drv[USB_MAX_INTERFACES];  // map interface number to driver (0xff is invalid)
  uint8_t ep2drv[USB_ENDPOINT_MAX][2];  // map endpoint to driver ( 0xff is invalid ), can use only 4-bit each

  tu_edpt_state_t ep_status[USB_ENDPOINT_MAX][2];

} usbd_device_t;

extern usbd_device_t _usbd_dev;

bool process_control_request(tusb_control_request_t const* p_request);

void tud_task_ext();

void usbd_int_set(bool enabled);

// Check if device is connected (may not mounted/configured yet)
// True if just got out of Bus Reset and received the very first data from host
bool tud_connected(void);

// Check if device is connected and configured
bool tud_mounted(void);

// Check if device is ready to transfer
__attribute__((always_inline)) static inline bool tud_ready(void) {
  return tud_mounted();
}

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

// Invoked when received control request with VENDOR TYPE
bool tud_vendor_control_xfer_cb(uint8_t stage, tusb_control_request_t const* request);

#endif /* _TUSB_USBD_H_ */
