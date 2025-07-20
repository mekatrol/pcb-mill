#include <stdint.h>

#include "../../grbl/grbl.h"
#include "clock.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

#define COOLANT_FLOOD_PC6_POS BIT_06_POS
#define COOLANT_FLOOD_PC6 (1 << COOLANT_FLOOD_PC6_POS)

#define COOLANT_MIST_PC7_POS BIT_07_POS
#define COOLANT_MIST_PC7 (1 << COOLANT_MIST_PC7_POS)

void coolant_stop_hal() {
  // Turn off flood
  GPIOC->ODR &= ~COOLANT_FLOOD_PC6;
#define ENABLE_M7

#ifdef ENABLE_M7
  // Turn off mist
  GPIOC->ODR &= ~COOLANT_MIST_PC7;
#endif
}

void coolant_init_hal() {
  // Set coolant flood (PC6) to output
  GPIOC->MODER &= ~(MODER_MSK << (COOLANT_FLOOD_PC6_POS *
                                  MODER_BIT_COUNT));  // Clear PC6 mode bits
  GPIOC->MODER |= (MODER_OUT << (COOLANT_FLOOD_PC6_POS *
                                 MODER_BIT_COUNT));  // Set PC6 as output

#ifdef ENABLE_M7
  // Set coolant mist (PC7) to output
  GPIOC->MODER &= ~(MODER_MSK << (COOLANT_MIST_PC7_POS *
                                  MODER_BIT_COUNT));  // Clear PC7 mode bits
  GPIOC->MODER |= (MODER_OUT << (COOLANT_MIST_PC7_POS *
                                 MODER_BIT_COUNT));  // Set PC7 as output
#endif

  // Stop coolant flow
  coolant_stop_hal();
}

void coolant_set_state_hal(uint8_t mode) {
  if (mode == COOLANT_FLOOD_ENABLE) {
    GPIOC->ODR |= COOLANT_FLOOD_PC6;

#ifdef ENABLE_M7
  } else if (mode == COOLANT_MIST_ENABLE) {
    GPIOC->ODR |= COOLANT_MIST_PC7;
#endif

  } else {
    coolant_stop();
  }
}