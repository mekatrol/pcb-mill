#include <stdint.h>

#include "clock.h"
#include "gpio.h"
#include "memory_map.h"
#include "register_bits.h"
#include "uart.h"

void usart2_init()
{
    // Enable clock to UART2
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    // Configure PA2 and PA3 as alternate function AF1 (USART2)
    GPIOA->MODER &= ~((MODER_MSK << (BIT_02 * MODER_BIT_COUNT)) | (MODER_MSK << (BIT_03 * MODER_BIT_COUNT)));
    GPIOA->MODER |= ((MODER_ALT << (BIT_02 * MODER_BIT_COUNT)) | (MODER_ALT << (BIT_03 * MODER_BIT_COUNT)));

    GPIOA->AFRL &= ~((GPIO_AF_MSK << (BIT_02 * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << (BIT_03 * GPIO_AF_BIT_COUNT))); // Clear AF bits
    GPIOA->AFRL |= ((GPIO_AF1 << (BIT_02 * GPIO_AF_BIT_COUNT)) | (GPIO_AF1 << (BIT_03 * GPIO_AF_BIT_COUNT)));

    // Configure USART2
    USART2->CR1 &= ~USART_CR1_UE;              // Disable USART
    USART2->BRR = USART_BRR_VALUE;             // Set baud rate
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE; // Enable transmitter and receiver
    USART2->CR1 |= USART_CR1_UE;               // Enable USART
}

void usart2_test(void)
{
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
