#include <stdint.h>

#include "board_hal.h"
#include "clock.h"

typedef enum {
  EE_WRITE = 0,
  EE_READ = 1
} EEPROM_Command_Type;

// The E3 mini has AT24C32 EEPROM fixed at base address 0x50 (A0/A1/A2 all pulled low)
#define AT24C32_ADDR 0x50

// TIMINGR = 0x10B17DB5
// Given: F_SYS_CLOCK = 64 MHz
//        t = time (period)
//          = 1/f (period is inverse of frequency)
//
// tI2CCLK = (1 / F_SYS_CLOCK)
//         = (1 / 64 MHz)
//         = 15.625 ns
//
// PRESC   = 1 → tPRESC
//         = (PRESC + 1) × tI2CCLK
//         = (1 + 1) × tI2CCLK
//         = 2 × 15.625 ns
//         = 31.25 ns
//
// SCLL    = 181 + 1 → 182 tPRESC
// SCLH    = 125 + 1 → 126 tPRESC
// T_I2C   = (182 + 126) × 31.25 ns
//         = 308 × 31.25 ns
//         = 9.625 µs
//
// f_I2C   = 1 / T_I2C
//         = 1 / 9.625 µs
//         ≈ 103.9 kHz

#define I2C_PRESC (1U << 28)    // PRESC = 1
#define I2C_SCLDEL (11U << 20)  // SCLDEL = 11 (so will be 11 + 1)
#define I2C_SDADEL (1U << 16)   // SDADEL = 1
#define I2C_SCLH (125U << 8)    // SCLH   = 125
#define I2C_SCLL (181U << 0)    // SCLL   = 181
#define I2C1_TIMINGR (I2C_PRESC | I2C_SCLDEL | I2C_SDADEL | I2C_SCLH | I2C_SCLL)

// I2C1 master init for ~100 kHz
void i2c1_master_init() {
  // Enable I2C1 clock
  RCC->APBENR1 |= RCC_APBENR1_I2C1EN;

  // Disable before configuring
  I2C1->CR1 &= ~I2C_CR1_PE;

  // Disable own address (used for slave mode)
  I2C1->OAR1 &= ~I2C_OAR1_OA1EN;

  // Disable analog filter, DNF = 0
  I2C1->CR1 &= ~I2C_CR1_DNF;
  I2C1->CR1 |= I2C_CR1_ANFOFF;

  // Refer to: RM0444 section 32.4.10 I2C_TIMINGR register configuration examples
  // Timing register value is: 0x10B17DB5 for 64MHz clock and I2C speed mode standard mode
  I2C1->TIMINGR = I2C1_TIMINGR;

  // Make sure 7‑bit addr only
  I2C1->CR2 &= ~I2C_CR2_ADD10;

  // Automatic end mode: a STOP condition is automatically sent when NBYTES data are transferred.
  I2C1->CR2 |= (I2C_CR2_AUTOEND);

  // Enable clock stretching (allow slave to slow down master if needed)
  I2C1->CR1 &= ~I2C_CR1_NOSTRETCH;

  // Enable
  I2C1->CR1 |= I2C_CR1_PE;
}

static void i2c1_start(uint8_t addr, uint16_t nbytes, EEPROM_Command_Type transfer_direction) {
  // Wait for bus to be free
  while (I2C1->ISR & I2C_ISR_BUSY);

  // Reset flags and address
  I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_TIMOUTCF | I2C_ICR_ADDRCF;

  // Clear old transfer config bits (especially NBYTES, SADD, etc)
  I2C1->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_AUTOEND);

  uint8_t shifted_addr = (addr << 1 | (transfer_direction == EE_READ ? 1 : 0));

  // Set up new transfer
  I2C1->CR2 |=
      // Device address
      (shifted_addr << I2C_CR2_SADD_Pos) |

      // Number of bytes that will be sent
      ((uint32_t)nbytes << I2C_CR2_NBYTES_Pos) |

      // Transfer direction
      (transfer_direction == EE_READ ? I2C_CR2_RD_WRN : 0) |

      // Set start bit
      I2C_CR2_START |

      // Set auto end
      I2C_CR2_AUTOEND;
}

static void i2c1_stop(void) {
  // Generate STOP condition
  I2C1->CR2 |= I2C_CR2_STOP;

  // Wait for STOPF flag to be set
  while (!(I2C1->ISR & I2C_ISR_STOPF));

  // Clear STOPF flag by writing 1 to STOPCF in ICR
  I2C1->ICR = I2C_ICR_STOPCF;

  // Wait for BUSY to clear (ensures STOP is fully finished)
  while (I2C1->ISR & I2C_ISR_BUSY);
}

