#include <stdint.h>

#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_OFFSET 0x00020000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + AHB1PERIPH_OFFSET)
#define GPIOD_BASE (AHB1PERIPH_BASE + 0x0C00UL)
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800UL)

#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define GPIOD_MODER (*(volatile uint32_t *)(GPIOD_BASE + 0x00))
#define GPIOD_ODR (*(volatile uint32_t *)(GPIOD_BASE + 0x14))

void delay(void)
{
    for (volatile int i = 0; i < 500000; ++i)
        ;
}

int main(void)
{
    RCC_AHB1ENR |= (1 << 3); // Enable GPIOD clock
    GPIOD_MODER &= ~(0xFF << (2 * 12));
    GPIOD_MODER |= (0x55 << (2 * 12)); // Set PD12-PD15 to output

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            GPIOD_ODR = (1 << (12 + i));
            delay();
        }
    }
}