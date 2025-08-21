#include <stdint.h>

#include "board_hal.h"
#include "clock.h"
#include "gpio.h"
#include "diagnostics.h"
#include "timers.h"

#include "stm32g0b1xx.h"

void diag_usart2_init();
void tmc2209_uart4_init();
void tmc2209_tick(uint32_t elapsed_ms, uint32_t elapsed_sec);
void init_eeprom();
void i2c1_master_init();

void init_gpio() {
  // Enable GPIO ports A, B, C & D
  RCC->IOPENR |= (RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN);

  GPIO_SET_MODE(GPIOD, BIT_08_POS, MODER_OUT);  // Set LED status (PD8) to ouput
  GPIO_SET_MODE(GPIOC, BIT_06_POS, MODER_OUT);  // Set FAN 0 (PC6) to output
  GPIO_SET_MODE(GPIOB, BIT_15_POS, MODER_OUT);  // Set FAN 2 (PB15) to output
  GPIO_SET_MODE(GPIOC, BIT_08_POS, MODER_OUT);  // Set E0 heater (PC8) to output
}

void board_init_hal() {
  // Intialise board clock configuration
  clock_init();

  // Intialise GPIO
  init_gpio();

  // Intialise timers
  timer6_init();                   // Timer 6
  status_timer7_init(1000, true);  // Timer 7 used for status LED
  stepper_timer14_init();          // Timer 14 used for stepper pulse timing

  // Diagnostics usart initialisation
  diag_usart2_init();

  // TMC2209 usart initialisation
  tmc2209_uart4_init();

  // EEPROM init (on I2C1)
  init_eeprom();
  i2c1_master_init();
}

void device_halt() {
  // Disable interrupts
  __disable_irq();

  // Halt in tight loop toggling status led
  while (true) {
    GPIOD->ODR ^= BIT_08;  // Toggle PD8 (status LED)

    // Tight loop for a bit
    // 50 ms = 50,000,000 ns
    // 50,000,000 ns / 15.625 ns = 3,200,000 cycles (for 64MHz clock)
    // 3,200,000 cycles / 3 ≈ 1,066,666 loop iterations
    for (volatile int i = 0; i < 1066667; i++) {
      __NOP();
    }
  }
}

void do_motion_planning() {
}

void hal_tick() {
  uint32_t elapsed_ms_count = get_sys_tick();
  uint32_t elapsed_sec_count = get_elapsed_seconds();

  // Run background tasks about every 100ms
  if (elapsed_ms_count % 100 == 0) {
    tmc2209_tick(elapsed_ms_count, elapsed_sec_count);
  }

  if (elapsed_ms_count % 2000 == 0) {
    do_motion_planning();
  }
}
