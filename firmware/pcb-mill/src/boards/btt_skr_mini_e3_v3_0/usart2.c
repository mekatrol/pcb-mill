#include "clock.h"
#include "board_hal.h"
#include "usart_hal.h"

// USART2 uart baud rate
#define USART2_BAUD_RATE 115200

// USART2 RX buffer
#define USART2_RX_BUFFER_SIZE 2048
static uint8_t usart2_rx_buffer[USART2_RX_BUFFER_SIZE];

// USART2 TX buffer
#define USART2_TX_BUFFER_SIZE 2048
static uint8_t usart2_tx_buffer[USART2_TX_BUFFER_SIZE];

static usart_instance_t usart2_instance = {
    .hal = USART2,
    .baud_rate = USART_BRR(F_SYS_CLOCK, USART2_BAUD_RATE),

    .rx_buffer = usart2_rx_buffer,
    .rx_buffer_head = 0,
    .rx_buffer_tail = 0,
    .rx_buffer_size = USART2_RX_BUFFER_SIZE,

    .tx_buffer = usart2_tx_buffer,
    .tx_buffer_head = 0,
    .tx_buffer_tail = 0,
    .tx_buffer_size = USART2_TX_BUFFER_SIZE,

    /**/};

void diag_usart2_init() {
  usart_init_hal(
      &usart2_instance,      // The usart instance data
      GPIOA,                 // The GPIO port the usart is on
      BIT_02_POS,            // The GPIO TX pin
      BIT_03_POS,            // The GPIO rx pin
      RCC_APBENR1_USART2EN,  // The USART enable bit
      USART2_LPUART2_IRQn    // The usart IRQ number
  );
}

void diag_send(uint8_t b) {
  usart_send(&usart2_instance, b);
}

void diag_flush() {
  usart_wait_data_sent(&usart2_instance);
}

void USART2_IRQHandler() {
  usart_irq_handler(&usart2_instance);
}