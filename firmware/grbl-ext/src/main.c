#include <stdint.h>

#define RCC_BASE 0x40021000
#define RCC_IOPENR (*(volatile uint32_t *)(RCC_BASE + 0x34))

#define GPIOC_BASE 0x50000800
#define GPIOC_MODER (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR (*(volatile uint32_t *)(GPIOC_BASE + 0x14))

#define LED_PIN (1 << 6)

typedef volatile uint32_t vuint32_t;

void delay_ms(uint32_t ms)
{
    // For STM32G0B1 running at 64 MHz default HSI, approximate loop
    // This is a rough software delay loop
    // 1 ms ~ 64000 cycles (assuming no wait states, for busy wait)
    for (uint32_t i = 0; i < ms * 6400; ++i)
    {
        __asm volatile("nop");
    }
}

int main(void)
{
    RCC_IOPENR |= (1 << 2);         // Enable clock for GPIOC (bit 2)
    GPIOC_MODER &= ~(3 << (6 * 2)); // Clear mode bits for PC6
    GPIOC_MODER |= (1 << (6 * 2));  // Set PC6 as output (01)

    while (1)
    {
        GPIOC_ODR ^= LED_PIN; // Toggle PC6
        delay_ms(500);        // Delay ~500ms
    }
}
