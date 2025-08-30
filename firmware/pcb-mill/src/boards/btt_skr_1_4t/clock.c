#include <stdint.h>

#include "clock.h"
#include "macros.h"

// The PLL0FEED register is a write-only, 2-step unlock mechanism. It’s a hardware protection feature built into the NXP LPC17xx family to:
// Prevent accidental or partial changes to the PLL configuration (which could lock up the system or cause instability),
// Force software to intentionally commit changes.
// Whenever you change any PLL0 configuration registers like:
//   - PLL0CON – control (enable, connect)
//   - PLL0CFG – multiplier/divider settings
// These values are magic constants chosen by NXP as part of the feed sequence. The hardware FSM (finite state machine) inside the chip expects exactly this two-step write:
// Write 0xAA to PLL0FEED
// Then write 0x55 to PLL0FEED
// They must be done in sequence, with no other writes or reads in between.
ALWAYS_INLINE static void pll_feed() {
  PLL0FEED = 0xAA;
  PLL0FEED = 0x55;
}

ALWAYS_INLINE static void sys_tick_init() {
  SYST_CSR = 0;                   // Disable SysTick
  SYST_RVR = (F_CPU / 1000) - 1;  // 1ms reload value
  SYST_CVR = 0;                   // Clear current value
  SYST_CSR = 5;                   // Enable SysTick, no interrupt, use CPU clock
}

/* Configure system clock: 12 MHz external crystal → 120 MHz CPU */
void clock_init() {
  // 1. Enable the main oscillator (external 12 MHz crystal)
  SCS |= SCS_OSCEN;
  for (volatile int i = 0; i < 10000; i++);  // Short delay
  while ((SCS & SCS_OSCSTAT) == 0);          // Wait for oscillator ready

  // 2. Select main oscillator as PLL0 source
  CLKSRCSEL = 0x01;

  // 3. Disable PLL0 before reconfiguration
  PLL0CON = 0x00;
  pll_feed();

  // 4. Configure PLL0 for M=20 (MSEL=19), N=1 (NSEL=0)
  PLL0CFG = (19 << 0) | (0 << 16);  // FCCO = 480 MHz
  pll_feed();

  // 5. Enable PLL0 (but not yet connected)
  PLL0CON = PLL0CON_PLLE;
  pll_feed();

  // 6. Wait for PLL0 to lock
  while ((PLL0STAT & PLL0STAT_PLOCK) == 0);

  // 7. Set CPU Clock Divider: CCLK = FCCO / (2 * (CCLKCFG + 1)) = 480 / 4 = 120 MHz
  CCLKCFG = 3;

  // 8. Set Flash access timing: 5 CPU clocks for 120 MHz
  FLASHCFG = (FLASHCFG & ~0xF) | 0x5;

  // 9. Connect PLL0
  PLL0CON = PLL0CON_PLLE | PLL0CON_PLLC;
  pll_feed();
}
