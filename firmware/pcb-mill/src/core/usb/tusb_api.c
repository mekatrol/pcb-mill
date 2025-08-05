#include <stdint.h>

#include "clock.h"

uint32_t tusb_time_millis_api(void) {
  return get_sys_tick();
}