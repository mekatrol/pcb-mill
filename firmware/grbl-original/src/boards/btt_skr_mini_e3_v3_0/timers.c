#include "timers.h"

#include "../../grbl/hal.h"
#include "clock.h"
#include "gpio.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

extern volatile uint32_t systick_global;

void delay_us(uint32_t us) {
  uint32_t start = TIM6->CNT;  // TIM6->CNT increments every µs
  while ((uint32_t)(TIM6->CNT - start) < us);
}

void delay_ms(uint32_t ms) {
  uint32_t start = systick_global;  // systick_global increments every ms
  while ((uint32_t)(systick_global - start) < ms);
}

void timer_init(TIM_TypeDef *TIMx, uint32_t prescaler, uint32_t ticks, bool enable_interrupt, uint8_t irq_number,
                volatile uint32_t *rcc_enr, uint32_t rcc_timer_en) {
  // Enable timer  clock
  *rcc_enr |= rcc_timer_en;

  // Configure prescaler and auto-reload ticks
  TIMx->PSC = prescaler - 1;
  TIMx->ARR = ticks - 1;
  TIMx->CNT = 0;

  // Generate update event to apply registers
  TIMx->EGR = TIM_EGR_UG;

  // Enable TIMx
  TIMx->CR1 |= TIM_CR1_CEN;

  // Disable one time mode
  TIMx->CR1 &= ~TIM_CR1_OPM;

  if (!enable_interrupt) {
    return;
  }

  // Enable update event interrupt
  TIMx->SR &= ~TIM_SR_UIF;     // Clear
  TIMx->DIER |= TIM_DIER_UIE;  // Enable

  // Enable TIMx interrupt in NVIC
  ENABLE_IRQ(irq_number);
}

void timer6_init() {
  // Set TIM6 to tick every interval ms:
  //  64 MHz / 64 = 1 MHz → 1 µs prescaled
  timer_init(TIM6, 64, 0, false, TIM6_DAC_LPTIM1_IRQn, &RCC->APBENR1, RCC_APBENR1_TIM6EN);
}

void timer7_init(uint32_t interval, bool enable_interrupt) {
  // Set TIM7 to tick every interval ms:
  //  64 MHz / 64000 = 1000 Hz → 1 ms per tick
  //  ARR is interval ms, so interrupt fires every interval ms
  timer_init(TIM7, 64000, interval, enable_interrupt, TIM7_LPTIM2_IRQn, &RCC->APBENR1, RCC_APBENR1_TIM7EN);
}

void timer14_init() {
  // Set TIM14 to tick every interval ms:
  //  64 MHz / 64 = 1 MHz → 1 µs prescaled
  //  10 ticks = 10µs per tick (100kHz)
  timer_init(TIM14, 64, 10, true, TIM14_IRQn, &RCC->APBENR2, RCC_APBENR2_TIM14EN);
}

void set_timer_interval(TIM_TypeDef *TIMx, uint32_t interval) {
  // Disable TIMx update interrupt (optional, if enabled)
  TIMx->DIER &= ~(1 << 0);  // Clear UIE (Update Interrupt Enable)

  // Set timer interval
  TIMx->ARR = interval <= 1 ? 1 : interval - 1;

  // Force update event to reload the ARR immediately
  TIMx->EGR = 1;  // UG bit set

  // Re-enable update interrupt
  TIMx->DIER |= (1 << 0);
}

void set_timer7_interval(uint32_t interval) { set_timer_interval(TIM7, interval); }

void TIM6_DAC_IRQHandler(void) {
  if (TIM6->SR & TIM_SR_UIF) {
    TIM6->SR &= ~TIM_SR_UIF;  // Clear interrupt flag
  }
}

void TIM7_IRQHandler(void) {
  if (TIM7->SR & TIM_SR_UIF) {
    TIM7->SR &= ~TIM_SR_UIF;  // Clear interrupt flag
    GPIOD->ODR ^= BIT_08;     // Toggle PD8
  }
}

void TIM14_IRQHandler(void) {
  if (TIM14->SR & TIM_SR_UIF) {
    TIM14->SR &= ~TIM_SR_UIF;  // Clear interrupt flag
    stepper_interrupt();
  }
}