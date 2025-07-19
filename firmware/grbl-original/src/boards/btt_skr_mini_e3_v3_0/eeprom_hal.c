#include <stdint.h>

#include "clock.h"
#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

// The E3 mini has EEPROM fixed at address 0 (A0/A1/A2 all pulled low)
#define E3_MINI_EEPROM_DEV_ADDR 0

// Tight loop delay
static void delay(volatile uint32_t d) {
  while (d--) __asm__("nop");
}

// I2C1 master init for ~100 kHz
void i2c1_master_init(void) {
  // Enable I2C1 clock
  RCC->APBENR1 |= RCC_APBENR1_I2C1EN;

  // Disable before configuring
  I2C1->CR1 &= ~I2C_CR1_PE;

  // Filters off, DNFs = 3
  I2C1->CR1 |= (3U << I2C_CR1_DNF_Pos);
  I2C1->CR1 |= I2C_CR1_ANFOFF;

  // Timing: presc=0, SCLL/SCLH/SDADEL/SCLDEL tuned for 64 MHz PCLK
  I2C1->TIMINGR = ((0U << 28) | /* PRESC = 0 */
                   (4U << 20) | /* SCLDEL = 4 + 1 */
                   (2U << 16) | /* SDADEL = 2     */
                   (15U << 8) | /* SCLH   = 15+1  */
                   (19U << 0)   /* SCLL   = 19+1  */
  );

  // 7‑bit addr, enable stretching
  I2C1->CR2 &= ~I2C_CR2_ADD10;
  I2C1->CR1 &= ~I2C_CR1_NOSTRETCH;

  // Enable
  I2C1->CR1 |= I2C_CR1_PE;
}

static void i2c1_start(uint8_t addr, uint16_t nbytes, uint8_t read) {
  // Wait for bus free
  while (I2C1->ISR & I2C_ISR_BUSY);

  // Clear STOP flag
  I2C1->ICR = I2C_ICR_STOPCF;

  // Program slave address + R/W, byte count, generate START
  I2C1->CR2 = ((uint32_t)addr << I2C_CR2_SADD_Pos) |
              ((uint32_t)nbytes << I2C_CR2_NBYTES_Pos) |
              (read ? (1U << 10) : 0) | I2C_CR2_START;
}

static void i2c1_stop(void) {
  I2C1->CR2 |= I2C_CR2_STOP;

  // Wait for STOP flag, then clear it
  while (!(I2C1->ISR & I2C_ISR_STOPF));
  I2C1->ICR = I2C_ICR_STOPCF;
}

static void i2c1_write_byte(uint8_t b) {
  // Wait TXDR empty
  while (!(I2C1->ISR & I2C_ISR_TXIS));
  I2C1->TXDR = b;
}

static uint8_t i2c1_read_byte(void) {
  // Wait RXDR full
  while (!(I2C1->ISR & I2C_ISR_RXNE));
  return (uint8_t)I2C1->RXDR;
}

// AT24C32 single‑byte write
void at24c32_write_byte(uint8_t dev_addr, uint16_t mem_addr, uint8_t data) {
  // AT24C: dev_addr = 0x50 + (block bits from mem_addr >> 8)
  uint8_t sa = dev_addr | ((mem_addr >> 8) & 0x07);

  // START + send device+W + 2 addr bytes + data
  i2c1_start(sa, 3, 0);
  i2c1_write_byte((uint8_t)(mem_addr >> 8));
  i2c1_write_byte((uint8_t)(mem_addr & 0xFF));
  i2c1_write_byte(data);

  // wait transfer complete, then STOP
  while (!(I2C1->ISR & I2C_ISR_TC));
  i2c1_stop();

  // Give the EEPROM time to finish its internal write cycle
  delay(50000);
}

// AT24C32 single‑byte read
uint8_t at24c32_read_byte(uint8_t dev_addr, uint16_t mem_addr) {
  uint8_t sa = dev_addr | ((mem_addr >> 8) & 0x07);
  uint8_t val;

  // Write mem‑address pointer (no STOP)
  i2c1_start(sa, 2, 0);
  i2c1_write_byte((uint8_t)(mem_addr >> 8));
  i2c1_write_byte((uint8_t)(mem_addr & 0xFF));

  // Wait TX complete
  while (!(I2C1->ISR & I2C_ISR_TC));

  // Repeated START + read 1 byte
  i2c1_start(sa, 1, 1);
  val = i2c1_read_byte();
  i2c1_stop();

  return val;
}

void init_eeprom(void) {
  // Set mode to alternate function (AF) on PB6 & PB7
  GPIOB->MODER &= ~((MODER_MSK << (BIT_06 * MODER_BIT_COUNT)) |
                    (MODER_MSK << (BIT_07 * MODER_BIT_COUNT)));

  GPIOB->MODER |= (MODER_ALT << (BIT_06 * MODER_BIT_COUNT)) |
                  (MODER_ALT << (BIT_07 * MODER_BIT_COUNT));

  // Set open-drain on PB6 & PB7
  GPIOB->OTYPER |= (1U << 6) | (1U << 7);

  // Set high speed on PB6 & PB7
  GPIOB->OSPEEDR &= ~((MODER_MSK << (BIT_06 * MODER_BIT_COUNT)) |
                      (MODER_MSK << (BIT_07 * MODER_BIT_COUNT)));
  GPIOB->OSPEEDR |=
      (1 << (BIT_06 * MODER_BIT_COUNT)) | (1 << (BIT_07 * MODER_BIT_COUNT));

  // Set to AF6 on PB6 & PB7
  GPIOB->AFRL &= ~((GPIO_AF_MSK << (BIT_06 * GPIO_AF_BIT_COUNT)) |
                   (GPIO_AF_MSK << (BIT_07 * GPIO_AF_BIT_COUNT)));

  GPIOB->AFRL |= ((GPIO_AF6 << (BIT_06 * GPIO_AF_BIT_COUNT)) |
                  (GPIO_AF6 << (BIT_07 * GPIO_AF_BIT_COUNT)));
}

// /* === Example usage === */
// int main(void) {
//   gpiob_i2c_pins_init();
//   i2c1_master_init();

//   /* Write 0x37 at address 0x0123 */
//   at24c32_write_byte(0x50, 0x0123, 0x37);

//   /* Read it back */
//   uint8_t x = at24c32_read_byte(0x50, 0x0123);
//   (void)x;

//   while (1) { /* … */
//   }
// }

uint8_t eeprom_get_char_hal(uint32_t addr) {
  return at24c32_read_byte(E3_MINI_EEPROM_DEV_ADDR, addr);
}

void eeprom_put_char_hal(uint32_t addr, uint8_t new_value) {
  at24c32_write_byte(E3_MINI_EEPROM_DEV_ADDR, addr, new_value);
}
