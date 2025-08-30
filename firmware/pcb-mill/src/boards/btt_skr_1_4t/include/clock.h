#ifndef __CLOCK_H__
#define __CLOCK_H__

#define SYST_CSR (*(volatile uint32_t *)0xE000E010)  // Control and status
#define SYST_RVR (*(volatile uint32_t *)0xE000E014)  // Reload value
#define SYST_CVR (*(volatile uint32_t *)0xE000E018)  // Current value

/* Clock control registers */
#define SCS (*(volatile uint32_t *)0x400FC1A0)
#define CLKSRCSEL (*(volatile uint32_t *)0x400FC10C)
#define PLL0CON (*(volatile uint32_t *)0x400FC080)
#define PLL0CFG (*(volatile uint32_t *)0x400FC084)
#define PLL0STAT (*(volatile uint32_t *)0x400FC088)
#define PLL0FEED (*(volatile uint32_t *)0x400FC08C)
#define CCLKCFG (*(volatile uint32_t *)0x400FC104)
#define FLASHCFG (*(volatile uint32_t *)0x400FC000)

/* Bit masks */
#define SCS_OSCEN (1 << 5)
#define SCS_OSCSTAT (1 << 6)
#define PLL0STAT_PLOCK (1 << 26)
#define PLL0CON_PLLE (1 << 0)
#define PLL0CON_PLLC (1 << 1)
#define F_CPU 120000000  // 120 MHz external

void clock_init();

#endif  // __CLOCK_H__