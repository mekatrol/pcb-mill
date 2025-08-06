/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#include "dcd.h"
#include "usbd_pvt.h"

//--------------------------------------------------------------------+
// Callback weak stubs (called if application does not provide)
//--------------------------------------------------------------------+
__attribute__((weak)) void dcd_edpt0_status_complete(uint8_t rhport, const tusb_control_request_t* request) {
  (void)rhport;
  (void)request;
}

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

enum {
  EDPT_CTRL_OUT = 0x00,
  EDPT_CTRL_IN = 0x80
};

typedef struct {
  tusb_control_request_t request;
  uint8_t* buffer;
  uint16_t data_len;
  uint16_t total_xferred;
  usbd_control_xfer_cb_t complete_cb;
} usbd_control_xfer_t;

static usbd_control_xfer_t _ctrl_xfer;

static struct {
  union {
    __attribute__((aligned(4)))
    uint8_t buf[CFG_TUD_ENDPOINT0_SIZE];

    __attribute__((aligned(CFG_TUD_MEM_DCACHE_ENABLE_DEFAULT ? CFG_TUSB_MEM_DCACHE_LINE_SIZE : 1)))
    uint8_t buf_dcache_padding[(CFG_TUD_MEM_DCACHE_ENABLE_DEFAULT ? (TU_DIV_CEIL(CFG_TUD_ENDPOINT0_SIZE, CFG_TUSB_MEM_DCACHE_LINE_SIZE) *
                                                                     CFG_TUSB_MEM_DCACHE_LINE_SIZE)
                                                                  : (CFG_TUD_ENDPOINT0_SIZE))];
  };
} _ctrl_epbuf;

//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+

// Queue ZLP status transaction
static inline bool status_stage_xact(uint8_t rhport, const tusb_control_request_t* request) {
  // Opposite to endpoint in Data Phase
  const uint8_t ep_addr = request->bmRequestType_bit.direction ? EDPT_CTRL_OUT : EDPT_CTRL_IN;
  return usbd_edpt_xfer(rhport, ep_addr, NULL, 0);
}

// Status phase
bool tud_control_status(uint8_t rhport, const tusb_control_request_t* request) {
  _ctrl_xfer.request = (*request);
  _ctrl_xfer.buffer = NULL;
  _ctrl_xfer.total_xferred = 0;
  _ctrl_xfer.data_len = 0;

  return status_stage_xact(rhport, request);
}

// Queue a transaction in Data Stage
// Each transaction has up to Endpoint0's max packet size.
// This function can also transfer an zero-length packet
static bool data_stage_xact(uint8_t rhport) {
  const uint16_t xact_len = tu_min16(_ctrl_xfer.data_len - _ctrl_xfer.total_xferred, CFG_TUD_ENDPOINT0_SIZE);
  uint8_t ep_addr = EDPT_CTRL_OUT;

  if (_ctrl_xfer.request.bmRequestType_bit.direction == TUSB_DIR_IN) {
    ep_addr = EDPT_CTRL_IN;
    if (xact_len) {
      if (tu_memcpy_s(_ctrl_epbuf.buf, CFG_TUD_ENDPOINT0_SIZE, _ctrl_xfer.buffer, xact_len) != 0) {
        return false;
      }
    }
  }

  return usbd_edpt_xfer(rhport, ep_addr, xact_len ? _ctrl_epbuf.buf : NULL, xact_len);
}

// Transmit data to/from the control endpoint.
// If the request's wLength is zero, a status packet is sent instead.
bool tud_control_xfer(uint8_t rhport, const tusb_control_request_t* request, void* buffer, uint16_t len) {
  _ctrl_xfer.request = (*request);
  _ctrl_xfer.buffer = (uint8_t*)buffer;
  _ctrl_xfer.total_xferred = 0U;
  _ctrl_xfer.data_len = tu_min16(len, request->wLength);

  if (request->wLength > 0U) {
    if (_ctrl_xfer.data_len > 0U) {
      if (!buffer) {
        return false;
      }
    }
    if (!data_stage_xact(rhport)) {
      return false;
    }
  } else {
    if (!status_stage_xact(rhport, request)) {
      return false;
    }
  }

  return true;
}

//--------------------------------------------------------------------+
// USBD API
//--------------------------------------------------------------------+
void usbd_control_reset(void);
void usbd_control_set_request(const tusb_control_request_t* request);
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp);
bool usbd_control_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);

void usbd_control_reset(void) {
  memset(&_ctrl_xfer, 0, sizeof(usbd_control_xfer_t));
}

// Set complete callback
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp) {
  _ctrl_xfer.complete_cb = fp;
}

// for dcd_set_address where DCD is responsible for status response
void usbd_control_set_request(const tusb_control_request_t* request) {
  _ctrl_xfer.request = (*request);
  _ctrl_xfer.buffer = NULL;
  _ctrl_xfer.total_xferred = 0;
  _ctrl_xfer.data_len = 0;
}

// callback when a transaction complete on
// - DATA stage of control endpoint or
// - Status stage
bool usbd_control_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
  (void)result;

  // Endpoint Address is opposite to direction bit, this is Status Stage complete event
  if (tu_edpt_dir(ep_addr) != _ctrl_xfer.request.bmRequestType_bit.direction) {
    if (xferred_bytes != 0) {
      return false;
    }

    // invoke optional dcd hook if available
    dcd_edpt0_status_complete(rhport, &_ctrl_xfer.request);

    if (_ctrl_xfer.complete_cb) {
      // TODO refactor with usbd_driver_print_control_complete_name
      _ctrl_xfer.complete_cb(rhport, CONTROL_STAGE_ACK, &_ctrl_xfer.request);
    }

    return true;
  }

  if (_ctrl_xfer.request.bmRequestType_bit.direction == TUSB_DIR_OUT) {
    if (!_ctrl_xfer.buffer) {
      return false;
    }
    memcpy(_ctrl_xfer.buffer, _ctrl_epbuf.buf, xferred_bytes);
  }

  _ctrl_xfer.total_xferred += (uint16_t)xferred_bytes;
  _ctrl_xfer.buffer += xferred_bytes;

  // Data Stage is complete when all request's length are transferred or
  // a short packet is sent including zero-length packet.
  if ((_ctrl_xfer.request.wLength == _ctrl_xfer.total_xferred) ||
      (xferred_bytes < CFG_TUD_ENDPOINT0_SIZE)) {
    // DATA stage is complete
    bool is_ok = true;

    // invoke complete callback if set
    // callback can still stall control in status phase e.g out data does not make sense
    if (_ctrl_xfer.complete_cb) {
      is_ok = _ctrl_xfer.complete_cb(rhport, CONTROL_STAGE_DATA, &_ctrl_xfer.request);
    }

    if (is_ok) {
      if (!status_stage_xact(rhport, &_ctrl_xfer.request)) {
        return false;
      }
    } else {
      // Stall both IN and OUT control endpoint
      dcd_edpt_stall(rhport, EDPT_CTRL_OUT);
      dcd_edpt_stall(rhport, EDPT_CTRL_IN);
    }
  } else {
    // More data to transfer
    if (!data_stage_xact(rhport)) {
      return false;
    }
  }

  return true;
}
