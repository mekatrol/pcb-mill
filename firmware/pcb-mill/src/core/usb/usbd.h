#ifndef _TUSB_USBD_H_
#define _TUSB_USBD_H_

#include "tusb_common.h"

//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+

// New API to replace tud_init() to init device stack on specific roothub port
bool tud_rhport_init(uint8_t rhport, const tusb_rhport_init_t* rh_init);

__attribute__((always_inline)) static inline bool tud_init(uint8_t rhport) {
  const tusb_rhport_init_t rh_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL};
  return tud_rhport_init(rhport, &rh_init);
}

// Deinit device stack on roothub port
bool tud_deinit(uint8_t rhport);

// Check if device stack is already initialized
bool tud_inited(void);

// Task function should be called in main/rtos loop, extended version of tud_task()
// - timeout_ms: millisecond to wait, zero = no wait, 0xFFFFFFFF = wait forever
// - in_isr: if function is called in ISR
void tud_task_ext(uint32_t timeout_ms, bool in_isr);

// Task function should be called in main/rtos loop
__attribute__((always_inline)) static inline void tud_task(void) {
  tud_task_ext(UINT32_MAX, false);
}

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

// Enable or disable the Start Of Frame callback support
void tud_sof_cb_enable(bool en);

// Carry out Data and Status stage of control transfer
// - If len = 0, it is equivalent to sending status only
// - If len > wLength : it will be truncated
bool tud_control_xfer(uint8_t rhport, tusb_control_request_t const* request, void* buffer, uint16_t len);

// Send STATUS (zero length) packet
bool tud_control_status(uint8_t rhport, tusb_control_request_t const* request);

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

// Invoked when device is mounted (configured)
void tud_mount_cb(void);

// Invoked when device is unmounted
void tud_umount_cb(void);

// Invoked when usb bus is suspended
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en);

// Invoked when usb bus is resumed
void tud_resume_cb(void);

// Invoked when there is a new usb event, which need to be processed by tud_task()/tud_task_ext()
void tud_event_hook_cb(uint8_t rhport, uint32_t eventid, bool in_isr);

// Invoked when a new (micro) frame started
void tud_sof_cb(uint32_t frame_count);

// Invoked when received control request with VENDOR TYPE
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request);

//--------------------------------------------------------------------+
// Configuration Descriptor Templates
//--------------------------------------------------------------------+

#define TUD_CONFIG_DESC_LEN (9)

// Config number, interface count, string index, total length, attribute, power in mA
#define TUD_CONFIG_DESCRIPTOR(config_num, _itfcount, _stridx, _total_len, _attribute, _power_ma) \
  9, TUSB_DESC_CONFIGURATION, U16_TO_U8S_LE(_total_len), _itfcount, config_num, _stridx, TU_BIT(7) | _attribute, (_power_ma) / 2

//--------------------------------------------------------------------+
// CDC Descriptor Templates
//--------------------------------------------------------------------+

// Length of template descriptor: 66 bytes
#define TUD_CDC_DESC_LEN (8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 7)

// CDC Descriptor Template
// Interface number, string index, EP notification address and size, EP data address (out, in) and size.
#define TUD_CDC_DESCRIPTOR(_itfnum, _stridx, _ep_notif, _ep_notif_size, _epout, _epin, _epsize)                                                                                          \
  /* Interface Associate */                                                                                                                                                              \
  8, TUSB_DESC_INTERFACE_ASSOCIATION, _itfnum, 2, TUSB_CLASS_CDC, CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL, CDC_COMM_PROTOCOL_NONE, 0,  /* CDC Control Interface */                      \
      9, TUSB_DESC_INTERFACE, _itfnum, 0, 1, TUSB_CLASS_CDC, CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL, CDC_COMM_PROTOCOL_NONE, _stridx, /* CDC Header */                                 \
      5, TUSB_DESC_CS_INTERFACE, CDC_FUNC_DESC_HEADER, U16_TO_U8S_LE(0x0120),                                                           /* CDC Call */                                   \
      5, TUSB_DESC_CS_INTERFACE, CDC_FUNC_DESC_CALL_MANAGEMENT, 0, (uint8_t)((_itfnum) + 1),                                            /* CDC ACM: support line request + send break */ \
      4, TUSB_DESC_CS_INTERFACE, CDC_FUNC_DESC_ABSTRACT_CONTROL_MANAGEMENT, 6,                                                          /* CDC Union */                                  \
      5, TUSB_DESC_CS_INTERFACE, CDC_FUNC_DESC_UNION, _itfnum, (uint8_t)((_itfnum) + 1),                                                /* Endpoint Notification */                      \
      7, TUSB_DESC_ENDPOINT, _ep_notif, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_ep_notif_size), 1,                                          /* CDC Data Interface */                         \
      9, TUSB_DESC_INTERFACE, (uint8_t)((_itfnum) + 1), 0, 2, TUSB_CLASS_CDC_DATA, 0, 0, 0,                                             /* Endpoint Out */                               \
      7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0,                                                         /* Endpoint In */                                \
      7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0

#endif /* _TUSB_USBD_H_ */
