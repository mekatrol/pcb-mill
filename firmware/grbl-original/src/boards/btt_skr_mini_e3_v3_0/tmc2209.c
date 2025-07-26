#include "tmc2209.h"

#include "clock.h"
#include "gpio.h"
#include "irq.h"
#include "log.h"
#include "memory_map.h"
#include "register_bits.h"
#include "timers.h"

typedef enum {
  STATE_INIT_X = 0,
  STATE_INIT_Y = 1,
  STATE_INIT_Z = 2,
  STATE_INIT_E = 3,
  STATE_INIT_DONE = 4

} TMC2209_Initialise_State;

// TMC2209 uart baud rate
#define TMC2209_BAUD_RATE 115200

// TMC2209 RX buffer
#define TMC2209_RX_BUFFER_SIZE 16  // Characters are echoed (so rx buffer size should be twice maximum message length)
uint8_t tmc2209_rx_buffer[TMC2209_RX_BUFFER_SIZE];
uint8_t tmc2209_rx_buffer_head = 0;
volatile uint8_t tmc2209_rx_buffer_tail = 0;

// TMC2209 TX buffer
#define TMC2209_TX_BUFFER_SIZE 8
uint8_t tmc2209_tx_buffer[TMC2209_TX_BUFFER_SIZE];
uint8_t tmc2209_tx_buffer_head = 0;
volatile uint8_t tmc2209_tx_buffer_tail = 0;

// Number of bytes currently queued in rx buffer
volatile uint8_t tmc2209_rx_count = 0;

