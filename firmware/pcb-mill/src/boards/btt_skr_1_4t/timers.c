#include <stdint.h>

#include "clock.h"

void delay_ms(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i++) {
    while ((SYST_CSR & (1 << 16)) == 0);  // Wait for COUNTFLAG
  }
}