static bool eeprom_ack_polling(void) {
  uint32_t now = get_sys_tick();  // Get current tick
  uint32_t expires = now + 100;   // After about 1 second

  // Clear previous flags
  I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;

  while (get_sys_tick() < expires) {
    I2C1->CR2 = ((AT24C32_ADDR << I2C_CR2_SADD_Pos) |
                 (0 << I2C_CR2_NBYTES_Pos) |
                 I2C_CR2_START);

    while (I2C1->CR2 & I2C_CR2_START);

    if (!(I2C1->ISR & I2C_ISR_NACKF)) {
      if (I2C1->ISR & I2C_ISR_STOPF) {
        I2C1->ICR = I2C_ICR_STOPCF;
      }
      return true;
    }

    // Clear NAK
    I2C1->ICR = I2C_ICR_NACKCF;

    // Just sit idle for a bit so we are not hammering get_sys_tick
    for (int i = 0; i < 100; i++);
  }

  return false;  // timeout or no ACK
}

static bool i2c1_write_byte(uint8_t b) {
  if (I2C1->ISR & I2C_ISR_NACKF) {
    // Clear the NACK flag
    I2C1->ICR = I2C_ICR_NACKCF;

    // This means the slave didn't acknowledge the address phase
    // You can return or print a debug error here
    return false;
  }

  // Wait TXDR empty
  while (!(I2C1->ISR & I2C_ISR_TXIS));
  I2C1->TXDR = b;

  return true;
}

static uint8_t i2c1_read_byte(void) {
  // Wait RXDR full
  while (!(I2C1->ISR & I2C_ISR_RXNE));
  return (uint8_t)I2C1->RXDR;
}

void at24c32_write_byte(uint16_t mem_addr, uint8_t data) {
  // Start with device at address specified, 3 bytes (2 mem addr + 1 data) and is write mode (read == 0)
  i2c1_start(AT24C32_ADDR, 3, EE_WRITE);

  i2c1_write_byte((uint8_t)(mem_addr >> 8));
  i2c1_write_byte((uint8_t)(mem_addr & 0xFF));
  i2c1_write_byte(data);

  // Wait TX all nbytes data complete
  while (!(I2C1->ISR & I2C_ISR_STOPF));

  // Release bus
  i2c1_stop();

  // Wait for ACK from eeprom, ACK is sent once it has finished writing and is idle
  eeprom_ack_polling();
}

// AT24C32 single‑byte read
uint8_t at24c32_read_byte(uint16_t mem_addr) {
  uint8_t val;

  // Write mem‑address pointer (no STOP)
  i2c1_start(AT24C32_ADDR, 2, EE_WRITE);
  i2c1_write_byte((uint8_t)(mem_addr >> 8));
  i2c1_write_byte((uint8_t)(mem_addr & 0xFF));

  // Wait TX all nbytes data complete
  while (!(I2C1->ISR & I2C_ISR_STOPF));

  // Repeated START + read 1 byte
  i2c1_start(AT24C32_ADDR, 1, EE_READ);
  val = i2c1_read_byte();
  i2c1_stop();

  return val;
}

void init_eeprom(void) {
  GPIO_SET_MODE(GPIOB, BIT_06_POS, MODER_ALT);  // Set mode to alternate function (AF) on PB6
  GPIO_SET_MODE(GPIOB, BIT_07_POS, MODER_ALT);  // Set mode to alternate function (AF) on PB7

  // Set open-drain on PB6 & PB7
  GPIOB->OTYPER |= BIT_06 | BIT_07;

  // Set high speed on PB6 & PB7
  GPIOB->OSPEEDR &= ~((0b11 << (BIT_06_POS * 2)) |
                      (0b11 << (BIT_07_POS * 2)));
  GPIOB->OSPEEDR |=
      (0b10 << (BIT_06_POS * 2)) | (0b10 << (BIT_07_POS * 2));

  // Reset AF on PB6 & PB7
  GPIOB->AFR[0] &= ~((GPIO_AF_MSK << (BIT_06_POS * GPIO_AF_BIT_COUNT)) |
                     (GPIO_AF_MSK << (BIT_07_POS * GPIO_AF_BIT_COUNT)));

  // See: Table 15. Port B alternate function mapping (AF0 to AF7)
  // AF6 -> PB6: I2C1_SCL, PB7: I2C1_SDA
  GPIOB->AFR[0] |= ((GPIO_AF6 << (BIT_06_POS * GPIO_AF_BIT_COUNT)) |
                    (GPIO_AF6 << (BIT_07_POS * GPIO_AF_BIT_COUNT)));
}
