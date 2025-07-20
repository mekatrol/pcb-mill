#include "irq.h"

inline void disable_irq(void) {
  __asm volatile("cpsid i" ::: "memory");
}

inline void enable_irq(void) {
  __asm volatile("cpsie i" ::: "memory");
}

inline void wait_for_interrupt(void) {
  __asm volatile("wfi");
}