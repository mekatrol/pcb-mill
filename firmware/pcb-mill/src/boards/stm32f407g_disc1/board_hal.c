#include <stdint.h>

void device_halt() {}

void diag_flush() {}

void diag_send(uint8_t b) { (void)b; }

void board_init_hal() {}

void limits_init_hal() {}
