#include "fans.h"

void set_fan_0_pwm(uint32_t frequency, uint32_t duty_cycle)
{
    // 0 <= duty_cyle <= 100
    if (duty_cycle > 100)
    {
        duty_cycle = 100;
    }

    uint32_t reload = frequency - 1;
    uint32_t duty_cycle_period = (uint32_t)((float)frequency * ((float)duty_cycle / 100.0f));

    TIM3->ARR = reload;             // Auto-reload (period)
    TIM3->CCR1 = duty_cycle_period; // Compare = % duty
}

void enable_fan_0(uint32_t frequency, uint32_t duty_cycle)
{
    uint32_t reload = frequency - 1;
    uint32_t duty_cycle_period = (uint32_t)((float)frequency * ((float)duty_cycle / 100.0f));

    RCC->IOPENR |= (1 << 2);  // GPIOC enable (bit 2)
    RCC->APBENR1 |= (1 << 1); // TIM3 enable (bit 1)

    // Set MODER[13:12] to 10 (alternate function) for PC6
    GPIOC->MODER &= ~(0x3 << (6 * 2));
    GPIOC->MODER |= (0x2 << (6 * 2));

    // Set AFRL[27:24] to 0001 (AF1) for PC6
    GPIOC->AFR[0] &= ~(0xF << (6 * 4));
    GPIOC->AFR[0] |= (0x1 << (6 * 4));

    // Timer base setup: 64 MHz / 64 = 1 MHz -> PWM period = 1 kHz (ARR=999)
    TIM3->PSC = 64 - 1; // Prescaler

    set_fan_0_pwm(frequency, duty_cycle);

    // Set PWM mode 1 on CH1, preload enable
    TIM3->CCMR1 &= ~0xFF;
    TIM3->CCMR1 |= (6 << 4) | (1 << 3); // OC1M = 110, OC1PE = 1

    // Enable CH1 output
    TIM3->CCER |= (1 << 0); // CC1E = 1

    // Enable auto-reload preload
    TIM3->CR1 |= (1 << 7); // ARPE = 1

    // Generate update event to latch values
    TIM3->EGR = 1;

    // Enable counter
    TIM3->CR1 |= (1 << 0); // CEN = 1
}