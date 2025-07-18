#include "grbl.h"

#include <stddef.h>

hal_interface_t *_hal = NULL;

void grbl_run(hal_interface_t *hal) {
  hal->terminal.printf("GRBL v%d.%d.%d starting! \r\n", 0, 0, 1);

  // Disable steppers by default
  hal->steppers.x.disable();
  hal->steppers.y.disable();
  hal->steppers.z.disable();

  // Set global HAL variable
  _hal = hal;

  hal->steppers.x.enable();

  // Loop forever (GRBL never exits)
  while (true) {
    uint8_t c;
    while ((c = hal->terminal.getchar()) != 0) {
      hal->terminal.putchar(c);
    }

    hal->timer.delay_ms(10);
  }
}

volatile uint8_t step_state = 0;
volatile uint32_t step_tick = 0;
void grbl_tick() {
  // Increment tick in critical
  _hal->enter_critical();
  step_tick++;
  _hal->exit_critical();

  // Tick frequency should be 100KHz so for a 10KHz tick then
  // step_tick should be modulo 5 for half the step cycle
  if (step_tick % 5 != 0 || !_hal) {
    return;
  }

  if (step_state == 0) {
    _hal->steppers.x.set_state(step_state);
    step_state = 1;
  } else {
    _hal->steppers.x.set_state(step_state);
    step_state = 0;
  }
}