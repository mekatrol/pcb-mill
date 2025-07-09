#include <stdint.h>

/**********************************************************************************
 * GPIO registers
 **********************************************************************************/
#define GPIO0_BASE 0x2009C000UL // Base address for GPIO port 1 registers
#define GPIO1_BASE 0x2009C020UL // Base address for GPIO port 1 registers
#define GPIO2_BASE 0x2009C040UL // Base address for GPIO port 2 registers
#define GPIO3_BASE 0x2009C060UL // Base address for GPIO port 3 registers
#define GPIO4_BASE 0x2009C080UL // Base address for GPIO port 4 registers

#define GPIO_DIR_OFFSET 0x00
#define GPIO_MSK_OFFSET 0x10
#define GPIO_PIN_OFFSET 0x14
#define GPIO_SET_OFFSET 0x18
#define GPIO_CLR_OFFSET 0x1C

#define GPIO0_DIR (*(volatile uint32_t *)(GPIO0_BASE + GPIO_DIR_OFFSET))
#define GPIO0_MSK (*(volatile uint32_t *)(GPIO0_BASE + GPIO_MSK_OFFSET))
#define GPIO0_PIN (*(volatile uint32_t *)(GPIO0_BASE + GPIO_PIN_OFFSET))
#define GPIO0_SET (*(volatile uint32_t *)(GPIO0_BASE + GPIO_SET_OFFSET))
#define GPIO0_CLR (*(volatile uint32_t *)(GPIO0_BASE + GPIO_CLR_OFFSET))

#define GPIO1_DIR (*(volatile uint32_t *)(GPIO1_BASE + GPIO_DIR_OFFSET))
#define GPIO1_MSK (*(volatile uint32_t *)(GPIO1_BASE + GPIO_MSK_OFFSET))
#define GPIO1_PIN (*(volatile uint32_t *)(GPIO1_BASE + GPIO_PIN_OFFSET))
#define GPIO1_SET (*(volatile uint32_t *)(GPIO1_BASE + GPIO_SET_OFFSET))
#define GPIO1_CLR (*(volatile uint32_t *)(GPIO1_BASE + GPIO_CLR_OFFSET))

#define GPIO2_DIR (*(volatile uint32_t *)(GPIO2_BASE + GPIO_DIR_OFFSET))
#define GPIO2_MSK (*(volatile uint32_t *)(GPIO2_BASE + GPIO_MSK_OFFSET))
#define GPIO2_PIN (*(volatile uint32_t *)(GPIO2_BASE + GPIO_PIN_OFFSET))
#define GPIO2_SET (*(volatile uint32_t *)(GPIO2_BASE + GPIO_SET_OFFSET))
#define GPIO2_CLR (*(volatile uint32_t *)(GPIO2_BASE + GPIO_CLR_OFFSET))

#define GPIO3_DIR (*(volatile uint32_t *)(GPIO3_BASE + GPIO_DIR_OFFSET))
#define GPIO3_MSK (*(volatile uint32_t *)(GPIO3_BASE + GPIO_MSK_OFFSET))
#define GPIO3_PIN (*(volatile uint32_t *)(GPIO3_BASE + GPIO_PIN_OFFSET))
#define GPIO3_SET (*(volatile uint32_t *)(GPIO3_BASE + GPIO_SET_OFFSET))
#define GPIO3_CLR (*(volatile uint32_t *)(GPIO3_BASE + GPIO_CLR_OFFSET))

#define GPIO4_DIR (*(volatile uint32_t *)(GPIO4_BASE + GPIO_DIR_OFFSET))
#define GPIO4_MSK (*(volatile uint32_t *)(GPIO4_BASE + GPIO_MSK_OFFSET))
#define GPIO4_PIN (*(volatile uint32_t *)(GPIO4_BASE + GPIO_PIN_OFFSET))
#define GPIO4_SET (*(volatile uint32_t *)(GPIO4_BASE + GPIO_SET_OFFSET))
#define GPIO4_CLR (*(volatile uint32_t *)(GPIO4_BASE + GPIO_CLR_OFFSET))

/**********************************************************************************
 * PINSEL registers
 **********************************************************************************/
#define PINSEL0 (*(volatile uint32_t *)(0x4002C000UL))
#define PINSEL1 (*(volatile uint32_t *)(0x4002C004UL))
#define PINSEL2 (*(volatile uint32_t *)(0x4002C008UL))
#define PINSEL3 (*(volatile uint32_t *)(0x4002C00CUL))
#define PINSEL4 (*(volatile uint32_t *)(0x4002C010UL))
#define PINSEL7 (*(volatile uint32_t *)(0x4002C01CUL))
#define PINSEL8 (*(volatile uint32_t *)(0x4002C020UL))
#define PINSEL9 (*(volatile uint32_t *)(0x4002C024UL))
#define PINSEL10 (*(volatile uint32_t *)(0x4002C028UL))
#define PINMODE0 (*(volatile uint32_t *)(0x4002C040UL))
#define PINMODE1 (*(volatile uint32_t *)(0x4002C044UL))
#define PINMODE2 (*(volatile uint32_t *)(0x4002C048UL))
#define PINMODE3 (*(volatile uint32_t *)(0x4002C04CUL))
#define PINMODE4 (*(volatile uint32_t *)(0x4002C050UL))
#define PINMODE5 (*(volatile uint32_t *)(0x4002C054UL))
#define PINMODE6 (*(volatile uint32_t *)(0x4002C058UL))
#define PINMODE7 (*(volatile uint32_t *)(0x4002C05CUL))
#define PINMODE9 (*(volatile uint32_t *)(0x4002C064UL))
#define PINMODE_OD0 (*(volatile uint32_t *)(0x4002C068UL))
#define PINMODE_OD1 (*(volatile uint32_t *)(0x4002C06CUL))
#define PINMODE_OD2 (*(volatile uint32_t *)(0x4002C070UL))
#define PINMODE_OD3 (*(volatile uint32_t *)(0x4002C074UL))
#define PINMODE_OD4 (*(volatile uint32_t *)(0x4002C078UL))
#define I2CPADCFG (*(volatile uint32_t *)(0x4002C07CUL))

/**********************************************************************************
 * SPI registers
 **********************************************************************************/
#define S0SPCR (*(volatile uint32_t *)0x40020000)
#define S0SPSR (*(volatile uint32_t *)0x40020004)
#define S0SPDR (*(volatile uint32_t *)0x40020008)
#define S0SPCCR (*(volatile uint32_t *)0x4002000C)
