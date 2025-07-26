#include "hal.h"

#include <stdint.h>

#include "clock.h"
#include "eeprom_hal.h"
#include "gpio.h"
#include "irq.h"
#include "log.h"
#include "memory_map.h"
#include "register_bits.h"
#include "timers.h"
#include "tmc2209.h"

inline void interrupts_enable() {
  enable_irq();
}

inline void interrupts_disable() {
  disable_irq();
}

void init_gpio() {
  // Enable GPIO ports
  RCC->IOPENR |= IOPENR_PORTA_ENABLE;  // Enable PORTA
  RCC->IOPENR |= IOPENR_PORTB_ENABLE;  // Enable PORTB
  RCC->IOPENR |= IOPENR_PORTC_ENABLE;  // Enable PORTC
  RCC->IOPENR |= IOPENR_PORTD_ENABLE;  // Enable PORTD

  GPIO_SET_MODE(GPIOD, BIT_08_POS, MODER_OUT);  // Set LED status (PD8) to ouput
  GPIO_SET_MODE(GPIOC, BIT_06_POS, MODER_OUT);  // Set FAN 0 (PC6) to output
  GPIO_SET_MODE(GPIOB, BIT_15_POS, MODER_OUT);  // Set FAN 2 (PB15) to output
  GPIO_SET_MODE(GPIOC, BIT_08_POS, MODER_OUT);  // Set E0 heater (PC8) to output
}

void board_init_hal() {
  init_clock();

  init_gpio();

  // Init timers
  timer6_init();
  timer7_init(1000, true);
  timer14_init();

  set_timer7_interval(200);

  // TMC2209 uart
  tmc2209_uart4_init();

  init_eeprom();
  i2c1_master_init();
}

void system_init_hal() {
}

uint8_t tmc2209_configure_addr = 0;

void hal_tick() {
  if (tmc2209_configure_addr > 3) {
    return;
  }

  uint8_t errors = 0;

  uart_printf("Reading gconf for addr: 0x%x\r\n", tmc2209_configure_addr);

  tmc2209_read_gconf(tmc2209_configure_addr);

  delay_ms(100);

  uint8_t gconf[4];
  uint32_t value;
  int result = tmc2209_parse_reply(4, gconf);
  if (result == 0) {
    value = gconf[0] | (gconf[1] << 8) | (gconf[2] << 16) | (gconf[3] << 24);
    uart_printf("gconf: 0x%x [for addr: 0x%x]\r\n", value, tmc2209_configure_addr);
  } else {
    errors++;
  }

  if (errors == 0) {
    set_timer7_interval(1000);
  } else {
    set_timer7_interval(100);
  }

  tmc2209_configure_addr++;
}
