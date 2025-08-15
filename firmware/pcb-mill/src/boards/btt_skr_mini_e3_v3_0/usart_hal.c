#include <stdint.h>

#include "hal.h"
#include "board_hal.h"
#include "clock.h"
#include "build_config.h"
#include "usart_hal.h"

void usart_init_hal(
    usart_instance_t *usart,
    GPIO_TypeDef *gpio,
    uint32_t tx_pin,
    uint32_t rx_pin,
    uint32_t usart_enable_bit,
    IRQn_Type irq_no) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  // Enable clock to UART
  RCC->APBENR1 |= usart_enable_bit;

  // Configure PA2 and PA3 to alternate function
  GPIO_SET_MODE(gpio, tx_pin, MODER_ALT);  // Set mode to alternate function (AF) on PA2
  GPIO_SET_MODE(gpio, rx_pin, MODER_ALT);  // Set mode to alternate function (AF) on PA3

  // Configure PA2 and PA3 to alternate function AF1 (usart)
  gpio->AFR[0] &= ~((GPIO_AF_MSK << (tx_pin * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << (rx_pin * GPIO_AF_BIT_COUNT)));
  gpio->AFR[0] |= ((GPIO_AF1 << (tx_pin * GPIO_AF_BIT_COUNT)) | (GPIO_AF1 << (rx_pin * GPIO_AF_BIT_COUNT)));

  // Configure usart
  hal->CR1 &= ~USART_CR1_UE;                                          // Disable usart
  hal->BRR = usart->baud_rate;                                        // Set baud rate
  hal->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE;  // Enable tx and rx + rx interrupts (tx interrupts will be enabled when data sent)
  hal->CR3 = 0;                                                       // No half-duplex
  hal->CR1 |= USART_CR1_UE;                                           // Enable usart

  // Enable interrupts for usart
  NVIC_EnableIRQ(irq_no);
}

void usart_tx_enable_hal(usart_instance_t *usart) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  hal->CR1 |= USART_CR1_TXEIE_TXFNFIE;
}

void usart_tx_disable_hal(usart_instance_t *usart) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  hal->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
}

int16_t usart_read(usart_instance_t *usart) {
  // Default to no data available
  int16_t data = -1;

  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  // Disable receive data interrupt while getting rx data
  hal->CR1 &= ~(USART_CR1_RXNEIE_RXFNEIE);

  if (usart->rx_buffer_tail != usart->rx_buffer_head) {
    // Read from rx buffer
    data = usart->rx_buffer[usart->rx_buffer_tail++];

    if (usart->rx_buffer_tail >= usart->rx_buffer_size) {
      usart->rx_buffer_tail = 0;
    }

    usart->rx_count--;
  } else {
    usart->rx_count = 0;
  }

  // Enable receive data interrupt
  hal->CR1 |= USART_CR1_RXNEIE_RXFNEIE;

  // Will be -1 if no data was available
  return data;
}

void usart_send(usart_instance_t *usart, uint8_t b) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  // Disable transmit empty interrupt while sending
  hal->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE);

  // Add byte to TX buffer
  usart->tx_buffer[usart->tx_buffer_head++] = b;

  if (usart->tx_buffer_head >= usart->tx_buffer_size) {
    usart->tx_buffer_head = 0;
  }

  // Enable transmit empty interrupt
  hal->CR1 |= USART_CR1_TXEIE_TXFNFIE;
}

void usart_wait_data_sent(usart_instance_t *usart) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  // // Make sure TX empty status enabled
  // hal->CR1 |= USART_CR1_TXEIE_TXFNFIE;

  // Wait until transmit data register empty
  while (!(hal->ISR & USART_ISR_TXE_TXFNF));

  // Wait transmit complete
  while (!(hal->ISR & USART_ISR_TC));
}

void usart_irq_handler(usart_instance_t *usart) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  // Is the TX buffer empty?
  if ((hal->ISR & USART_ISR_TXE_TXFNF)) {
    if (usart->tx_buffer_tail != usart->tx_buffer_head) {
      uint8_t data = usart->tx_buffer[usart->tx_buffer_tail++];

      if (usart->tx_buffer_tail >= usart->tx_buffer_size) {
        usart->tx_buffer_tail = 0;
      }

      hal->TDR = data;
    } else {
      // Disable TX empty interrupt until more data sent
      hal->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE);
    }
  }

  // Is RX buffer not empty?
  if (hal->ISR & USART_ISR_RXNE_RXFNE) {
    usart->rx_buffer[usart->rx_buffer_head++] = hal->RDR;
    usart->rx_count++;

    if (usart->rx_buffer_head >= usart->rx_buffer_size) {
      usart->rx_buffer_head = 0;
    }
  }
}

bool usart_wait_for_count(usart_instance_t *usart, uint8_t count, uint32_t max_wait_ms) {
  while (usart->rx_count < count && max_wait_ms-- > 0) {
    delay_ms(1);
  }

  // Was successful if rx count is at least desired count
  return usart->rx_count >= count;
}

void usart_clear_buffers(usart_instance_t *usart) {
  // Cast specific hal type
  USART_TypeDef *hal = (USART_TypeDef *)usart->hal;

  // Disable interrupt while clearing
  hal->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE | USART_CR1_RXNEIE_RXFNEIE);

  // Reset heads, tails and count
  usart->tx_buffer_head = usart->tx_buffer_tail = 0;
  usart->rx_buffer_head = usart->rx_buffer_tail = 0;
  usart->rx_count = 0;

  // Restore receive interrupt (TX interrupt will be enabled when data is next sent)
  hal->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
}