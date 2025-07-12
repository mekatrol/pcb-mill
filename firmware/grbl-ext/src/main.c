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

#define RCC_APB1ENR_TIM6EN (1 << 4)
#define TIM_CR1_CEN (1 << 0)
#define TIM_CR1_OPM (1 << 3) // One-pulse mode
#define TIM_EGR_UG (1 << 0)
#define TIM_SR_UIF (1 << 0)

#define TIM_DIER_UIE (1 << 0)

void timer6_init(uint32_t psc, uint32_t arr, uint8_t enable_interrupt)
{
    // Enable TIM6 clock
    RCC->APBENR1 |= RCC_APB1ENR_TIM6EN;

    // Configure prescaler and auto-reload
    TIM6->PSC = psc;
    TIM6->ARR = arr;
    TIM6->CNT = 0;

    // Generate update event to apply registers
    TIM6->EGR = TIM_EGR_UG;

    // Enable TIM6
    TIM6->CR1 |= TIM_CR1_CEN;

    // Disable 1 time mode
    TIM6->CR1 &= ~TIM_CR1_OPM;

    if (!enable_interrupt)
    {
        return;
    }

    // Enable update event interrupt
    TIM6->SR &= ~TIM_SR_UIF;    // Clear
    TIM6->DIER |= TIM_DIER_UIE; // Enable

    // Enable TIM6 interrupt in NVIC
    NVIC->ISER[0] = (1 << 17); // TIM6 interrupt is IRQ #17
}

// Interrupt handler — must match symbol in vector table
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)
    {
        TIM6->SR &= ~TIM_SR_UIF;      // clear interrupt flag
        GPIOD->ODR ^= STATUS_LED_PIN; // toggle LED on PD8
    }
}

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

    // Enable GPIOC peripheral
    RCC->IOPENR |= IOPENR_PORTC_ENABLE;                          // Enable PORTC in IOPENR
    GPIOC->MODER &= ~(MODER_MSK << (MODE_06 * MODER_BIT_COUNT)); // Clear PC6 mode bits
    GPIOC->MODER |= (MODER_OUT << (MODE_06 * MODER_BIT_COUNT));  // Set PC6 as output

    // Enable GPIOD peripheral
    RCC->IOPENR |= IOPENR_PORTD_ENABLE;                          // Enable PORTC in IOPENR
    GPIOD->MODER &= ~(MODER_MSK << (MODE_08 * MODER_BIT_COUNT)); // Clear PD8 mode bits
    GPIOD->MODER |= (MODER_OUT << (MODE_08 * MODER_BIT_COUNT));  // Set PD8 as output

    // timer6_init();

    // Set TIM6 to tick every 100 ms:
    //  64 MHz / 64000 = 1000 Hz → 1 ms per tick
    //  Reload is 100ms, so interrupt fires every 100ms
    timer6_init(64000 - 1, 100 - 1, 1);

    // Enable global interrupts
    __asm volatile("cpsie i"); // Enable interrupts globally

    while (1)
    {
        GPIOC->ODR ^= FAN_0_PIN; // Toggle PC6
        delay_ms(100);
    }
}
