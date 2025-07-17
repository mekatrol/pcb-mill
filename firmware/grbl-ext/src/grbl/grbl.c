#include "grbl.h"

void grbl_run(hal_interface_t *hal) {
  hal->terminal.printf("GRBL v%d.%d.%d starting! \r\n", 0, 0, 1);

  // Disable steppers by default
  hal->steppers.x.disable();
  hal->steppers.y.disable();
  hal->steppers.z.disable();

  // Loop forever (GRBL never exits)
  while (true) {
    uint8_t c;
    while ((c = hal->terminal.getchar()) != 0) {
      hal->terminal.putchar(c);
    }

    hal->timer.delay_ms(10);
  }
}