void tmc2209_uart4_init() {
  // Enable USART4 clocks
  RCC->APBENR1 |= RCC_APBENR1_USART4EN;

  // Confiure PC10 and PC11 to alternate function AF1 (USART4)
  GPIOC->MODER &= ~((MODER_MSK << (BIT_10_POS * MODER_BIT_COUNT)) | (MODER_MSK << (BIT_11_POS * MODER_BIT_COUNT)));
  GPIOC->MODER |= ((MODER_ALT << (BIT_10_POS * MODER_BIT_COUNT)) | (MODER_ALT << (BIT_11_POS * MODER_BIT_COUNT)));

  GPIOC->AFRH &= ~((GPIO_AF_MSK << ((BIT_10_POS - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << ((BIT_11_POS - 8) * GPIO_AF_BIT_COUNT)));
  GPIOC->AFRH |= ((GPIO_AF1 << ((BIT_10_POS - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF1 << ((BIT_11_POS - 8) * GPIO_AF_BIT_COUNT)));

  // GPIOC->OTYPER &= ~((1 << BIT_10) | (1 << BIT_11)); // Push-pull

  // // Set outputs to high speed
  // GPIOC->OSPEEDR &= ~((OSPEEDR_MSK << (BIT_10_POS * OSPEEDR_BIT_COUNT)) | (OSPEEDR_MSK << (BIT_11_POS * OSPEEDR_BIT_COUNT)));
  // GPIOC->OSPEEDR |= ((OSPEEDR_HIGH << (BIT_10_POS * OSPEEDR_BIT_COUNT)) | (OSPEEDR_HIGH << (BIT_11_POS * OSPEEDR_BIT_COUNT)));

  // GPIOC->PUPDR &= ~((3 << (BIT_10_POS * 2)) | (3 << (BIT_11_POS * 2)));
  // GPIOC->PUPDR |= (0 << (BIT_10_POS * 2)) | (1 << (BIT_11_POS * 2)); // No pull-up TX, pull-up RX

  // Configure USART4 (8-bit data, 1 stop bit)
  USART4->CR1 &= ~USART_CR1_UE;                             // Disable USART
  USART4->BRR = USART_BRR(F_SYS_CLOCK, TMC2209_BAUD_RATE);  // Set baud rate
  USART4->CR1 =                                             //
      USART_CR1_TE | USART_CR1_RE |                         // Enable transmit and receive
      USART_CR1_TXEIE_TXFNFIE | USART_CR1_RXNEIE_RXFNEIE;   // Enable interrupts
  USART4->CR3 = 0;                                          // No half-duplex
  USART4->CR1 |= USART_CR1_UE;                              // Enable USART

  // Enable USART4 interrupt in NVIC
  ENABLE_IRQ(USART3_4_5_6_LPUART1_IRQn);
}

void USART3_4_LPUART1_IRQHandler(void) {
  // Is the TX buffer empty?
  if ((USART4->ISR & USART_ISR_TXE_TXFNF)) {
    if (tmc2209_tx_buffer_tail != tmc2209_tx_buffer_head) {
      uint8_t data = tmc2209_tx_buffer[tmc2209_tx_buffer_tail++];

      if (tmc2209_tx_buffer_tail >= TMC2209_TX_BUFFER_SIZE) {
        tmc2209_tx_buffer_tail = 0;
      }

      USART4->TDR = data;
    } else {
      // Disable TX empty interrupt until more data sent
      USART4->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE);
    }
  }

  // Is RX FIFO not empty?
  if (USART4->ISR & USART_ISR_RXNE_RXFNE) {
    tmc2209_rx_buffer[tmc2209_rx_buffer_head++] = USART4->RDR;
    tmc2209_rx_count++;

    if (tmc2209_rx_buffer_head >= TMC2209_RX_BUFFER_SIZE) {
      tmc2209_rx_buffer_head = 0;
    }
  }
}

void tmc2209_uart_send(uint8_t b) {
  // Disable transmit empty interrupt while sending
  USART4->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE);

  // Add byte to TX buffer
  tmc2209_tx_buffer[tmc2209_tx_buffer_head++] = b;

  if (tmc2209_tx_buffer_head >= TMC2209_TX_BUFFER_SIZE) {
    tmc2209_tx_buffer_head = 0;
  }

  // Enable transmit empty interrupt
  USART4->CR1 |= USART_CR1_TXEIE_TXFNFIE;
}

int16_t tmc2209_uart_recv() {
  // Default to no data available
  int16_t data = -1;

  // Disable receive data interrupt while getting rx data
  USART4->CR1 &= ~(USART_CR1_RXNEIE_RXFNEIE);

  if (tmc2209_rx_buffer_tail != tmc2209_rx_buffer_head) {
    // Read from rx buffer
    data = tmc2209_rx_buffer[tmc2209_rx_buffer_tail++];

    if (tmc2209_rx_buffer_tail >= TMC2209_RX_BUFFER_SIZE) {
      tmc2209_rx_buffer_tail = 0;
    }

    tmc2209_rx_count--;
  } else {
    tmc2209_rx_count = 0;
  }

  // Enable receive data interrupt
  USART4->CR1 |= USART_CR1_RXNEIE_RXFNEIE;

  // Will be -1 if no data was available
  return data;
}

bool tmc2209_wait_for_count(uint8_t count, uint32_t max_wait_ms) {
  while (tmc2209_rx_count < count && max_wait_ms-- > 0) {
    delay_ms(1);
  }

  // Was successful if rx count is at least desired count
  return tmc2209_rx_count >= count;
}

uint8_t tmc2209_crc8(uint8_t *data, uint8_t length) {
  uint8_t crc = 0;
  uint8_t currentByte;

  for (uint8_t i = 0; i < length; i++) {  // Execute for all bytes of a message
    currentByte = data[i];                // Retrieve a byte to be sent from Array
    for (uint8_t j = 0; j < 8; j++) {
      if ((crc >> 7) ^ (currentByte & 0x01))  // update CRC based result of XOR operation
      {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc = (crc << 1);
      }
      currentByte = currentByte >> 1;
    }  // for CRC bit
  }  // for message byte

  return crc;
}

void tmc2209_clear_buffers() {
  // Disable interrupt while clearing
  USART4->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE | USART_CR1_RXNEIE_RXFNEIE);

  // Reset heads, tails and count
  tmc2209_tx_buffer_head = tmc2209_tx_buffer_tail = 0;
  tmc2209_rx_buffer_head = tmc2209_rx_buffer_tail = 0;
  tmc2209_rx_count = 0;

  // Restore receive interrupt (TX interrupt will be enabled when data is sent next)
  USART4->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
}

// Reference: TMC2209 DATASHEET (Rev. 1.09 / 2023-FEB-16)
// Section: 4.1.1 Write Access
void tmc2209_send_write_reg(uint8_t addr, uint8_t reg, uint32_t data) {
  tmc2209_clear_buffers();

  uint8_t packet[8];
  packet[0] = 0x05;                     // Sync nibble
  packet[1] = addr | 0x80;              // Device addr with write bit set
  packet[2] = reg;                      // Register address
  packet[3] = (data >> 24) & 0xFF;      // MSB
  packet[4] = (data >> 16) & 0xFF;      //
  packet[5] = (data >> 8) & 0xFF;       //
  packet[6] = (data >> 0) & 0xFF;       // LSB
  packet[7] = tmc2209_crc8(packet, 7);  // CRC

  for (int i = 0; i < 4; i++) {
    tmc2209_uart_send(packet[i]);
  }
}

// Reference: TMC2209 DATASHEET (Rev. 1.09 / 2023-FEB-16)
// Section: 4.1.2 Read Access
void tmc2209_send_read_reg(uint8_t addr, uint8_t reg) {
  tmc2209_clear_buffers();

  uint8_t packet[4];
  packet[0] = 0x05;                     // Sync nibble
  packet[1] = addr;                     // Device addr with write bit clear
  packet[2] = reg;                      // Register address
  packet[3] = tmc2209_crc8(packet, 3);  // CRC

  for (int i = 0; i < 4; i++) {
    tmc2209_uart_send(packet[i]);
  }
}

int tmc2209_parse_response(uint8_t sent_count, uint8_t *data_out, uint8_t reg) {
  // Note, as the E3 Mini uses one wire on RX4 then the transmitted bytes will be echoed
  // via the one wire resitor, so we need to ignore first 'sent_count' bytes of received data (because we transmitted it)

  // Need the sent count plus expected rx count of 8
  if (!tmc2209_wait_for_count(sent_count + 8, 100)) {
    // Not enough data
    return -1;
  }

  // We need to remove echoed bytes
  while (sent_count--) {
    tmc2209_uart_recv();
  }

  uint8_t rx_data[8];
  uint8_t rx_data_index = 0;

  // Check sync
  if ((tmc2209_uart_recv() & 0xFF) != 0x05) {
    return -2;
  }
  rx_data[rx_data_index++] = 0x05;

  // Check addr is 0xFF which is reserved for responses to master
  if ((tmc2209_uart_recv() & 0xFF) != 0xFF) {
    return -3;
  }
  rx_data[rx_data_index++] = 0xFF;

  // Check register
  if ((tmc2209_uart_recv() & 0xFF) != reg) {
    return -4;
  }
  rx_data[rx_data_index++] = reg;

  // Read response data (4 bytes)
  rx_data[rx_data_index++] = (tmc2209_uart_recv() & 0xFF);
  rx_data[rx_data_index++] = (tmc2209_uart_recv() & 0xFF);
  rx_data[rx_data_index++] = (tmc2209_uart_recv() & 0xFF);
  rx_data[rx_data_index++] = (tmc2209_uart_recv() & 0xFF);

  // Read received CRC
  uint8_t crc = (tmc2209_uart_recv() & 0xFF);

  // Check CRC
  uint8_t calc_crc = tmc2209_crc8(rx_data, 7);
  if (crc != calc_crc) {
    return -5;  // Bad CRC
  }

  // Copy 4-byte GCONF register value (LSB first)
  for (int i = 0; i < 4; i++) {
    data_out[i] = rx_data[3 + i];
  }

  return 0;
}

uint32_t tmc2209_read_reg(uint8_t addr, uint8_t reg) {
  tmc2209_send_read_reg(addr, reg);

  uint8_t value_bytes[4];
  uint32_t value = 0;
  int result = tmc2209_parse_response(4, value_bytes, reg);
  if (result == 0) {
    value = (value_bytes[0] << 24) |
            (value_bytes[1] << 16) |
            (value_bytes[2] << 8) |
            (value_bytes[3]);
  }

  return value;
}

void tmc2209_read_gconf(uint8_t addr) {
  uint32_t gconf = tmc2209_read_reg(addr, TMC2209_REG_GCONF);
  uart_printf("   gconf: 0x%x\r\n", gconf);
}

void tmc2209_reset_status(uint8_t addr) {
  uart_printf("   gstat:");
  uint32_t gstat = tmc2209_read_reg(addr, TMC2209_REG_GSTAT);
  uart_printf(" 0x%x->", gstat);
  tmc2209_send_write_reg(addr, TMC2209_REG_GSTAT, gstat);
  gstat = tmc2209_read_reg(addr, TMC2209_REG_GSTAT);
  uart_printf("0x%x\r\n", gstat);
}

void tmc_init(uint8_t addr) {
  uart_printf("Initialising TMC2209 at address: 0x%x\r\n", addr);
  tmc2209_reset_status(addr);
  tmc2209_read_gconf(addr);

  uart_puts("\r\n");
}

// Default to initialising at boot
TMC2209_Initialise_State init_state = STATE_INIT_X;

void tmc2209_initialise() {
  switch (init_state) {
    case STATE_INIT_X:
      tmc_init(0x00);
      init_state = STATE_INIT_Y;
      break;

    case STATE_INIT_Y:
      tmc_init(0x01);
      init_state = STATE_INIT_Z;
      break;

    case STATE_INIT_Z:
      tmc_init(0x02);
      init_state = STATE_INIT_E;
      break;

    case STATE_INIT_E:
      tmc_init(0x03);
      init_state = STATE_INIT_DONE;
      break;

    case STATE_INIT_DONE:
    default:
      // Should never occur, but in case it does just set as done and continue
      init_state = STATE_INIT_DONE;
      break;
  }
}

void tmc2209_tick() {
  if (init_state != STATE_INIT_DONE) {
    tmc2209_initialise();
  }
}