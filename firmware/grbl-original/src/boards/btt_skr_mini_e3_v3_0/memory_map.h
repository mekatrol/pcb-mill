#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

/***************************************************************************************************
 * Memory map and register boundary addresses
 * Refer to section 2.2.2 of RM0444 Reference Manual
 ***************************************************************************************************/
#include <stdint.h>

/***************************************************************************************************
 * Memory base addresses
 ***************************************************************************************************/
#define FLASH_BASE 0x08000000UL  /* FLASH memory */
#define SRAM_BASE 0x20000000UL   /* SRAM memory */
#define IOPORT_BASE 0x50000000UL /* GPIO register block base */

#define SCS_BASE (0xE000E000UL)                   /* System Control Space Base Address */
#define DWT_BASE (0xE0001000UL)                   /* DWT Base Address */
#define TPI_BASE (0xE0040000UL)                   /* TPI Base Address */
#define CoreDebug_BASE (0xE000EDF0UL)             /* Core Debug Base Address */
#define SysTick_BASE (SCS_BASE + 0x0010UL)        /* SysTick Base Address */
#define NVIC_BASE (SCS_BASE + 0x0100UL)           /* NVIC Base Address */
#define SCB_BASE (SCS_BASE + 0x0D00UL)            /* System Control Block Base Address */
#define SYSCFG_BASE (0x40000000UL + 0x00010000UL) /* System Configuration Controller */
#define EXTI_BASE (0x40000000UL + 0x00001800UL)

/***************************************************************************************************
 * AHB - Advanced High-performance Bus
 ***************************************************************************************************/
#define AHB_BASE 0x40020000UL
#define RCC_BASE 0x40021000UL             /* Reset and Clock Control */
#define FLASH_REGISTERS_BASE 0x40022000UL /* Flash control registers */

/***************************************************************************************************
 * APB - Advanced Peripheral Bus
 ***************************************************************************************************/
#define APB1_BASE 0x40000000UL

/***************************************************************************************************
 * PWR - Power control registers
 ***************************************************************************************************/
#define PWR_BASE 0x40007000UL

/***************************************************************************************************
 * APB peripherals
 ***************************************************************************************************/
#define TIM1_BASE (0x40000000UL + 0x00012C00UL)
#define TIM2_BASE (0x40000000UL + 0UL)
#define TIM3_BASE (0x40000000UL + 0x00000400UL)
#define TIM4_BASE (0x40000000UL + 0x00000800UL)
#define TIM6_BASE (0x40000000UL + 0x00001000UL)
#define TIM7_BASE (0x40000000UL + 0x00001400UL)
#define TIM14_BASE (0x40000000UL + 0x00002000UL)
#define TIM15_BASE (0x40000000UL + 0x00014000UL)
#define TIM16_BASE (0x40000000UL + 0x00014400UL)
#define TIM17_BASE (0x40000000UL + 0x00014800UL)

/***************************************************************************************************
 * USART - Universal Synchronous/Asynchronous Receiver Transmitter
 ***************************************************************************************************/
#define USART1_BASE (0x40000000UL + 0x00013800UL)
#define USART2_BASE (0x40000000UL + 0x00004400UL)
#define USART3_BASE (0x40000000UL + 0x00004800UL)
#define USART4_BASE (0x40000000UL + 0x00004C00UL)
#define USART5_BASE (0x40000000UL + 0x00005000UL)
#define LPUART1_BASE (0x40000000UL + 0x00008000UL)
#define LPUART2_BASE (0x40000000UL + 0x00008400UL)

/***************************************************************************************************
 * I2C - Inter-integrated Circuit Interface
 ***************************************************************************************************/
#define I2C1_BASE (0x40000000UL + 0x00005400UL)
#define I2C2_BASE (0x40000000UL + 0x00005800UL)
#define I2C3_BASE (0x40000000UL + 0x00008800UL)

/***************************************************************************************************
 * GPIO - General Purpose Input/Output Base Addresses
 ***************************************************************************************************/
#define GPIOA_BASE 0x50000000UL
#define GPIOB_BASE 0x50000400UL
#define GPIOC_BASE 0x50000800UL
#define GPIOD_BASE 0x50000C00UL
#define GPIOE_BASE 0x50001000UL
#define GPIOF_BASE 0x50001400UL

