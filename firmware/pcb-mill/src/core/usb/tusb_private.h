#ifndef TUSB_PRIVATE_H_
#define TUSB_PRIVATE_H_

#include "tusb_types.h"

//--------------------------------------------------------------------+
// Endpoint
//--------------------------------------------------------------------+

typedef struct __attribute__((packed)) {
  volatile uint8_t busy : 1;
  volatile uint8_t stalled : 1;
  volatile uint8_t claimed : 1;
} tu_edpt_state_t;

//--------------------------------------------------------------------+
// Endpoint
//--------------------------------------------------------------------+

// Check if endpoint descriptor is valid per USB specs
bool tu_edpt_validate(tusb_desc_endpoint_t const* desc_ep, bool is_host);

// Bind all endpoint of a interface descriptor to class driver
void tu_edpt_bind_driver(uint8_t ep2drv[][2], tusb_desc_interface_t const* p_desc, uint16_t desc_len);

// Calculate total length of n interfaces (depending on IAD)
uint16_t tu_desc_get_interface_total_len(tusb_desc_interface_t const* desc_itf, uint8_t itf_count, uint16_t max_len);

// Claim an endpoint
bool tu_edpt_claim(tu_edpt_state_t* ep_state);

// Release an endpoint
bool tu_edpt_release(tu_edpt_state_t* ep_state);

#endif
