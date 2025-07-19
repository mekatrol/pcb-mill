#include "../../grbl/hal.h"

#include <stdint.h>

#include "clock.h"
#include "eeprom_hal.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

void init_gpio() {
  // Enable GPIO ports
  RCC->IOPENR |= IOPENR_PORTA_ENABLE;  // Enable PORTA
  RCC->IOPENR |= IOPENR_PORTB_ENABLE;  // Enable PORTB
  RCC->IOPENR |= IOPENR_PORTC_ENABLE;  // Enable PORTC
  RCC->IOPENR |= IOPENR_PORTD_ENABLE;  // Enable PORTD
}

extern void i2c1_master_init();

void board_init_hal() {
  init_clock();

  init_gpio();

  init_eeprom();
  i2c1_master_init();
}

void system_init_hal() {
}