/*************************************************************************************************************************************************
 * NVIC - Nested vectored interrupt controller
 *************************************************************************************************************************************************/
typedef struct
{
  volatile uint32_t ISER[16U]; /* Offset: 0x000 (R/W)  Interrupt Set Enable Register */
  uint32_t RESERVED0[16U];
  volatile uint32_t ICER[16U]; /* Offset: 0x080 (R/W)  Interrupt Clear Enable Register */
  uint32_t RSERVED1[16U];
  volatile uint32_t ISPR[16U]; /* Offset: 0x100 (R/W)  Interrupt Set Pending Register */
  uint32_t RESERVED2[16U];
  volatile uint32_t ICPR[16U]; /* Offset: 0x180 (R/W)  Interrupt Clear Pending Register */
  uint32_t RESERVED3[16U];
  volatile uint32_t IABR[16U]; /* Offset: 0x200 (R/W)  Interrupt Active bit Register */
  uint32_t RESERVED4[16U];
  volatile uint32_t ITNS[16U]; /* Offset: 0x280 (R/W)  Interrupt Non-Secure State Register */
  uint32_t RESERVED5[16U];
  volatile uint32_t IPR[124U]; /* Offset: 0x300 (R/W)  Interrupt Priority Register */
} NVIC_Type;

#define NVIC ((NVIC_Type *)NVIC_BASE) /* NVIC configuration struct */

/***************************************************************************************************
 * RCC - Reset and Clock Control
 ***************************************************************************************************/
