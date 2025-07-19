#include <stdint.h>

#include "../../grbl/hal.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

// #define RCC_BASE 0x40021000UL
// #define EXTI_BASE 0x40001800UL
// #define GPIOC_BASE 0x48000800UL
// #define SYSCFG_BASE 0x40010000UL
// #define EXTI_BASE 0x40021800UL
// #define NVIC_ISER0 (*(volatile uint32_t*)0xE000E100UL)

// #define RCC_IOPENR (*(volatile uint32_t*)(RCC_BASE + 0x34))
// #define RCC_APBENR2 (*(volatile uint32_t*)(RCC_BASE + 0x40))

// #define GPIOC_MODER (*(volatile uint32_t*)(GPIOC_BASE + 0x00))
// #define GPIOC_PUPDR (*(volatile uint32_t*)(GPIOC_BASE + 0x0C))
// #define GPIOC_IDR (*(volatile uint32_t*)(GPIOC_BASE + 0x10))

// #define SYSCFG_EXTICR1 (*(volatile uint32_t*)(SYSCFG_BASE + 0x08))

// #define EXTI_RTSR1 (*(volatile uint32_t*)(EXTI_BASE + 0x00))
// #define EXTI_FTSR1 (*(volatile uint32_t*)(EXTI_BASE + 0x04))
// #define EXTI_IMR1 (*(volatile uint32_t*)(EXTI_BASE + 0x08))
// #define EXTI_PR1 (*(volatile uint32_t*)(EXTI_BASE + 0x10))

void limits_init_hal() {
  // Enable SYSCFG clock
  RCC->APBENR2 |= RCC_APBENR2_SYSCFGEN;

  // Set PC0, PC1, PC2 as inputs
  GPIOC->MODER &= ~((MODER_MSK << (BIT_00 * MODER_BIT_COUNT)) | (MODER_MSK << (BIT_01 * MODER_BIT_COUNT)) | (MODER_MSK << (BIT_02 * MODER_BIT_COUNT)));

  // Route EXTI0–2 to PC0–2 via SYSCFG_EXTICR1
  SYSCFG->CFGR1 &= ~((0xF << 0) | (0xF << 4) | (0xF << 8));
  SYSCFG->CFGR1 |= ((2 << 0) | (2 << 4) | (2 << 8));

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