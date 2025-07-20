
#include "clock.h"

#include <stdint.h>

#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

uint32_t systick_global = 0;

#define HAL_TICK_FREQ_1KHZ 1U

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

#define _BIT_SHIFT(IRQn) (((((uint32_t)(int32_t)(IRQn))) & 0x03UL) * 8UL)
#define _SHP_IDX(IRQn) ((((((uint32_t)(int32_t)(IRQn)) & 0x0FUL) - 8UL) >> 2UL))
#define _IP_IDX(IRQn) ((((uint32_t)(int32_t)(IRQn)) >> 2UL))

static inline void nvic_set_priority(IRQn_Type IRQn, uint32_t priority) {
  if ((int32_t)(IRQn) >= 0) {
    NVIC->IP[_IP_IDX(IRQn)] = ((uint32_t)(NVIC->IP[_IP_IDX(IRQn)] & ~(0xFFUL << _BIT_SHIFT(IRQn))) |
                               (((priority << (8U - __NVIC_PRIO_BITS)) & (uint32_t)0xFFUL) << _BIT_SHIFT(IRQn)));
  } else {
    SCB->SHP[_SHP_IDX(IRQn)] = ((uint32_t)(SCB->SHP[_SHP_IDX(IRQn)] & ~(0xFFUL << _BIT_SHIFT(IRQn))) |
                                (((priority << (8U - __NVIC_PRIO_BITS)) & (uint32_t)0xFFUL) << _BIT_SHIFT(IRQn)));
  }
}

static inline void sys_tick_config(uint32_t ticks) {
  if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk) {
    return;  // Invalid reload value
  }

  SysTick->LOAD = (uint32_t)(ticks - 1UL);                          /* set reload register */
  nvic_set_priority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */
  SysTick->VAL = 0UL;                                               /* Load the SysTick Counter Value */
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk; /* Enable SysTick IRQ and SysTick Timer */
}

void init_clock() {
  // Init Sys Tick
  sys_tick_config(F_SYS_CLOCK / (1000U / (uint32_t)HAL_TICK_FREQ_1KHZ));

  RCC->CR |= RCC_CR_HSEON;             // Enable HSE
  while (!(RCC->CR & RCC_CR_HSERDY));  // Wait for HSE ready

  RCC->CR &= ~RCC_CR_PLLON;         // Disable PLL
  while (RCC->CR & RCC_CR_PLLRDY);  // Wait for PLL to unlock

  RCC->PLLCFGR =
      (0b11 << 0) |   // PLLSRC = HSE
      (0b000 << 4) |  // PLLM = /1
      (16 << 8) |     // PLLN = 16
      (0b00 << 25) |  // PLLR = /2
      (1 << 28);      // PLLREN = enable PLLR output

  RCC->CR |= RCC_CR_PLLON;             // Enable PLL
  while (!(RCC->CR & RCC_CR_PLLRDY));  // Wait for PLL ready

  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_MSK) | (1 << FLASH_ACR_LATENCY_POS);  // 1 wait state

  RCC->CFGR &= ~(0x3 << 0);  // Clear SW bits
  RCC->CFGR |= (0x2 << 0);   // Set SYSCLK = PLL
  RCC->CFGR &= ~(0xF << 4);  // AHB = /1
  RCC->CFGR &= ~(0x7 << 8);  // APB1 = /1

  while ((RCC->CFGR & (0x3 << 3)) != (0x2 << 3));  // Wait for switch to PLL
}

uint32_t get_systick() {
  uint32_t systick_local = 0;
  disable_irq();
  systick_local = systick_global;
  enable_irq();
  return systick_local;
}

void SysTick_Handler() {
  // The global system tick, increments at 1Kz (every 1ms)
  systick_global++;
}