typedef struct
{
  volatile uint32_t CR;        /*!< RCC Clock Sources Control Register,                                     Address offset: 0x00 */
  volatile uint32_t ICSCR;     /*!< RCC Internal Clock Sources Calibration Register,                        Address offset: 0x04 */
  volatile uint32_t CFGR;      /*!< RCC Regulated Domain Clocks Configuration Register,                     Address offset: 0x08 */
  volatile uint32_t PLLCFGR;   /*!< RCC System PLL configuration Register,                                  Address offset: 0x0C */
  volatile uint32_t RESERVED0; /*!< Reserved,                                                               Address offset: 0x10 */
  volatile uint32_t CRRCR;     /*!< RCC Clock Configuration Register,                                       Address offset: 0x14 */
  volatile uint32_t CIER;      /*!< RCC Clock Interrupt Enable Register,                                    Address offset: 0x18 */
  volatile uint32_t CIFR;      /*!< RCC Clock Interrupt Flag Register,                                      Address offset: 0x1C */
  volatile uint32_t CICR;      /*!< RCC Clock Interrupt Clear Register,                                     Address offset: 0x20 */
  volatile uint32_t IOPRSTR;   /*!< RCC IO port reset register,                                             Address offset: 0x24 */
  volatile uint32_t AHBRSTR;   /*!< RCC AHB peripherals reset register,                                     Address offset: 0x28 */
  volatile uint32_t APBRSTR1;  /*!< RCC APB peripherals reset register 1,                                   Address offset: 0x2C */
  volatile uint32_t APBRSTR2;  /*!< RCC APB peripherals reset register 2,                                   Address offset: 0x30 */
  volatile uint32_t IOPENR;    /*!< RCC IO port enable register,                                            Address offset: 0x34 */
  volatile uint32_t AHBENR;    /*!< RCC AHB peripherals clock enable register,                              Address offset: 0x38 */
  volatile uint32_t APBENR1;   /*!< RCC APB peripherals clock enable register1,                             Address offset: 0x3C */
  volatile uint32_t APBENR2;   /*!< RCC APB peripherals clock enable register2,                             Address offset: 0x40 */
  volatile uint32_t IOPSMENR;  /*!< RCC IO port clocks enable in sleep mode register,                       Address offset: 0x44 */
  volatile uint32_t AHBSMENR;  /*!< RCC AHB peripheral clocks enable in sleep mode register,                Address offset: 0x48 */
  volatile uint32_t APBSMENR1; /*!< RCC APB peripheral clocks enable in sleep mode register1,               Address offset: 0x4C */
  volatile uint32_t APBSMENR2; /*!< RCC APB peripheral clocks enable in sleep mode register2,               Address offset: 0x50 */
  volatile uint32_t CCIPR;     /*!< RCC Peripherals Independent Clocks Configuration Register,              Address offset: 0x54 */
  volatile uint32_t CCIPR2;    /*!< RCC Peripherals Independent Clocks Configuration Register2,             Address offset: 0x58 */
  volatile uint32_t BDCR;      /*!< RCC Backup Domain Control Register,                                     Address offset: 0x5C */
  volatile uint32_t CSR;       /*!< RCC Unregulated Domain Clock Control and Status Register,               Address offset: 0x60 */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/***************************************************************************************************
 * SYSCFG - System configuration controller
 ***************************************************************************************************/
typedef struct
{
  volatile uint32_t CFGR1;          /*!< SYSCFG configuration register 1,                   Address offset: 0x00 */
  uint32_t RESERVED0[5];            /*!< Reserved,                                                   0x04 --0x14 */
  volatile uint32_t CFGR2;          /*!< SYSCFG configuration register 2,                   Address offset: 0x18 */
  uint32_t RESERVED1[25];           /*!< Reserved                                                           0x1C */
  volatile uint32_t IT_LINE_SR[32]; /*!< SYSCFG configuration IT_LINE register,             Address offset: 0x80 */
} SYSCFG_TypeDef;

#define SYSCFG ((SYSCFG_TypeDef *)SYSCFG_BASE)

/***************************************************************************************************
 * EXTI - Asynch Interrupt/Event Controller
 ***************************************************************************************************/
typedef struct
{
  volatile uint32_t RTSR1;     /*!< EXTI Rising Trigger Selection Register 1,        Address offset:   0x00 */
  volatile uint32_t FTSR1;     /*!< EXTI Falling Trigger Selection Register 1,       Address offset:   0x04 */
  volatile uint32_t SWIER1;    /*!< EXTI Software Interrupt event Register 1,        Address offset:   0x08 */
  volatile uint32_t RPR1;      /*!< EXTI Rising Pending Register 1,                  Address offset:   0x0C */
  volatile uint32_t FPR1;      /*!< EXTI Falling Pending Register 1,                 Address offset:   0x10 */
  uint32_t RESERVED1[3];       /*!< Reserved 1,                                                0x14 -- 0x1C */
  volatile uint32_t RTSR2;     /*!< EXTI Rising Trigger Selection Register 2,        Address offset:   0x20 */
  volatile uint32_t FTSR2;     /*!< EXTI Falling Trigger Selection Register 2,       Address offset:   0x24 */
  volatile uint32_t SWIER2;    /*!< EXTI Software Interrupt event Register 2,        Address offset:   0x28 */
  volatile uint32_t RPR2;      /*!< EXTI Rising Pending Register 2,                  Address offset:   0x2C */
  volatile uint32_t FPR2;      /*!< EXTI Falling Pending Register 2,                 Address offset:   0x30 */
  uint32_t RESERVED3[11];      /*!< Reserved 3,                                                0x34 -- 0x5C */
  volatile uint32_t EXTICR[4]; /*!< EXTI External Interrupt Configuration Register,            0x60 -- 0x6C */
  uint32_t RESERVED4[4];       /*!< Reserved 4,                                                0x70 -- 0x7C */
  volatile uint32_t IMR1;      /*!< EXTI Interrupt Mask Register 1,                  Address offset:   0x80 */
  volatile uint32_t EMR1;      /*!< EXTI Event Mask Register 1,                      Address offset:   0x84 */
  uint32_t RESERVED5[2];       /*!< Reserved 5,                                                0x88 -- 0x8C */
  volatile uint32_t IMR2;      /*!< EXTI Interrupt Mask Register 2,                  Address offset:   0x90 */
  volatile uint32_t EMR2;      /*!< EXTI Event Mask Register 2,                      Address offset:   0x94 */
} EXTI_TypeDef;

#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

/***************************************************************************************************
 * USART - Universal Synchronous/Asynchronous Receiver Transmitter
 ***************************************************************************************************/
typedef struct
{
  volatile uint32_t CR1;   /*!< USART Control register 1,                 Address offset: 0x00  */
  volatile uint32_t CR2;   /*!< USART Control register 2,                 Address offset: 0x04  */
  volatile uint32_t CR3;   /*!< USART Control register 3,                 Address offset: 0x08  */
  volatile uint32_t BRR;   /*!< USART Baud rate register,                 Address offset: 0x0C  */
  volatile uint32_t GTPR;  /*!< USART Guard time and prescaler register,  Address offset: 0x10  */
  volatile uint32_t RTOR;  /*!< USART Receiver Time Out register,         Address offset: 0x14  */
  volatile uint32_t RQR;   /*!< USART Request register,                   Address offset: 0x18  */
  volatile uint32_t ISR;   /*!< USART Interrupt and status register,      Address offset: 0x1C  */
  volatile uint32_t ICR;   /*!< USART Interrupt flag Clear register,      Address offset: 0x20  */
  volatile uint32_t RDR;   /*!< USART Receive Data register,              Address offset: 0x24  */
  volatile uint32_t TDR;   /*!< USART Transmit Data register,             Address offset: 0x28  */
  volatile uint32_t PRESC; /*!< USART Prescaler register,                 Address offset: 0x2C  */
} USART_TypeDef;

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART2 ((USART_TypeDef *)USART2_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)
#define USART4 ((USART_TypeDef *)USART4_BASE)
#define USART5 ((USART_TypeDef *)USART5_BASE)
#define USART6 ((USART_TypeDef *)USART6_BASE)

#define LPUART1 ((USART_TypeDef *)LPUART1_BASE)
#define LPUART2 ((USART_TypeDef *)LPUART2_BASE)

/***************************************************************************************************
 * I2C - Inter-integrated Circuit Interface
 ***************************************************************************************************/

typedef struct
{
  volatile uint32_t CR1;      /*!< I2C Control register 1,            Address offset: 0x00 */
  volatile uint32_t CR2;      /*!< I2C Control register 2,            Address offset: 0x04 */
  volatile uint32_t OAR1;     /*!< I2C Own address 1 register,        Address offset: 0x08 */
  volatile uint32_t OAR2;     /*!< I2C Own address 2 register,        Address offset: 0x0C */
  volatile uint32_t TIMINGR;  /*!< I2C Timing register,               Address offset: 0x10 */
  volatile uint32_t TIMEOUTR; /*!< I2C Timeout register,              Address offset: 0x14 */
  volatile uint32_t ISR;      /*!< I2C Interrupt and status register, Address offset: 0x18 */
  volatile uint32_t ICR;      /*!< I2C Interrupt clear register,      Address offset: 0x1C */
  volatile uint32_t PECR;     /*!< I2C PEC register,                  Address offset: 0x20 */
  volatile uint32_t RXDR;     /*!< I2C Receive data register,         Address offset: 0x24 */
  volatile uint32_t TXDR;     /*!< I2C Transmit data register,        Address offset: 0x28 */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C2 ((I2C_TypeDef *)I2C2_BASE)
#define I2C3 ((I2C_TypeDef *)I2C3_BASE)

/***************************************************************************************************
 * GPIO - common structure for multiple GPIOs
 ***************************************************************************************************/
typedef struct
{
  volatile uint32_t MODER;   /* GPIO port mode register,               Address offset: 0x00      */
  volatile uint32_t OTYPER;  /* GPIO port output type register,        Address offset: 0x04      */
  volatile uint32_t OSPEEDR; /* GPIO port output speed register,       Address offset: 0x08      */
  volatile uint32_t PUPDR;   /* GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
  volatile uint32_t IDR;     /* GPIO port input data register,         Address offset: 0x10      */
  volatile uint32_t ODR;     /* GPIO port output data register,        Address offset: 0x14      */
  volatile uint32_t BSRR;    /* GPIO port bit set/reset register,      Address offset: 0x18      */
  volatile uint32_t LCKR;    /* GPIO port configuration lock register, Address offset: 0x1C      */
  union {
    volatile uint32_t AFR[2];  // AFR[0] = AFRL, AFR[1] = AFRH

    struct
    {
      volatile uint32_t AFRL;  // Offset 0x20
      volatile uint32_t AFRH;  // Offset 0x24
    };
  };
  volatile uint32_t BRR; /* GPIO Bit Reset register,               Address offset: 0x28      */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF ((GPIO_TypeDef *)GPIOF_BASE)

typedef struct
{
  volatile uint32_t ACR;       /*!< FLASH Access Control register,                     Address offset: 0x00 */
  uint32_t RESERVED1;          /*!< Reserved1,                                         Address offset: 0x04 */
  volatile uint32_t KEYR;      /*!< FLASH Key register,                                Address offset: 0x08 */
  volatile uint32_t OPTKEYR;   /*!< FLASH Option Key register,                         Address offset: 0x0C */
  volatile uint32_t SR;        /*!< FLASH Status register,                             Address offset: 0x10 */
  volatile uint32_t CR;        /*!< FLASH Control register,                            Address offset: 0x14 */
  volatile uint32_t ECCR;      /*!< FLASH ECC bank 1 register,                         Address offset: 0x18 */
  volatile uint32_t ECC2R;     /*!< FLASH ECC bank 2 register,                         Address offset: 0x1C */
  volatile uint32_t OPTR;      /*!< FLASH Option register,                             Address offset: 0x20 */
  volatile uint32_t PCROP1ASR; /*!< FLASH Bank PCROP area A Start address register,    Address offset: 0x24 */
  volatile uint32_t PCROP1AER; /*!< FLASH Bank PCROP area A End address register,      Address offset: 0x28 */
  volatile uint32_t WRP1AR;    /*!< FLASH Bank WRP area A address register,            Address offset: 0x2C */
  volatile uint32_t WRP1BR;    /*!< FLASH Bank WRP area B address register,            Address offset: 0x30 */
  volatile uint32_t PCROP1BSR; /*!< FLASH Bank PCROP area B Start address register,    Address offset: 0x34 */
  volatile uint32_t PCROP1BER; /*!< FLASH Bank PCROP area B End address register,      Address offset: 0x38 */
  uint32_t RESERVED5[2];       /*!< Reserved5,                                         Address offset: 0x3C--0x40 */
  volatile uint32_t PCROP2ASR; /*!< FLASH Bank2 PCROP area A Start address register,   Address offset: 0x44 */
  volatile uint32_t PCROP2AER; /*!< FLASH Bank2 PCROP area A End address register,     Address offset: 0x48 */
  volatile uint32_t WRP2AR;    /*!< FLASH Bank2 WRP area A address register,           Address offset: 0x4C */
  volatile uint32_t WRP2BR;    /*!< FLASH Bank2 WRP area B address register,           Address offset: 0x50 */
  volatile uint32_t PCROP2BSR; /*!< FLASH Bank2 PCROP area B Start address register,   Address offset: 0x54 */
  volatile uint32_t PCROP2BER; /*!< FLASH Bank2 PCROP area B End address register,     Address offset: 0x58 */
  uint32_t RESERVED7[9];       /*!< Reserved7,                                         Address offset: 0x5C--0x7C */
  volatile uint32_t SECR;      /*!< FLASH security register ,                          Address offset: 0x80 */
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)FLASH_REGISTERS_BASE)

/***************************************************************************************************
 * PWR - Power registers
 ***************************************************************************************************/

typedef struct
{
  volatile uint32_t CR1;   /*!< PWR Power Control Register 1,                     Address offset: 0x00 */
  volatile uint32_t CR2;   /*!< PWR Power Control Register 2,                     Address offset: 0x04 */
  volatile uint32_t CR3;   /*!< PWR Power Control Register 3,                     Address offset: 0x08 */
  volatile uint32_t CR4;   /*!< PWR Power Control Register 4,                     Address offset: 0x0C */
  volatile uint32_t SR1;   /*!< PWR Power Status Register 1,                      Address offset: 0x10 */
  volatile uint32_t SR2;   /*!< PWR Power Status Register 2,                      Address offset: 0x14 */
  volatile uint32_t SCR;   /*!< PWR Power Status Clear Register,                  Address offset: 0x18 */
  uint32_t RESERVED1;      /*!< Reserved,                                         Address offset: 0x1C */
  volatile uint32_t PUCRA; /*!< PWR Pull-Up Control Register of port A,           Address offset: 0x20 */
  volatile uint32_t PDCRA; /*!< PWR Pull-Down Control Register of port A,         Address offset: 0x24 */
  volatile uint32_t PUCRB; /*!< PWR Pull-Up Control Register of port B,           Address offset: 0x28 */
  volatile uint32_t PDCRB; /*!< PWR Pull-Down Control Register of port B,         Address offset: 0x2C */
  volatile uint32_t PUCRC; /*!< PWR Pull-Up Control Register of port C,           Address offset: 0x30 */
  volatile uint32_t PDCRC; /*!< PWR Pull-Down Control Register of port C,         Address offset: 0x34 */
  volatile uint32_t PUCRD; /*!< PWR Pull-Up Control Register of port D,           Address offset: 0x38 */
  volatile uint32_t PDCRD; /*!< PWR Pull-Down Control Register of port D,         Address offset: 0x3C */
  volatile uint32_t PUCRE; /*!< PWR Pull-Up Control Register of port E,           Address offset: 0x40 */
  volatile uint32_t PDCRE; /*!< PWR Pull-Down Control Register of port E,         Address offset: 0x44 */
  volatile uint32_t PUCRF; /*!< PWR Pull-Up Control Register of port F,           Address offset: 0x48 */
  volatile uint32_t PDCRF; /*!< PWR Pull-Down Control Register of port F,         Address offset: 0x4C */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)

/***************************************************************************************************
 * TIM - Timer registers
 ***************************************************************************************************/

typedef struct
{
  volatile uint32_t CR1;   /*!< TIM control register 1,                   Address offset: 0x00 */
  volatile uint32_t CR2;   /*!< TIM control register 2,                   Address offset: 0x04 */
  volatile uint32_t SMCR;  /*!< TIM slave mode control register,          Address offset: 0x08 */
  volatile uint32_t DIER;  /*!< TIM DMA/interrupt enable register,        Address offset: 0x0C */
  volatile uint32_t SR;    /*!< TIM status register,                      Address offset: 0x10 */
  volatile uint32_t EGR;   /*!< TIM event generation register,            Address offset: 0x14 */
  volatile uint32_t CCMR1; /*!< TIM capture/compare mode register 1,      Address offset: 0x18 */
  volatile uint32_t CCMR2; /*!< TIM capture/compare mode register 2,      Address offset: 0x1C */
  volatile uint32_t CCER;  /*!< TIM capture/compare enable register,      Address offset: 0x20 */
  volatile uint32_t CNT;   /*!< TIM counter register,                     Address offset: 0x24 */
  volatile uint32_t PSC;   /*!< TIM prescaler register,                   Address offset: 0x28 */
  volatile uint32_t ARR;   /*!< TIM auto-reload register,                 Address offset: 0x2C */
  volatile uint32_t RCR;   /*!< TIM repetition counter register,          Address offset: 0x30 */
  volatile uint32_t CCR1;  /*!< TIM capture/compare register 1,           Address offset: 0x34 */
  volatile uint32_t CCR2;  /*!< TIM capture/compare register 2,           Address offset: 0x38 */
  volatile uint32_t CCR3;  /*!< TIM capture/compare register 3,           Address offset: 0x3C */
  volatile uint32_t CCR4;  /*!< TIM capture/compare register 4,           Address offset: 0x40 */
  volatile uint32_t BDTR;  /*!< TIM break and dead-time register,         Address offset: 0x44 */
  volatile uint32_t DCR;   /*!< TIM DMA control register,                 Address offset: 0x48 */
  volatile uint32_t DMAR;  /*!< TIM DMA address for full transfer,        Address offset: 0x4C */
  volatile uint32_t OR1;   /*!< TIM option register,                      Address offset: 0x50 */
  volatile uint32_t CCMR3; /*!< TIM capture/compare mode register 3,      Address offset: 0x54 */
  volatile uint32_t CCR5;  /*!< TIM capture/compare register5,            Address offset: 0x58 */
  volatile uint32_t CCR6;  /*!< TIM capture/compare register6,            Address offset: 0x5C */
  volatile uint32_t AF1;   /*!< TIM alternate function register 1,        Address offset: 0x60 */
  volatile uint32_t AF2;   /*!< TIM alternate function register 2,        Address offset: 0x64 */
  volatile uint32_t TISEL; /*!< TIM Input Selection register,             Address offset: 0x68 */
} TIM_TypeDef;

#define TIM1 ((TIM_TypeDef *)TIM1_BASE)
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM3 ((TIM_TypeDef *)TIM3_BASE)
#define TIM4 ((TIM_TypeDef *)TIM4_BASE)
#define TIM6 ((TIM_TypeDef *)TIM6_BASE)
#define TIM7 ((TIM_TypeDef *)TIM7_BASE)
#define TIM14 ((TIM_TypeDef *)TIM14_BASE)
#define TIM15 ((TIM_TypeDef *)TIM15_BASE)
#define TIM16 ((TIM_TypeDef *)TIM16_BASE)
#define TIM17 ((TIM_TypeDef *)TIM17_BASE)

#endif  // __MEMORY_MAP_H__