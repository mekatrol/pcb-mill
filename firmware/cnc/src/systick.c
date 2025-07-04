#include <stdint.h>
#include "systick.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010) // Control and status
#define SYST_RVR (*(volatile uint32_t *)0xE000E014) // Reload value
#define SYST_CVR (*(volatile uint32_t *)0xE000E018) // Current value

#define F_CPU 120000000 // 120 MHz

void systick_init(void)
{
    SYST_CSR = 0;                  // Disable SysTick
    SYST_RVR = (F_CPU / 1000) - 1; // 1ms reload value
    SYST_CVR = 0;                  // Clear current value
    SYST_CSR = 5;                  // Enable SysTick, no interrupt, use CPU clock
}

void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        while ((SYST_CSR & (1 << 16)) == 0)
            ; // Wait for COUNTFLAG
    }
}
