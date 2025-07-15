#include "clock.h"
#include "gpio.h"
#include "timers.h"
#include "memory_map.h"
#include "irq.h"

// Accurate ms delay using polling
void delay_ms(uint32_t ms)
{
    uint16_t start = TIM6->CNT;
    while (ms > 0)
    {
        uint16_t now = TIM6->CNT;
        if ((uint16_t)(now - start) >= 1)
        {
            start = now;
            --ms;
        }
    }
}

void timer_init(
    TIM_TypeDef *TIMx,
    uint32_t psc, uint32_t arr,
    bool enable_interrupt,
    uint8_t irq_number,
    volatile uint32_t *rcc_enr, uint32_t rcc_timer_en)
{
    // Enable timer  clock
    *rcc_enr |= rcc_timer_en;

    // Configure prescaler and auto-reload
    TIMx->PSC = psc;
    TIMx->ARR = arr;
    TIMx->CNT = 0;

    // Generate update event to apply registers
    TIMx->EGR = TIM_EGR_UG;

    // Enable TIMx
    TIMx->CR1 |= TIM_CR1_CEN;

    // Disable one time mode
    TIMx->CR1 &= ~TIM_CR1_OPM;

    if (!enable_interrupt)
    {
        return;
    }

    // Enable update event interrupt
    TIMx->SR &= ~TIM_SR_UIF;    // Clear
    TIMx->DIER |= TIM_DIER_UIE; // Enable

    // Enable TIMx interrupt in NVIC
    ENABLE_IRQ(irq_number);
}

void timer6_init(uint32_t interval, bool enable_interrupt)
{
    // Set TIM6 to tick every interval ms:
    //  64 MHz / 64000 = 1000 Hz → 1 ms per tick
    //  ARR is interval ms, so interrupt fires every interval ms
    timer_init(TIM6, 64000 - 1, interval - 1, enable_interrupt, TIM6_IRQn, &RCC->APBENR1, RCC_APB1ENR_TIM6EN);
}

void timer7_init(uint32_t interval, bool enable_interrupt)
{
    // Set TIM7 to tick every interval ms:
    //  64 MHz / 64000 = 1000 Hz → 1 ms per tick
    //  ARR is interval ms, so interrupt fires every interval ms
    timer_init(TIM7, 64000 - 1, interval - 1, enable_interrupt, TIM7_IRQn, &RCC->APBENR1, RCC_APB1ENR_TIM7EN);
}

void set_timer_interval(TIM_TypeDef *TIMx, uint32_t interval)
{
    // Disable TIMx update interrupt (optional, if enabled)
    TIMx->DIER &= ~(1 << 0); // Clear UIE (Update Interrupt Enable)

    // Set timer interval
    TIMx->ARR = interval - 1;

    // Force update event to reload the ARR immediately
    TIMx->EGR = 1; // UG bit set

    // Re-enable update interrupt
    TIMx->DIER |= (1 << 0);
}

void set_timer6_interval(uint32_t interval)
{
    set_timer_interval(TIM6, interval);
}

void set_timer7_interval(uint32_t interval)
{
    set_timer_interval(TIM7, interval);
}

void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)
    {
        TIM6->SR &= ~TIM_SR_UIF;      // clear interrupt flag
        GPIOD->ODR ^= STATUS_LED_PIN; // Toggle PD8
    }
}

void TIM7_IRQHandler(void)
{
    if (TIM7->SR & TIM_SR_UIF)
    {
        TIM7->SR &= ~TIM_SR_UIF; // clear interrupt flag
        GPIOC->ODR ^= FAN_0_PIN; // Toggle PC6
    }
}