#include <stdint.h>

typedef struct
{
    volatile uint32_t DIR;          // Offset: 0x00-0x03
    volatile uint32_t RESERVED0[3]; // Offset: 0x04-0x0F (these register locations not used)
    volatile uint32_t MSK;          // Offset: 0x10-0x13
    volatile uint32_t PIN;          // Offset: 0x14-0x17
    volatile uint32_t SET;          // Offset: 0x18-0x1B
    volatile uint32_t CLR;          // Offset: 0x1C-0x1F
} LPC_GPIO_TypeDef;

/**********************************************************************************
 * GPIO registers
 **********************************************************************************/
#define GPIO0_BASE 0x2009C000UL // Base address for GPIO port 0 registers
#define GPIO1_BASE 0x2009C020UL // Base address for GPIO port 1 registers
#define GPIO2_BASE 0x2009C040UL // Base address for GPIO port 2 registers
#define GPIO3_BASE 0x2009C060UL // Base address for GPIO port 3 registers
#define GPIO4_BASE 0x2009C080UL // Base address for GPIO port 4 registers

#define GPIO0 ((LPC_GPIO_TypeDef *)GPIO1_BASE)
#define GPIO1 ((LPC_GPIO_TypeDef *)GPIO1_BASE)
#define GPIO2 ((LPC_GPIO_TypeDef *)GPIO2_BASE)
#define GPIO3 ((LPC_GPIO_TypeDef *)GPIO3_BASE)
#define GPIO4 ((LPC_GPIO_TypeDef *)GPIO4_BASE)

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
