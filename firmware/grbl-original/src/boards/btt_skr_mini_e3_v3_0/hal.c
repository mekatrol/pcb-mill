#include "hal.h"

#include <stdint.h>

#include "clock.h"
#include "eeprom_hal.h"
#include "gpio.h"
#include "irq.h"
#include "log.h"
#include "memory_map.h"
#include "motion.h"
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

  set_timer7_interval(1000);

  // TMC2209 uart
  tmc2209_uart4_init();

  init_eeprom();
  i2c1_master_init();
}

void system_init_hal() {
  init_motion(0, 0, 0, 0);

  // Enable stepper cooling fan
  GPIOC->BSRR = BIT_06;  // Turn Fan 0 on

  // Enable steppers
  steppers_enable_hal(true);
}

void do_motion_planning() {
  // if (motion.steps_remaining > 0) {
  //   // No new motion while still moving
  //   return;
  // }

  // steppers_enable_hal(true);

  // if (motion.x.cur_pos == 0) {
  //   start_motion(1000, 1000, 1000, 1000);
  // } else {
  //   start_motion(0, 0, 0, 0);
  // }
}

uint32_t hal_tick_count = 0;

void hal_tick() {
  uint32_t tick_count = get_systick();

  if (tick_count == hal_tick_count) {
    return;
  }

  hal_tick_count = tick_count;

  // Run background tasks about every 100ms
  if (hal_tick_count % 100 == 0) {
    tmc2209_tick();
  }

  if (hal_tick_count % 2000 == 0) {
    do_motion_planning();
  }
}
