
#include "serial_buffer.h"
#include "hal.h"

int16_t serial_uart_get(serial_buffer_t *buffer) {
  // Default to no data available
  int16_t data = -1;

  // Disable receive data interrupt while getting rx data
  buffer->uart->CR1 &= ~(USART_CR1_RXNEIE_RXFNEIE);

  if (buffer->rx_buffer_tail != buffer->rx_buffer_head) {
    // Read from rx buffer
    data = buffer->rx_buffer[buffer->rx_buffer_tail++];

    if (buffer->rx_buffer_tail >= buffer->rx_buffer_size) {
      buffer->rx_buffer_tail = 0;
    }

    buffer->rx_count--;
  } else {
    buffer->rx_count = 0;
  }

  // Enable receive data interrupt
  buffer->uart->CR1 |= USART_CR1_RXNEIE_RXFNEIE;

  // Will be -1 if no data was available
  return data;
}

void serial_uart_send(serial_buffer_t *buffer, uint8_t b) {
  // Disable transmit empty interrupt while sending
  buffer->uart->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE);

  // Add byte to TX buffer
  buffer->tx_buffer[buffer->tx_buffer_head++] = b;

  if (buffer->tx_buffer_head >= buffer->tx_buffer_size) {
    buffer->tx_buffer_head = 0;
  }

  // Enable transmit empty interrupt
  buffer->uart->CR1 |= USART_CR1_TXEIE_TXFNFIE;
}

void serial_wait_data_sent(serial_buffer_t *buffer) {
  // Wait until transmit data register empty
  while (!(buffer->uart->ISR & USART_ISR_TXE_TXFNF));

  // Wait transmit complete
  while (!(buffer->uart->ISR & USART_ISR_TC));
}

void serial_uart_irq_handler(serial_buffer_t *buffer) {
  // Is the TX buffer empty?
  if ((buffer->uart->ISR & USART_ISR_TXE_TXFNF)) {
    if (buffer->tx_buffer_tail != buffer->tx_buffer_head) {
      uint8_t data = buffer->tx_buffer[buffer->tx_buffer_tail++];

      if (buffer->tx_buffer_tail >= buffer->tx_buffer_size) {
        buffer->tx_buffer_tail = 0;
      }

      buffer->uart->TDR = data;
    } else {
      // Disable TX empty interrupt until more data sent
      buffer->uart->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE);
    }
  }

  // Is RX FIFO not empty?
  if (buffer->uart->ISR & USART_ISR_RXNE_RXFNE) {
    buffer->rx_buffer[buffer->rx_buffer_head++] = buffer->uart->RDR;
    buffer->rx_count++;

    if (buffer->rx_buffer_head >= buffer->rx_buffer_size) {
      buffer->rx_buffer_head = 0;
    }
  }
}

bool serial_wait_for_count(serial_buffer_t *buffer, uint8_t count, uint32_t max_wait_ms) {
  while (buffer->rx_count < count && max_wait_ms-- > 0) {
    delay_ms(1);
  }

  // Was successful if rx count is at least desired count
  return buffer->rx_count >= count;
}

void serial_clear_buffers(serial_buffer_t *buffer) {
  // Disable interrupt while clearing
  buffer->uart->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE | USART_CR1_RXNEIE_RXFNEIE);

  // Reset heads, tails and count
  buffer->tx_buffer_head = buffer->tx_buffer_tail = 0;
  buffer->rx_buffer_head = buffer->rx_buffer_tail = 0;
  buffer->rx_count = 0;

  // Restore receive interrupt (TX interrupt will be enabled when data is next sent)
  buffer->uart->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
}