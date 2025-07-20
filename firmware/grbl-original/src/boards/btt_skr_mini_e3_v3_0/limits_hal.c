#include <stdint.h>

#include "../../grbl/hal.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

void limits_init_hal() {
  // Enable SYSCFG clock
  RCC->APBENR2 |= RCC_APBENR2_SYSCFGEN;

  // Set PC0, PC1, PC2 as inputs
  GPIO_SET_MODE(GPIOC, BIT_00_POS, MODER_INP);  // Set mode to input on PC0
  GPIO_SET_MODE(GPIOC, BIT_01_POS, MODER_INP);  // Set mode to input on PC1
  GPIO_SET_MODE(GPIOC, BIT_02_POS, MODER_INP);  // Set mode to input on PC2

  // Clear existing EXTI mappings for lines 0, 1, 2
  EXTI->EXTICR[0] &= ~((0xF << (0 * 4)) |  // EXTI0
                       (0xF << (1 * 4)) |  // EXTI1
                       (0xF << (2 * 4)));  // EXTI2

  // Set EXTI0–2 to port C (2 = PC)
  EXTI->EXTICR[0] |= ((2 << (0 * 4)) |
                      (2 << (1 * 4)) |
                      (2 << (2 * 4)));

  // Configure EXTI lines 0–2 to trigger on rising and falling edges
  EXTI->RTSR1 |= (1 << 0) | (1 << 1) | (1 << 2);
  EXTI->FTSR1 |= (1 << 0) | (1 << 1) | (1 << 2);

  // Unmask EXTI lines
  EXTI->IMR1 |= (1 << 0) | (1 << 1) | (1 << 2);

  // Enable EXTI0_1 and EXTI2_3 in NVIC
  NVIC->ISER[0] |= (1 << 5);  // EXTI0_1_IRQn = position 5
  NVIC->ISER[0] |= (1 << 6);  // EXTI2_3_IRQn = position 6
}

uint8_t limits_get_state_hal() {
  return 0;
}

// Limit/stop inputs irq handlers
void EXTI0_1_IRQHandler(void) {
  if (EXTI->RPR1 & (1 << 0)) {
    EXTI->RPR1 = (1 << 0);  // Clear
    // Handle PC0 edge
    limits_triggered();
  }

  if (EXTI->RPR1 & (1 << 1)) {
    EXTI->RPR1 = (1 << 1);  // Clear
    // Handle PC1 edge
    limits_triggered();
  }
}

void EXTI2_3_IRQHandler(void) {
  if (EXTI->RPR1 & (1 << 2)) {
    EXTI->RPR1 = (1 << 2);  // Clear
    // Handle PC2 edge
    limits_triggered();
  }
}