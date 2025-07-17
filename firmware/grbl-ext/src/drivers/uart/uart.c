#include "clock.h"
#include "gpio.h"
#include "irq.h"
#include "register_bits.h"
#include "uart.h"

#include "clock.h"
#include "memory_map.h"

// RX buffer
#define RX_BUF_SIZE 64
volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;

void uart_send(USART_TypeDef *uart, uint8_t b)
{
    // Wait until transmit data register empty
    while (!(uart->ISR & USART_ISR_TXE_TXFNF))
        ;

    // Send byte
    uart->TDR = b;

    // Wait transmit complete (TC)
    while (!(uart->ISR & USART_ISR_TC))
        ;

    // Clear transmit complete (TC)
    uart->ICR |= USART_ISR_TC;
}

uint8_t uart_recv(USART_TypeDef *uart)
{
    if (uart->ISR & USART_ISR_RXNE_RXFNE)
    {
        uint8_t c = uart->RDR;
        return c;
    }

    // Zero indicates no data
    return 0;
}

void uart4_init()
{
    // Enable USART4 clocks
    RCC->APBENR1 |= RCC_APBENR1_USART4EN;

    // Confiure PC10 and PC11 to alternate function AF1 (USART4)
    GPIOC->MODER &= ~((MODER_MSK << (BIT_10 * MODER_BIT_COUNT)) | (MODER_MSK << (BIT_11 * MODER_BIT_COUNT)));
    GPIOC->MODER |= ((MODER_ALT << (BIT_10 * MODER_BIT_COUNT)) | (MODER_ALT << (BIT_11 * MODER_BIT_COUNT)));

    GPIOC->AFRH &= ~((GPIO_AF_MSK << ((BIT_10 - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << ((BIT_11 - 8) * GPIO_AF_BIT_COUNT)));
    GPIOC->AFRH |= ((GPIO_AF1 << ((BIT_10 - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF1 << ((BIT_11 - 8) * GPIO_AF_BIT_COUNT)));

    // GPIOC->OTYPER &= ~((1 << BIT_10) | (1 << BIT_11)); // Push-pull

    // // Set outputs to high speed
    // GPIOC->OSPEEDR &= ~((OSPEEDR_MSK << (BIT_10 * OSPEEDR_BIT_COUNT)) | (OSPEEDR_MSK << (BIT_11 * OSPEEDR_BIT_COUNT)));
    // GPIOC->OSPEEDR |= ((OSPEEDR_HIGH << (BIT_10 * OSPEEDR_BIT_COUNT)) | (OSPEEDR_HIGH << (BIT_11 * OSPEEDR_BIT_COUNT)));

    // GPIOC->PUPDR &= ~((3 << (BIT_10 * 2)) | (3 << (BIT_11 * 2)));
    // GPIOC->PUPDR |= (0 << (BIT_10 * 2)) | (1 << (BIT_11 * 2)); // No pull TX, pull-up RX

    // Configure USART4 (8-bit data, 1 stop bit)
    USART4->CR1 &= ~USART_CR1_UE;                                         // Disable USART
    USART4->BRR = USART_BRR(F_SYS_CLOCK, BAUD_RATE);                      // Set baud rate
    USART4->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE; // Enable transmitter and receiver
    USART4->CR3 = 0;                                                      // No half-duplex
    USART4->CR1 |= USART_CR1_UE;                                          // Enable USART

    // Enable USART4 interrupt in NVIC
    ENABLE_IRQ(USART3_4_LPUART1_IRQn);
}

void USART3_4_LPUART1_IRQHandler(void)
{
    if (USART4->ISR & USART_ISR_RXNE_RXFNE)
    {
        // RXFIFO not empty
        uint8_t b = USART4->RDR;
        rx_buf[rx_index++] = b;

        if (rx_index >= RX_BUF_SIZE)
        {
            rx_index = 0;
        }
    }
}

void uart2_init()
{
    // Enable clock to UART2
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    // Configure PA2 and PA3 to alternate function AF1 (USART2)
    GPIOA->MODER &= ~((MODER_MSK << (BIT_02 * MODER_BIT_COUNT)) | (MODER_MSK << (BIT_03 * MODER_BIT_COUNT)));
    GPIOA->MODER |= ((MODER_ALT << (BIT_02 * MODER_BIT_COUNT)) | (MODER_ALT << (BIT_03 * MODER_BIT_COUNT)));

    GPIOA->AFRL &= ~((GPIO_AF_MSK << (BIT_02 * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << (BIT_03 * GPIO_AF_BIT_COUNT))); // Clear AF bits
    GPIOA->AFRL |= ((GPIO_AF1 << (BIT_02 * GPIO_AF_BIT_COUNT)) | (GPIO_AF1 << (BIT_03 * GPIO_AF_BIT_COUNT)));

    // Configure USART2
    USART2->CR1 &= ~USART_CR1_UE;                    // Disable USART
    USART2->BRR = USART_BRR(F_SYS_CLOCK, BAUD_RATE); // Set baud rate
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE;       // Enable transmitter and receiver
    USART2->CR3 = 0;                                 // No half-duplex
    USART2->CR1 |= USART_CR1_UE;                     // Enable USART
}

inline bool uart_data_ready()
{
    return (USART2->ISR & USART_ISR_RXNE_RXFNE) != 0;
}

inline bool uart_can_send()
{
    return (USART2->ISR & USART_ISR_TXE_TXFNF) != 0;
}

void uart_putc(uint8_t b)
{
    uart_send(USART2, b);
}

uint8_t uart_getc()
{
    return uart_recv(USART2);
}

void uart4_send(uint8_t b)
{
    uart_send(USART4, b);
}

uint8_t uart4_recv()
{
    return uart_recv(USART4);
}
