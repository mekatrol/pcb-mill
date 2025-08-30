#include <stdint.h>

#include "clock.h"
#include "uart.h"

void device_halt() {}

void diag_flush() {}

void diag_send(uint8_t b) { (void)b; }

void board_init_hal() {
  clock_init();
  uart_init();

  uart_send_str("UART0 ready!\r\n");
}

void limits_init_hal() {}
