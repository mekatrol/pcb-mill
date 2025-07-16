#include <stdint.h>

#include "memory_map.h"

#define FLASH_ACR_LATENCY_POS (0U)
#define FLASH_ACR_LATENCY_MSK (0x3UL << FLASH_ACR_LATENCY_POS)

// Bit masks
#define RCC_CR_HSEON (1 << 16)
#define RCC_CR_HSERDY (1 << 17)
#define RCC_CR_PLLON (1 << 24)
#define RCC_CR_PLLRDY (1 << 25)
#define RCC_CFGR_SW_PLL (0x3)
#define RCC_CFGR_SWS_PLL ((RCC_CFGR_SW_PLL) << 2)
#define RCC_CFGR_SWS_PLL_MSK ((RCC_CFGR_SW_PLL) << 2)

void init_clock()
{
    RCC->CR |= RCC_CR_HSEON; // Enable HSE
    while (!(RCC->CR & RCC_CR_HSERDY))
        ; // Wait for HSE ready

    RCC->CR &= ~RCC_CR_PLLON; // Disable PLL
    while (RCC->CR & RCC_CR_PLLRDY)
        ; // Wait for PLL to unlock

    RCC->PLLCFGR =
        (0b11 << 0) |  // PLLSRC = HSE
        (0b000 << 4) | // PLLM = /1
        (16 << 8) |    // PLLN = 16
        (0b00 << 25) | // PLLR = /2
        (1 << 28);     // PLLREN = enable PLLR output

    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    while (!(RCC->CR & RCC_CR_PLLRDY))
        ; // Wait for PLL ready

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_MSK) | (1 << FLASH_ACR_LATENCY_POS); // 1 wait state

    RCC->CFGR &= ~(0x3 << 0); // Clear SW bits
    RCC->CFGR |= (0x2 << 0);  // Set SYSCLK = PLL
    RCC->CFGR &= ~(0xF << 4); // AHB = /1
    RCC->CFGR &= ~(0x7 << 8); // APB1 = /1

    while ((RCC->CFGR & (0x3 << 3)) != (0x2 << 3))
        ; // Wait for switch to PLL
}