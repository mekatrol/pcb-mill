#include <stdint.h>

#include "clock.h"
#include "uart.h"

void device_halt() {}

void board_init_hal() {
  clock_init();
  uart_init();
}

void limits_init_hal() {}
