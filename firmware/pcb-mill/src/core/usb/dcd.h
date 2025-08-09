#ifndef TUSB_DCD_H_
#define TUSB_DCD_H_

#include "usb.h"
#include "tusb_fifo.h"
#include "tusb_types.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

typedef enum {
  DCD_EVENT_INVALID = 0,     // 0
  DCD_EVENT_BUS_RESET,       // 1
  DCD_EVENT_UNPLUGGED,       // 2
  DCD_EVENT_SOF,             // 3
  DCD_EVENT_SETUP_RECEIVED,  // 6
  DCD_EVENT_XFER_COMPLETE,   // 7
  USBD_EVENT_FUNC_CALL,      // 8 Not an DCD event, just a convenient way to defer ISR function
  DCD_EVENT_COUNT
} dcd_eventid_t;

typedef struct __attribute__((aligned(4))) {
  uint8_t event_id;

  union {
    // SOF
    struct {
      uint32_t frame_count;
    } sof;

    // SETUP_RECEIVED
    tusb_control_request_t setup_received;

    // XFER_COMPLETE
    struct {
      uint8_t ep_addr;
      uint32_t len;
    } xfer_complete;

    // FUNC_CALL
    struct {
      void (*func)(void*);
      void* param;
    } func_call;
  };
} dcd_event_t;

//--------------------------------------------------------------------+
// Controller API
//--------------------------------------------------------------------+

// Initialize controller to device mode
void dcd_init();

// Deinitialize controller, unset device mode.
bool dcd_deinit();

// Interrupt Handler
void dcd_int_handler();

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

// Submit a transfer, When complete dcd_event_xfer_complete() is invoked to notify the stack
bool dcd_edpt_xfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes);

// Submit an transfer using fifo, When complete dcd_event_xfer_complete() is invoked to notify the stack
// This API is optional, may be useful for register-based for transferring data.
bool dcd_edpt_xfer_fifo(uint8_t ep_addr, tu_fifo_t* ff, uint16_t total_bytes) __attribute__((weak));

// Stall endpoint, any queuing transfer should be removed from endpoint
void dcd_edpt_stall(uint8_t ep_addr);

// clear stall, data toggle is also reset to DATA0
// This API never calls with control endpoints, since it is auto cleared when receiving setup packet
void dcd_edpt_clear_stall(uint8_t ep_addr);

// Allocate packet buffer used by ISO endpoints
// Some MCU need manual packet buffer allocation, we allocate the largest size to avoid clustering
bool dcd_edpt_iso_alloc(uint8_t ep_addr, uint16_t largest_packet_size);

// Configure and enable an ISO endpoint according to descriptor
bool dcd_edpt_iso_activate(tusb_desc_endpoint_t const* desc_ep);

//--------------------------------------------------------------------+
// Event API (implemented by stack)
//--------------------------------------------------------------------+

// Called by DCD to notify device stack
extern void dcd_event_handler(dcd_event_t const* event, bool in_isr);

// helper to send bus signal event
__attribute__((always_inline)) static inline void dcd_event_bus_signal(dcd_eventid_t eid, bool in_isr) {
  dcd_event_t event;
  event.event_id = eid;
  dcd_event_handler(&event, in_isr);
}

// helper to send bus reset event
__attribute__((always_inline)) static inline void dcd_event_bus_reset(bool in_isr) {
  dcd_event_t event;
  event.event_id = DCD_EVENT_BUS_RESET;
  dcd_event_handler(&event, in_isr);
}

// helper to send setup received
__attribute__((always_inline)) static inline void dcd_event_setup_received(uint8_t const* setup, bool in_isr) {
  dcd_event_t event;
  event.event_id = DCD_EVENT_SETUP_RECEIVED;
  memcpy(&event.setup_received, setup, sizeof(tusb_control_request_t));
  dcd_event_handler(&event, in_isr);
}

// helper to send transfer complete event
__attribute__((always_inline)) static inline void dcd_event_xfer_complete(uint8_t ep_addr, uint32_t xferred_bytes, bool in_isr) {
  dcd_event_t event;
  event.event_id = DCD_EVENT_XFER_COMPLETE;
  event.xfer_complete.ep_addr = ep_addr;
  event.xfer_complete.len = xferred_bytes;
  dcd_event_handler(&event, in_isr);
}

__attribute__((always_inline)) static inline void dcd_event_sof(uint32_t frame_count, bool in_isr) {
  dcd_event_t event;
  event.event_id = DCD_EVENT_SOF;
  event.sof.frame_count = frame_count;
  dcd_event_handler(&event, in_isr);
}

#endif
