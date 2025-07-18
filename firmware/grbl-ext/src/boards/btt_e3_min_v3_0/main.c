#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../drivers/logging/log_uart.h"
#include "../../drivers/timers/timers.h"
#include "../../drivers/tmc2209/tmc2209.h"
#include "../../drivers/uart/uart.h"
#include "../../grbl/grbl.h"
#include "clock.h"
#include "fans.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"
#include "steppers.h"

void panic() { while (true); }

int main(void) {
  init_clock();

  // Enable GPIO ports
  RCC->IOPENR |= IOPENR_PORTA_ENABLE;  // Enable PORTA
  RCC->IOPENR |= IOPENR_PORTB_ENABLE;  // Enable PORTB
  RCC->IOPENR |= IOPENR_PORTC_ENABLE;  // Enable PORTC
  RCC->IOPENR |= IOPENR_PORTD_ENABLE;  // Enable PORTD

  // Enable GPIOC peripheral
  GPIOC->MODER &= ~(MODER_MSK << (BIT_06 * MODER_BIT_COUNT));  // Clear PC6 mode bits
  GPIOC->MODER |= (MODER_OUT << (BIT_06 * MODER_BIT_COUNT));   // Set PC6 as output

  // Enable GPIOD peripheral
  GPIOD->MODER &= ~(MODER_MSK << (BIT_08 * MODER_BIT_COUNT));  // Clear PD8 mode bits
  GPIOD->MODER |= (MODER_OUT << (BIT_08 * MODER_BIT_COUNT));   // Set PD8 as output

  timer6_init(500, true);
  timer7_init(1000, true);
  timer14_init();

  uart2_init();
  uart4_init();

  init_steppers();

  enable_irq();

  delay_ms(100);

  tmc2209_read_gconf(0x00);  // Send read to slave 0

  delay_ms(100);

  uint8_t gconf[4];
  uint32_t value;
  int result = tmc2209_parse_reply(4, gconf);
  if (result == 0) {
    value = gconf[0] | (gconf[1] << 8) | (gconf[2] << 16) | (gconf[3] << 24);
    uart_printf("gconf: 0x%x\r\n", value);
  } else {
    // Handle error
    set_timer6_interval(100);
  }

  // Configure HALF
  hal_interface_t hal = {.terminal = {.getchar = uart_getc, .putchar = uart_putc, .printf = uart_printf},

                         .timer = {.delay_ms = delay_ms},

                         .steppers = {.x = {.enable = stepper_enable_x,
                                            .disable = stepper_disable_x,
                                            .set_dir = stepper_x_set_dir,
                                            .set_state = stepper_x_set_state},
                                      .y = {.enable = stepper_enable_y,
                                            .disable = stepper_disable_y,
                                            .set_dir = stepper_y_set_dir,
                                            .set_state = stepper_y_set_state},
                                      .z = {.enable = stepper_enable_z,
                                            .disable = stepper_disable_z,
                                            .set_dir = stepper_z_set_dir,
                                            .set_state = stepper_z_set_state}},

                         .enter_critical = disable_irq,
                         .exit_critical = enable_irq,
                         .panic = panic};

  // Run GRBL with HAL configuration
  grbl_run(&hal);
}
