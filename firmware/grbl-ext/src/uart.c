#include "clock.h"
#include "irq.h"
#include "register_bits.h"
#include "uart.h"

#define USART3_4_LPUART1_IRQn 29
#define USART_BRR(PCLK, BAUD) (((PCLK) + ((BAUD) / 2)) / (BAUD))

#define SET_MODER_AF(GPIO, PIN)                       \
    do                                                \
    {                                                 \
        (GPIO)->MODER &= ~(MODER_MSK << ((PIN) * 2)); \
        (GPIO)->MODER |= (MODER_ALT << ((PIN) * 2));  \
    } while (0)

#define AFR_BITS_PER_PIN 4
#define AFR_PINS_PER_REG 8
#define AFR_MASK 0xF

#define PIN_PC10 10
#define PIN_PC11 11
#define AF1_USART4 0x1

#define AFRH_POS(pin) (((pin) - AFR_PINS_PER_REG) * AFR_BITS_PER_PIN)
#define AFRH_MASK(pin) (AFR_MASK << AFRH_POS(pin))
#define AFRH_VAL(pin, af) ((af) << AFRH_POS(pin))

// RX buffer
#define RX_BUF_SIZE 64
volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;

uint8_t tmc_crc8(uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;
    uint8_t currentByte;

    for (uint8_t i = 0; i < length; i++)
    {                          // Execute for all bytes of a message
        currentByte = data[i]; // Retrieve a byte to be sent from Array
        for (uint8_t j = 0; j < 8; j++)
        {
            if ((crc >> 7) ^ (currentByte & 0x01)) // update CRC based result of XOR operation
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc = (crc << 1);
            }
            currentByte = currentByte >> 1;
        } // for CRC bit
    } // for message byte

    return crc;
}

void tmc2209_read_gconf(uint8_t slave)
{
    uint8_t packet[4];
    packet[0] = 0x05;                // Sync nibble
    packet[1] = slave;               // Slave addr
    packet[2] = 0x00;                // Register address (GCONF) + read b7=1
    packet[3] = tmc_crc8(packet, 3); // CRC

    for (int i = 0; i < 4; i++)
    {
        uart4_send(packet[i]);
    }
}

int tmc2209_parse_reply(uint8_t *data_out, uint8_t slave)
{
    if (rx_index < 8)
        return -1; // Not enough data

    // Check sync
    if (rx_buf[0] != 0x05)
        return -2;

    // Check slave addr + write=0
    if (rx_buf[1] != ((slave << 1) | 0))
        return -3;

    // Check register
    if (rx_buf[2] != 0x00)
        return -4;

    // Check CRC
    uint8_t calc_crc = tmc_crc8((uint8_t *)rx_buf, 7);
    if (rx_buf[7] != calc_crc)
        return -5;

    // Copy 4-byte GCONF register value (LSB first)
    for (int i = 0; i < 4; i++)
        data_out[i] = rx_buf[3 + i];

    return 0;
}

void uart4_init(void)
{
    // Enable USART4 clocks
    RCC->APBENR1 |= RCC_APBENR1_USART4EN;

    // Enable USART4 clocks
    // SET_READ_BIT(RCC->APBENR1, RCC_APBENR1_USART4EN);

    // Make sure PORTC enabled
    // SET_READ_BIT(RCC->IOPENR, IOPENR_PORTC_ENABLE);

    // PC10 = TX, PC11 = RX => Set both to AF mode
    // SET_MODER_AF(GPIOC, 10);
    // SET_MODER_AF(GPIOC, 11);

    // Set PC10 and PC11 to Alternate Function mode (MODER = 10)
    GPIOC->MODER &= ~((3 << (10 * 2)) | (3 << (11 * 2))); // Clear bits
    GPIOC->MODER |= ((2 << (10 * 2)) | (2 << (11 * 2)));  // Set AF mode

    // Select AF1 (USART4) for PC10/11 (AFRH: pins 8-15)
    GPIOC->AFRH &= ~((0xF << ((10 - 8) * 4)) | (0xF << ((11 - 8) * 4))); // Clear
    GPIOC->AFRH |= ((1 << ((10 - 8) * 4)) | (1 << ((11 - 8) * 4)));      // AF1 = USART4

    GPIOC->OTYPER &= ~((1 << PIN_PC10) | (1 << PIN_PC11)); // Push-pull

    GPIOC->OSPEEDR &= ~((3 << (PIN_PC10 * 2)) | (3 << (PIN_PC11 * 2)));
    GPIOC->OSPEEDR |= ((2 << (PIN_PC10 * 2)) | (2 << (PIN_PC11 * 2))); // High speed

    GPIOC->PUPDR &= ~((3 << (PIN_PC10 * 2)) | (3 << (PIN_PC11 * 2)));
    GPIOC->PUPDR |= (0 << (PIN_PC10 * 2)) | (1 << (PIN_PC11 * 2)); // No pull TX, pull-up RX

    // // Clear AFR[10] and AFR[11] bits
    // GPIOC->AFRH &= ~(AFRH_MASK(PIN_PC10) | AFRH_MASK(PIN_PC11));

    // // Set AFR[10] and AFR[11] to AF1 (USART4)
    // GPIOC->AFRH |= (AFRH_VAL(PIN_PC10, AF1_USART4) | AFRH_VAL(PIN_PC11, AF1_USART4));

    // Disable USART before config
    USART4->CR1 &= ~(USART_CR1_UE);

    // USARTDIV = 64000000 / 115200 ≈ 555.555
    // BRR = round(USARTDIV) = 556
    // USART4->BRR = 556; // For 115200 baud with 64 MHz PCLK
    USART4->BRR = USART_BRR(64000000, 115200);

    // 8-bit data, 1 stop bit
    USART4->CR1 = (USART_CR1_RE) | (USART_CR1_TE) | (USART_CR1_RXNEIE_RXFNEIE); // RE, TE, RXNEIE
    USART4->CR2 = 0;                                                            // 1 stop bit (default)
    USART4->CR3 = 0;                                                            // No half-duplex

    // Enable USART
    USART4->CR1 |= USART_CR1_UE;

    USART4->ICR = 0xFFFFFFFF;

    // Enable USART4 interrupt in NVIC
    ENABLE_IRQ(USART3_4_LPUART1_IRQn);
}

void uart4_send(uint8_t b)
{
    while (!(USART4->ISR & USART_ISR_TXE_TXFNF))
        ; // Wait until TX FIFO has space (TXFNF == 1)

    USART4->TDR = b;
    while (!(USART4->ISR & USART_ISR_TC))
        ; // Wait transmit complete (TC)

    USART4->ICR |= USART_ISR_TC; // Clear transmit complete (TC)
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