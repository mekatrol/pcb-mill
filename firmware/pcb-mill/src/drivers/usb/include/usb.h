#ifndef __USB_H__
#define __USB_H__

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "diagnostics.h"
#include "macros.h"
#include "usb_constants.h"
#include "usb_macros.h"
#include "usb_types.h"

/*
 * USB HAL methods - these must be implmented by a HAL (typically the board code)
 */
void usb_init_board_hal();                                                                                  //
void usb_device_start_hal();                                                                                //
void usb_init_enable_hal();                                                                                 // Start USB in device mode
bool usb_ep_queue_transfer_hal(uint8_t ep_idn, uint8_t ep_dir_idx, uint8_t* buffer, uint16_t total_bytes);  // Queue endpoint transfer in HAL
void usb_device_set_addr_hal(const uint8_t device_addr);                                                    //
bool usb_ep_stall_get_hal(uint8_t ep_idn, uint8_t ep_dir_idx);                                              //
void usb_ep_stall_set_hal(uint8_t ep_idn, uint8_t ep_dir_idx);                                              //
void usb_ep_stall_clear_hal(uint8_t ep_idn, uint8_t ep_dir_idx);                                            //
bool usb_ep_open_hal(const usb_ep_descriptor_t* ep_descriptor);                                             //
bool usb_remote_wakeup_start_hal();                                                                         // Call this in HAL to start wakeup from suspension
void usb_systick_hal();                                                                                     // This should be called but core about every 1ms, used for timeouts

/*
 * USB Device level methods
 */
void usb_reset();                                                          // Reset USB subsystem
void usb_device_init();                                                    // Initialise USB device
bool usb_process_control_request(const usb_control_request_t* request);    // Process a control request
bool usb_control_transfer(uint8_t ep_addr, uint32_t transferred_bytes);    // An EP0 control transfer has completed
bool usb_control_init_status_stage(const usb_control_request_t* request);  //
void usb_device_suspended();                                               // HAL can call this method to indicate the device has been suspended
void usb_device_suspended_sof_timeout();                                   // HAL can call this method to indicate the device has been suspended due to SOF timeout

/*
 * USB endpoint level methods
 */
void usb_ep_close_all();                                                   // Close all endpoints (unconfigure them)
bool usb_ep_buffer_transfer(uint8_t ep_addr, uint32_t transferred_bytes);  // Transfer bytes between and endpoint and the endpoint function (e.g. CDC) buffers
bool usb_ep_initiate_control_response(                                     // Initiate a staged control response
    const usb_control_request_t* request,                                  //
    const uint8_t* buffer,                                                 //
    uint16_t len);                                                         //
bool usb_ep_open_in_out_pair(                                              // Configure consecutive endpoint descriptors (IN & OUT)
    const usb_ep_descriptor_t* descriptor,                                 //
    uint8_t transfer_type,                                                 //
    uint8_t* ep_addr_out,                                                  //
    uint8_t* ep_addr_in);                                                  //

/*
 * USB descriptor methods
 */
const usb_configuration_descriptor_t* usb_get_configuration_descriptor();  // Get the USB descriptor configuration
const uint8_t* usb_get_device_descriptor();                                // Get the USB device descriptor
const uint16_t* usb_descriptor_string(uint8_t index);                      // Retrieve a USB string descriptor
const uint8_t* usb_descriptor_device_qualifier();                          //

/*
 * The USB device configuration and state
 */
extern usb_device_t usb_device;

/*
 * True if device configured
 */
ALWAYS_INLINE static bool usb_configured(void) {
  return usb_device.config_num != 0;
}

/*
 * Request type, recipient and direction helpers
 */
ALWAYS_INLINE static usb_request_recipient_t usb_request_recipient(uint8_t bmRequestType) { return (bmRequestType)&USB_REQUEST_RECIPIENT_MASK; }
ALWAYS_INLINE static usb_request_type_t usb_request_type(uint8_t bmRequestType) { return ((bmRequestType)&USB_REQUEST_TYPE_MASK) >> 5; }
ALWAYS_INLINE static usb_request_direction_index_t usb_request_direction(uint8_t bmRequestType) { return USB_EP_DIR((bmRequestType)) >> 7; }

/*
 * Moves from one descriptor to next descriptor by offseting location by bLength
 */
ALWAYS_INLINE static const usb_descriptor_base_t* usb_configuration_next_descriptor(const void* desc) {
  const usb_descriptor_base_t* desc_base = (const usb_descriptor_base_t*)desc;
  return (const usb_descriptor_base_t*)(desc + desc_base->bLength);
}

/*
 * Queue a transfer in HAL
 */
ALWAYS_INLINE static bool usb_ep_queue_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  return usb_ep_queue_transfer_hal(ep_idn, ep_dir_idx, buffer, total_bytes);
}

/*
 * USB callback methods
 */
// New data received on CDC
__attribute__((weak)) void usb_mounted_cb();
__attribute__((weak)) void usb_unmounted_cb();
__attribute__((weak)) void usb_suspended_cb();

#endif  // __USB_H__