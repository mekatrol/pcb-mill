#include <stdint.h>

#include "irq.h"

static inline void bit_true_atomic(volatile uint8_t* addr, uint32_t mask) {
  disable_irq();  // Disable interrupts
  *addr |= mask;  // Set the bit(s)
  enable_irq();   // Re-enable interrupts
}

static inline void bit_false_atomic(volatile uint8_t* x, uint32_t mask) {
  disable_irq();    // Disable interrupts
  (*x) &= ~(mask);  // Clear the bit(s)
  enable_irq();     // Re-enable interrupts
}

static inline void bit_toggle_atomic(volatile uint8_t* x, uint32_t mask) {
  disable_irq();   // Disable interrupts
  (*x) ^= (mask);  // Toggle the bit(s)
  enable_irq();    // Re-enable interrupts
}
