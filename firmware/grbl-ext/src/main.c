#include <stdint.h>

#include "memory_map.h"

// Bit masks
#define FAN_0_PIN (1 << 6)
#define STATUS_LED_PIN (1 << 8)
#define RCC_CR_HSEON (1 << 16)
#define RCC_CR_HSERDY (1 << 17)
#define RCC_CR_PLLON (1 << 24)
#define RCC_CR_PLLRDY (1 << 25)
#define RCC_CFGR_SW_PLL (0x3)
#define RCC_CFGR_SWS_PLL ((RCC_CFGR_SW_PLL) << 2)
#define RCC_CFGR_SWS_PLL_MSK ((RCC_CFGR_SW_PLL) << 2)

/*
 * MODER = Port mode register
 */
#define MODER_BIT_COUNT 0x02UL // 2 bits per MODER port configuration
#define MODER_MSK 0x03UL
#define MODER_INP 0x00UL
#define MODER_OUT 0x01UL
#define MODER_ALT 0x02UL
#define MODER_ANA 0x03UL

#define MODE_00 0x00
#define MODE_01 0x01
#define MODE_02 0x02
#define MODE_03 0x03
#define MODE_04 0x04
#define MODE_05 0x05
#define MODE_06 0x06
#define MODE_07 0x07
#define MODE_08 0x08
#define MODE_09 0x09
#define MODE_10 0x10
#define MODE_11 0x11
#define MODE_12 0x12
#define MODE_13 0x13
#define MODE_14 0x14
#define MODE_15 0x15

#define IOPENR_PORTA_ENABLE (1 << 0)
#define IOPENR_PORTB_ENABLE (1 << 1)
#define IOPENR_PORTC_ENABLE (1 << 2)
#define IOPENR_PORTD_ENABLE (1 << 3)
#define IOPENR_PORTE_ENABLE (1 << 4)
#define IOPENR_PORTF_ENABLE (1 << 5)

#define FLASH_ACR_LATENCY_Pos (0U)
#define FLASH_ACR_LATENCY_Msk (0x3UL << FLASH_ACR_LATENCY_Pos)

#define RCC_APB1ENR (*(volatile uint32_t *)0x40021038)

#define TIM6_CR1 (*(volatile uint32_t *)(TIM6_BASE + 0x00))
#define TIM6_SR (*(volatile uint32_t *)(TIM6_BASE + 0x10))
#define TIM6_CNT (*(volatile uint32_t *)(TIM6_BASE + 0x24))
#define TIM6_PSC (*(volatile uint32_t *)(TIM6_BASE + 0x28))
#define TIM6_ARR (*(volatile uint32_t *)(TIM6_BASE + 0x2C))
#define TIM6_EGR (*(volatile uint32_t *)(TIM6_BASE + 0x14))

#define RCC_APB1ENR_TIM6EN (1 << 4)
#define TIM_CR1_CEN (1 << 0)
#define TIM_CR1_OPM (1 << 3) // One-pulse mode
#define TIM_EGR_UG (1 << 0)
#define TIM_SR_UIF (1 << 0)

// Call once to configure TIM6
void timer6_init(void)
{
    // Enable TIM6 peripheral clock
    RCC->APBENR1 |= RCC_APB1ENR_TIM6EN;

    // Set TIM6 to tick every 1 ms:
    // 64 MHz / 64000 = 1000 Hz → 1 ms per tick
    TIM6->PSC = 64000 - 1; // Prescaler
    TIM6->ARR = 0xFFFF;    // Max count
    TIM6->CNT = 0;
    TIM6->EGR = TIM_EGR_UG;  // Apply prescaler
    TIM6->CR1 = TIM_CR1_CEN; // Start timer in continuous mode
}

// Accurate ms delay using polling
void delay_ms(uint32_t ms)
{
    uint16_t start = TIM6_CNT;
    while (ms > 0)
    {
        uint16_t now = TIM6_CNT;
        if ((uint16_t)(now - start) >= 1)
        {
            start = now;
            --ms;
        }
    }
}

void init_clock(void)
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

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | (1 << FLASH_ACR_LATENCY_Pos); // 1 wait state

    RCC->CFGR &= ~(0x3 << 0); // Clear SW bits
    RCC->CFGR |= (0x2 << 0);  // Set SYSCLK = PLL
    RCC->CFGR &= ~(0xF << 4); // AHB = /1
    RCC->CFGR &= ~(0x7 << 8); // APB1 = /1

    while ((RCC->CFGR & (0x3 << 3)) != (0x2 << 3))
        ; // Wait for switch to PLL
}

int main(void)
{
    init_clock();
    timer6_init();

    // Enable GPIOC peripheral
    RCC->IOPENR |= IOPENR_PORTC_ENABLE;                          // Enable PORTC in IOPENR
    GPIOC->MODER &= ~(MODER_MSK << (MODE_06 * MODER_BIT_COUNT)); // Clear PC6 mode bits
    GPIOC->MODER |= (MODER_OUT << (MODE_06 * MODER_BIT_COUNT));  // Set PC6 as output

    // Enable GPIOD peripheral
    RCC->IOPENR |= IOPENR_PORTD_ENABLE;                          // Enable PORTC in IOPENR
    GPIOD->MODER &= ~(MODER_MSK << (MODE_08 * MODER_BIT_COUNT)); // Clear PD8 mode bits
    GPIOD->MODER |= (MODER_OUT << (MODE_08 * MODER_BIT_COUNT));  // Set PD8 as output

    uint32_t i = 0;
    while (1)
    {
        GPIOC->ODR ^= FAN_0_PIN; // Toggle PC6

        if (i % 5 == 0)
        {
            GPIOD->ODR ^= STATUS_LED_PIN; // Toggle PD8
        }
        delay_ms(100);
        i++;
    }
}
