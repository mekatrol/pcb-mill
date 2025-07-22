#include <stdint.h>

#include "../../grbl/config.h"
#include "clock.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

void serial_init_hal() {
  // Enable clock to UART2
  RCC->APBENR1 |= RCC_APBENR1_USART2EN;

  // Configure PA2 and PA3 to alternate function
  GPIO_SET_MODE(GPIOA, BIT_02_POS, MODER_ALT);  // Set mode to alternate function (AF) on PA2
  GPIO_SET_MODE(GPIOA, BIT_03_POS, MODER_ALT);  // Set mode to alternate function (AF) on PA3

  // Configure PA2 and PA3 to alternate function AF1 (USART2)
  GPIOA->AFRL &= ~((GPIO_AF_MSK << (BIT_02_POS * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << (BIT_03_POS * GPIO_AF_BIT_COUNT)));
  GPIOA->AFRL |= ((GPIO_AF1 << (BIT_02_POS * GPIO_AF_BIT_COUNT)) | (GPIO_AF1 << (BIT_03_POS * GPIO_AF_BIT_COUNT)));

  // Configure USART2
  USART2->CR1 &= ~USART_CR1_UE;                                          // Disable USART
  USART2->BRR = USART_BRR(F_SYS_CLOCK, BAUD_RATE);                       // Set baud rate
  USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE;  // Enable transmitter and receiver
  USART2->CR3 = 0;                                                       // No half-duplex
  USART2->CR1 |= USART_CR1_UE;                                           // Enable USART

  ENABLE_IRQ(USART2_LPUART2_IRQn);
}

void serial_tx_enable_hal() {
  USART2->CR1 |= USART_CR1_TXEIE_TXFNFIE;
}

void serial_tx_disable_hal() {
  USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
}

void USART2_IRQHandler(void) {
  // Is the TX buffer empty?
  if ((USART2->ISR & USART_ISR_TXE_TXFNF)) {
    uint16_t data = serial_data_can_send();

    if (data != SERIAL_NO_DATA) {
      USART2->TDR = data;
    }
  }

  // RXNE: Receive Data Register Not Empty
  if ((USART2->ISR & USART_ISR_RXNE_RXFNE) && (USART2->CR1 & USART_CR1_RXNEIE_RXFNEIE)) {
    uint8_t data = USART2->RDR;
    serial_data_received(data);
  }
}

void USART3_4_LPUART1_IRQHandler(void) {
  if (USART4->ISR & USART_ISR_RXNE_RXFNE) {
    // RXFIFO not empty
    uint8_t b = USART4->RDR;
    (void)b;
  }
}