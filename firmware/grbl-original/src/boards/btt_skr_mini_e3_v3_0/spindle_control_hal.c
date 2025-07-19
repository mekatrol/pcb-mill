#include <stdint.h>

#include "../../grbl/grbl.h"
#include "clock.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

#define SPINDLE_PC9_POS BIT_09
#define SPINDLE_PC9 (1 << SPINDLE_PC9_POS)

void spindle_stop_hal() {
  // Turn off spindle
  GPIOC->ODR &= ~SPINDLE_PC9;
}

void spindle_init_hal() {
  // Set spindle PC9 to output
  GPIOC->MODER &= ~(MODER_MSK << (SPINDLE_PC9_POS *
                                  MODER_BIT_COUNT));  // Clear PC9 mode bits
  GPIOC->MODER |= (MODER_OUT << (SPINDLE_PC9_POS *
                                 MODER_BIT_COUNT));  // Set PC9 as output

#ifdef VARIABLE_SPINDLE
  // TODO: Variable spindle PWM
#endif

  // Stop spindle
  spindle_stop_hal();
}

void spindle_set_state_hal(uint8_t state, float rpm) {
  if (state == SPINDLE_DISABLE) {
    coolant_stop();
  } else {
    GPIOC->ODR |= SPINDLE_PC9;
  }
}