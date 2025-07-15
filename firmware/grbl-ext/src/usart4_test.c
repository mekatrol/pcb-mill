#include <stdint.h>

#include "clock.h"
#include "gpio.h"
#include "memory_map.h"
#include "register_bits.h"
#include "uart.h"

int usart2_test(void)
{
    // 1. Enable GPIOA and USART2 clocks
    RCC->IOPENR |= IOPENR_PORTA_ENABLE;
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    // Enable GPIOD clock
    RCC->IOPENR |= IOPENR_PORTD_ENABLE; // Enable PORTD in IOPENR

    // Enable GPIOD peripheral
    GPIOD->MODER &= ~(MODER_MSK << (MODE_08 * MODER_BIT_COUNT)); // Clear PD8 mode bits
    GPIOD->MODER |= (MODER_OUT << (MODE_08 * MODER_BIT_COUNT));  // Set PD8 as output

    // 2. Configure PA2 and PA3 as alternate function AF1 (USART2)
    GPIOA->MODER &= ~((0b11 << (PIN2 * 2)) | (0b11 << (PIN3 * 2))); // Clear mode bits
    GPIOA->MODER |= ((GPIO_MODE_ALT << (PIN2 * 2)) | (GPIO_MODE_ALT << (PIN3 * 2)));

    GPIOA->AFRL &= ~((0xF << (PIN2 * 4)) | (0xF << (PIN3 * 4))); // Clear AF bits
    GPIOA->AFRL |= ((GPIO_AF1 << (PIN2 * 4)) | (GPIO_AF1 << (PIN3 * 4)));

    // 3. Configure USART2
    USART2->CR1 &= ~USART_CR1_UE;              // Disable USART
    USART2->BRR = USART_BRR_VALUE;             // Set baud rate
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE; // Enable transmitter and receiver
    USART2->CR1 |= USART_CR1_UE;               // Enable USART

    uint8_t c = 'A';
    while (1)
    {
        GPIOD->ODR ^= STATUS_LED_PIN; // Toggle PD8

        uint8_t byte_to_send = c;

        // Wait until transmit data register empty
        while (!(USART2->ISR & USART_ISR_TXE_TXFNF))
            ;

        USART2->TDR = byte_to_send;

        // Wait until data received
        while (!(USART2->ISR & USART_ISR_RXNE_RXFNE))
            ;

        c = USART2->RDR;
    }
}
