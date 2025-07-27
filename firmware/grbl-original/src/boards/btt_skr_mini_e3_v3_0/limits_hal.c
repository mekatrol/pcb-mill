#include <stdint.h>

#include "hal.h"
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

  // Clear EXTI0 (bits 7:0), EXTI1 (15:8), EXTI2 (23:16)
  EXTI->EXTICR[0] &= ~((0xFF << 0) | (0xFF << 8) | (0xFF << 16));

  // Set EXTI0–2 to Port C (0x02)
  EXTI->EXTICR[0] |= (0x02 << 0) | (0x02 << 8) | (0x02 << 16);

  // Configure EXTI lines 0–2 to trigger on rising and falling edges
  EXTI->RTSR1 |= (1 << 0) | (1 << 1) | (1 << 2);
  EXTI->FTSR1 |= (1 << 0) | (1 << 1) | (1 << 2);

  // Unmask EXTI lines
  EXTI->IMR1 |= (1 << 0) | (1 << 1) | (1 << 2);

  // Enable interrupts
  ENABLE_IRQ(EXTI0_1_IRQn);
  ENABLE_IRQ(EXTI2_3_IRQn);
}

uint8_t limits_get_state_hal() {
  return 0;
}

// Limit/stop inputs irq handlers
void EXTI0_1_IRQHandler(void) {
  if (EXTI->RPR1 & (1 << 0)) {
    EXTI->RPR1 = (1 << 0);  // Clear
    // Handle PC0 edge
    // TODO: limits_triggered();
  }

  if (EXTI->RPR1 & (1 << 1)) {
    EXTI->RPR1 = (1 << 1);  // Clear
    // Handle PC1 edge
    // TODO: limits_triggered();
  }

  if (EXTI->FPR1 & (1 << 0)) {
    EXTI->FPR1 = (1 << 0);  // Clear
    // Handle PC0 edge
    // TODO: limits_triggered();
  }

  if (EXTI->FPR1 & (1 << 1)) {
    EXTI->FPR1 = (1 << 1);  // Clear
    // Handle PC1 edge
    // TODO: limits_triggered();
  }

  // Disable steppers
  steppers_enable_hal(false);
}

void EXTI2_3_IRQHandler(void) {
  if (EXTI->RPR1 & (1 << 2)) {
    EXTI->RPR1 = (1 << 2);  // Clear
    // Handle PC2 edge
    // TODO: limits_triggered();
  }

  if (EXTI->FPR1 & (1 << 2)) {
    EXTI->FPR1 = (1 << 2);  // Clear
    // Handle PC2 edge
    // TODO: limits_triggered();
  }

  // Disable steppers
  steppers_enable_hal(false);
}