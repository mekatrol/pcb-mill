#ifndef __REGISTER_BITS_H__
#define __REGISTER_BITS_H__

#define BIT_00_POS 0
#define BIT_00 (1 << BIT_00_POS)
#define BIT_01_POS 1
#define BIT_01 (1 << BIT_01_POS)
#define BIT_02_POS 2
#define BIT_02 (1 << BIT_02_POS)
#define BIT_03_POS 3
#define BIT_03 (1 << BIT_03_POS)
#define BIT_04_POS 4
#define BIT_04 (1 << BIT_04_POS)
#define BIT_05_POS 5
#define BIT_05 (1 << BIT_05_POS)
#define BIT_06_POS 6
#define BIT_06 (1 << BIT_06_POS)
#define BIT_07_POS 7
#define BIT_07 (1 << BIT_07_POS)
#define BIT_08_POS 8
#define BIT_08 (1 << BIT_08_POS)
#define BIT_09_POS 9
#define BIT_09 (1 << BIT_09_POS)
#define BIT_10_POS 10
#define BIT_10 (1 << BIT_10_POS)
#define BIT_11_POS 11
#define BIT_11 (1 << BIT_11_POS)
#define BIT_12_POS 12
#define BIT_12 (1 << BIT_12_POS)
#define BIT_13_POS 13
#define BIT_13 (1 << BIT_13_POS)
#define BIT_14_POS 14
#define BIT_14 (1 << BIT_14_POS)
#define BIT_15_POS 15
#define BIT_15 (1 << BIT_15_POS)

#define __NVIC_PRIO_BITS 2U /*!< STM32G0xx uses 2 Bits for the Priority Levels */

/*
 * MODER = Port mode register
 */
#define MODER_BIT_COUNT 2U  // 2 bits per MODER port configuration
#define MODER_MSK 0b11
#define MODER_INP 0b00
#define MODER_OUT 0b01
#define MODER_ALT 0b10
#define MODER_ANA 0b11

/*
 * AF = Alternate function
 */
#define GPIO_AF_BIT_COUNT 4U  // 4 bits per AF port configuration
#define GPIO_AF_MSK 0x0F
#define GPIO_AF0 0x00
#define GPIO_AF1 0x01
#define GPIO_AF2 0x02
#define GPIO_AF3 0x03
#define GPIO_AF4 0x04
#define GPIO_AF5 0x05
#define GPIO_AF6 0x06
#define GPIO_AF7 0x07

/*
 * Helper macros
 */
#define GPIO_SET_MODE(PORT, PIN_POS, MODE)                          \
  do {                                                              \
    (PORT)->MODER &= ~(MODER_MSK << ((PIN_POS) * MODER_BIT_COUNT)); \
    (PORT)->MODER |= ((MODE) << ((PIN_POS) * MODER_BIT_COUNT));     \
  } while (0)

/*
 * OSPEED = Output speed register
 */
#define OSPEEDR_BIT_COUNT 2U  // 2 bits per MODER port configuration
#define OSPEEDR_MSK 0b11      // Mask
#define OSPEEDR_VL 0b00       // Very low
#define OSPEEDR_LOW 0b01      // Low
#define OSPEEDR_HIGH 0b10     // High
#define OSPEEDR_VH 0b11       // Very high

#define IOPENR_PORTA_ENABLE (1 << 0)
#define IOPENR_PORTB_ENABLE (1 << 1)
#define IOPENR_PORTC_ENABLE (1 << 2)
#define IOPENR_PORTD_ENABLE (1 << 3)
#define IOPENR_PORTE_ENABLE (1 << 4)
#define IOPENR_PORTF_ENABLE (1 << 5)

#define RCC_APBENR1_TIM2EN (1 << 0)
#define RCC_APBENR1_TIM3EN (1 << 1)
#define RCC_APBENR1_TIM4EN (1 << 2)
#define RCC_APBENR1_TIM6EN (1 << 4)
#define RCC_APBENR1_TIM7EN (1 << 5)
#define RCC_APBENR1_LPUART2EN (1 << 7)
#define RCC_APBENR1_USART2EN (1 << 17)
#define RCC_APBENR1_USART3EN (1 << 18)
#define RCC_APBENR1_USART4EN (1 << 19)
#define RCC_APBENR1_LPUART1EN (1 << 20)
#define RCC_APBENR1_I2C1EN (1 << 21)
#define RCC_APBENR1_I2C2EN (1 << 22)
#define RCC_APBENR1_I2C3EN (1 << 23)

#define RCC_APBENR2_SYSCFGEN (1 < 0)
#define RCC_APBENR2_TIM14EN (1 << 15)

/* SysTick Control / Status Register Definitions */
#define SysTick_CTRL_COUNTFLAG_Pos 16U                                 /*!< SysTick CTRL: COUNTFLAG Position */
#define SysTick_CTRL_COUNTFLAG_Msk (1UL << SysTick_CTRL_COUNTFLAG_Pos) /*!< SysTick CTRL: COUNTFLAG Mask */

#define SysTick_CTRL_CLKSOURCE_Pos 2U                                  /*!< SysTick CTRL: CLKSOURCE Position */
#define SysTick_CTRL_CLKSOURCE_Msk (1UL << SysTick_CTRL_CLKSOURCE_Pos) /*!< SysTick CTRL: CLKSOURCE Mask */

#define SysTick_CTRL_TICKINT_Pos 1U                                /*!< SysTick CTRL: TICKINT Position */
#define SysTick_CTRL_TICKINT_Msk (1UL << SysTick_CTRL_TICKINT_Pos) /*!< SysTick CTRL: TICKINT Mask */

#define SysTick_CTRL_ENABLE_Pos 0U                                   /*!< SysTick CTRL: ENABLE Position */
#define SysTick_CTRL_ENABLE_Msk (1UL /*<< SysTick_CTRL_ENABLE_Pos*/) /*!< SysTick CTRL: ENABLE Mask */

/* SysTick Reload Register Definitions */
#define SysTick_LOAD_RELOAD_Pos 0U                                          /*!< SysTick LOAD: RELOAD Position */
#define SysTick_LOAD_RELOAD_Msk (0xFFFFFFUL /*<< SysTick_LOAD_RELOAD_Pos*/) /*!< SysTick LOAD: RELOAD Mask */

/* SysTick Current Register Definitions */
#define SysTick_VAL_CURRENT_Pos 0U                                          /*!< SysTick VAL: CURRENT Position */
#define SysTick_VAL_CURRENT_Msk (0xFFFFFFUL /*<< SysTick_VAL_CURRENT_Pos*/) /*!< SysTick VAL: CURRENT Mask */

/* SysTick Calibration Register Definitions */
#define SysTick_CALIB_NOREF_Pos 31U                              /*!< SysTick CALIB: NOREF Position */
#define SysTick_CALIB_NOREF_Msk (1UL << SysTick_CALIB_NOREF_Pos) /*!< SysTick CALIB: NOREF Mask */

#define SysTick_CALIB_SKEW_Pos 30U                             /*!< SysTick CALIB: SKEW Position */
#define SysTick_CALIB_SKEW_Msk (1UL << SysTick_CALIB_SKEW_Pos) /*!< SysTick CALIB: SKEW Mask */

#define SysTick_CALIB_TENMS_Pos 0U                                          /*!< SysTick CALIB: TENMS Position */
#define SysTick_CALIB_TENMS_Msk (0xFFFFFFUL /*<< SysTick_CALIB_TENMS_Pos*/) /*!< SysTick CALIB: TENMS Mask */

#define TIM_CR1_CEN (1 << 0)   //
#define TIM_CR1_OPM (1 << 3)   // One-pulse mode
#define TIM_EGR_UG (1 << 0)    //
#define TIM_SR_UIF (1 << 0)    //
#define TIM_DIER_UIE (1 << 0)  //

/******************************************************************************
 *      Universal Synchronous Asynchronous Receiver Transmitter (USART)       *
 ******************************************************************************/
#define USART_BRR(PCLK, BAUD) (((PCLK) + ((BAUD) / 2)) / (BAUD))

/******************  Bit definition for USART_CR1 register  *******************/
#define USART_CR1_UE_Pos (0U)
#define USART_CR1_UE_Msk (0x1UL << USART_CR1_UE_Pos)                         /* 0x00000001 */
#define USART_CR1_UE USART_CR1_UE_Msk                                        /* USART Enable */
#define USART_CR1_UESM_Pos (1U)                                              //
#define USART_CR1_UESM_Msk (0x1UL << USART_CR1_UESM_Pos)                     /* 0x00000002 */
#define USART_CR1_UESM USART_CR1_UESM_Msk                                    /* USART Enable in STOP Mode */
#define USART_CR1_RE_Pos (2U)                                                //
#define USART_CR1_RE_Msk (0x1UL << USART_CR1_RE_Pos)                         /* 0x00000004 */
#define USART_CR1_RE USART_CR1_RE_Msk                                        /* Receiver Enable */
#define USART_CR1_TE_Pos (3U)                                                //
#define USART_CR1_TE_Msk (0x1UL << USART_CR1_TE_Pos)                         /* 0x00000008 */
#define USART_CR1_TE USART_CR1_TE_Msk                                        /* Transmitter Enable */
#define USART_CR1_IDLEIE_Pos (4U)                                            //
#define USART_CR1_IDLEIE_Msk (0x1UL << USART_CR1_IDLEIE_Pos)                 /* 0x00000010 */
#define USART_CR1_IDLEIE USART_CR1_IDLEIE_Msk                                /* IDLE Interrupt Enable */
#define USART_CR1_RXNEIE_RXFNEIE_Pos (5U)                                    //
#define USART_CR1_RXNEIE_RXFNEIE_Msk (0x1UL << USART_CR1_RXNEIE_RXFNEIE_Pos) /* 0x00000020 */
#define USART_CR1_RXNEIE_RXFNEIE USART_CR1_RXNEIE_RXFNEIE_Msk                /* RXNE/RXFIFO not empty Interrupt Enable */
#define USART_CR1_TCIE_Pos (6U)                                              //
#define USART_CR1_TCIE_Msk (0x1UL << USART_CR1_TCIE_Pos)                     /* 0x00000040 */
#define USART_CR1_TCIE USART_CR1_TCIE_Msk                                    /* Transmission Complete Interrupt Enable */
#define USART_CR1_TXEIE_TXFNFIE_Pos (7U)                                     //
#define USART_CR1_TXEIE_TXFNFIE_Msk (0x1UL << USART_CR1_TXEIE_TXFNFIE_Pos)   /* 0x00000080 */
#define USART_CR1_TXEIE_TXFNFIE USART_CR1_TXEIE_TXFNFIE_Msk                  /* TXE/TXFIFO not full Interrupt Enable */
#define USART_CR1_PEIE_Pos (8U)                                              //
#define USART_CR1_PEIE_Msk (0x1UL << USART_CR1_PEIE_Pos)                     /* 0x00000100 */
#define USART_CR1_PEIE USART_CR1_PEIE_Msk                                    /* PE Interrupt Enable */
#define USART_CR1_PS_Pos (9U)                                                //
#define USART_CR1_PS_Msk (0x1UL << USART_CR1_PS_Pos)                         /* 0x00000200 */
#define USART_CR1_PS USART_CR1_PS_Msk                                        /* Parity Selection */
#define USART_CR1_PCE_Pos (10U)                                              //
#define USART_CR1_PCE_Msk (0x1UL << USART_CR1_PCE_Pos)                       /* 0x00000400 */
#define USART_CR1_PCE USART_CR1_PCE_Msk                                      /* Parity Control Enable */
#define USART_CR1_WAKE_Pos (11U)                                             //
#define USART_CR1_WAKE_Msk (0x1UL << USART_CR1_WAKE_Pos)                     /* 0x00000800 */
#define USART_CR1_WAKE USART_CR1_WAKE_Msk                                    /* Receiver Wakeup method */
#define USART_CR1_M_Pos (12U)                                                //
#define USART_CR1_M_Msk (0x10001UL << USART_CR1_M_Pos)                       /* 0x10001000 */
#define USART_CR1_M USART_CR1_M_Msk                                          /* Word length */
#define USART_CR1_M0_Pos (12U)                                               //
#define USART_CR1_M0_Msk (0x1UL << USART_CR1_M0_Pos)                         /* 0x00001000 */
#define USART_CR1_M0 USART_CR1_M0_Msk                                        /* Word length - Bit 0 */
#define USART_CR1_MME_Pos (13U)                                              //
#define USART_CR1_MME_Msk (0x1UL << USART_CR1_MME_Pos)                       /* 0x00002000 */
#define USART_CR1_MME USART_CR1_MME_Msk                                      /* Mute Mode Enable */
#define USART_CR1_CMIE_Pos (14U)                                             //
#define USART_CR1_CMIE_Msk (0x1UL << USART_CR1_CMIE_Pos)                     /* 0x00004000 */
#define USART_CR1_CMIE USART_CR1_CMIE_Msk                                    /* Character match interrupt enable */
#define USART_CR1_OVER8_Pos (15U)                                            //
#define USART_CR1_OVER8_Msk (0x1UL << USART_CR1_OVER8_Pos)                   /* 0x00008000 */
#define USART_CR1_OVER8 USART_CR1_OVER8_Msk                                  /* Oversampling by 8-bit or 16-bit mode */
#define USART_CR1_DEDT_Pos (16U)                                             //
#define USART_CR1_DEDT_Msk (0x1FUL << USART_CR1_DEDT_Pos)                    /* 0x001F0000 */
#define USART_CR1_DEDT USART_CR1_DEDT_Msk                                    /* DEDT[4:0] bits (Driver Enable Deassertion Time) */
#define USART_CR1_DEDT_0 (0x01UL << USART_CR1_DEDT_Pos)                      /* 0x00010000 */
#define USART_CR1_DEDT_1 (0x02UL << USART_CR1_DEDT_Pos)                      /* 0x00020000 */
#define USART_CR1_DEDT_2 (0x04UL << USART_CR1_DEDT_Pos)                      /* 0x00040000 */
#define USART_CR1_DEDT_3 (0x08UL << USART_CR1_DEDT_Pos)                      /* 0x00080000 */
#define USART_CR1_DEDT_4 (0x10UL << USART_CR1_DEDT_Pos)                      /* 0x00100000 */
#define USART_CR1_DEAT_Pos (21U)                                             //
#define USART_CR1_DEAT_Msk (0x1FUL << USART_CR1_DEAT_Pos)                    /* 0x03E00000 */
#define USART_CR1_DEAT USART_CR1_DEAT_Msk                                    /* DEAT[4:0] bits (Driver Enable Assertion Time) */
#define USART_CR1_DEAT_0 (0x01UL << USART_CR1_DEAT_Pos)                      /* 0x00200000 */
#define USART_CR1_DEAT_1 (0x02UL << USART_CR1_DEAT_Pos)                      /* 0x00400000 */
#define USART_CR1_DEAT_2 (0x04UL << USART_CR1_DEAT_Pos)                      /* 0x00800000 */
#define USART_CR1_DEAT_3 (0x08UL << USART_CR1_DEAT_Pos)                      /* 0x01000000 */
#define USART_CR1_DEAT_4 (0x10UL << USART_CR1_DEAT_Pos)                      /* 0x02000000 */
#define USART_CR1_RTOIE_Pos (26U)                                            //
#define USART_CR1_RTOIE_Msk (0x1UL << USART_CR1_RTOIE_Pos)                   /* 0x04000000 */
#define USART_CR1_RTOIE USART_CR1_RTOIE_Msk                                  /* Receive Time Out interrupt enable */
#define USART_CR1_EOBIE_Pos (27U)                                            //
#define USART_CR1_EOBIE_Msk (0x1UL << USART_CR1_EOBIE_Pos)                   /* 0x08000000 */
#define USART_CR1_EOBIE USART_CR1_EOBIE_Msk                                  /* End of Block interrupt enable */
#define USART_CR1_M1_Pos (28U)                                               //
#define USART_CR1_M1_Msk (0x1UL << USART_CR1_M1_Pos)                         /* 0x10000000 */
#define USART_CR1_M1 USART_CR1_M1_Msk                                        /* Word length - Bit 1 */
#define USART_CR1_FIFOEN_Pos (29U)                                           //
#define USART_CR1_FIFOEN_Msk (0x1UL << USART_CR1_FIFOEN_Pos)                 /* 0x20000000 */
#define USART_CR1_FIFOEN USART_CR1_FIFOEN_Msk                                /* FIFO mode enable */
#define USART_CR1_TXFEIE_Pos (30U)                                           //
#define USART_CR1_TXFEIE_Msk (0x1UL << USART_CR1_TXFEIE_Pos)                 /* 0x40000000 */
#define USART_CR1_TXFEIE USART_CR1_TXFEIE_Msk                                /* TXFIFO empty interrupt enable */
#define USART_CR1_RXFFIE_Pos (31U)                                           //
#define USART_CR1_RXFFIE_Msk (0x1UL << USART_CR1_RXFFIE_Pos)                 /* 0x80000000 */
#define USART_CR1_RXFFIE USART_CR1_RXFFIE_Msk                                /* RXFIFO Full interrupt enable */
/******************  Bit definition for USART_CR2 register  *******************/
#define USART_CR2_SLVEN_Pos (0U)
#define USART_CR2_SLVEN_Msk (0x1UL << USART_CR2_SLVEN_Pos) /* 0x00000001 */
#define USART_CR2_SLVEN USART_CR2_SLVEN_Msk                /* Synchronous Slave mode enable */
#define USART_CR2_DIS_NSS_Pos (3U)
#define USART_CR2_DIS_NSS_Msk (0x1UL << USART_CR2_DIS_NSS_Pos) /* 0x00000008 */
#define USART_CR2_DIS_NSS USART_CR2_DIS_NSS_Msk                /* NSS input pin disable for SPI slave selection */
#define USART_CR2_ADDM7_Pos (4U)
#define USART_CR2_ADDM7_Msk (0x1UL << USART_CR2_ADDM7_Pos) /* 0x00000010 */
#define USART_CR2_ADDM7 USART_CR2_ADDM7_Msk                /* 7-bit or 4-bit Address Detection */
#define USART_CR2_LBDL_Pos (5U)
#define USART_CR2_LBDL_Msk (0x1UL << USART_CR2_LBDL_Pos) /* 0x00000020 */
#define USART_CR2_LBDL USART_CR2_LBDL_Msk                /* LIN Break Detection Length */
#define USART_CR2_LBDIE_Pos (6U)
#define USART_CR2_LBDIE_Msk (0x1UL << USART_CR2_LBDIE_Pos) /* 0x00000040 */
#define USART_CR2_LBDIE USART_CR2_LBDIE_Msk                /* LIN Break Detection Interrupt Enable */
#define USART_CR2_LBCL_Pos (8U)
#define USART_CR2_LBCL_Msk (0x1UL << USART_CR2_LBCL_Pos) /* 0x00000100 */
#define USART_CR2_LBCL USART_CR2_LBCL_Msk                /* Last Bit Clock pulse */
#define USART_CR2_CPHA_Pos (9U)
#define USART_CR2_CPHA_Msk (0x1UL << USART_CR2_CPHA_Pos) /* 0x00000200 */
#define USART_CR2_CPHA USART_CR2_CPHA_Msk                /* Clock Phase */
#define USART_CR2_CPOL_Pos (10U)
#define USART_CR2_CPOL_Msk (0x1UL << USART_CR2_CPOL_Pos) /* 0x00000400 */
#define USART_CR2_CPOL USART_CR2_CPOL_Msk                /* Clock Polarity */
#define USART_CR2_CLKEN_Pos (11U)
#define USART_CR2_CLKEN_Msk (0x1UL << USART_CR2_CLKEN_Pos) /* 0x00000800 */
#define USART_CR2_CLKEN USART_CR2_CLKEN_Msk                /* Clock Enable */
#define USART_CR2_STOP_Pos (12U)
#define USART_CR2_STOP_Msk (0x3UL << USART_CR2_STOP_Pos) /* 0x00003000 */
#define USART_CR2_STOP USART_CR2_STOP_Msk                /* STOP[1:0] bits (STOP bits) */
#define USART_CR2_STOP_0 (0x1UL << USART_CR2_STOP_Pos)   /* 0x00001000 */
#define USART_CR2_STOP_1 (0x2UL << USART_CR2_STOP_Pos)   /* 0x00002000 */
#define USART_CR2_LINEN_Pos (14U)
#define USART_CR2_LINEN_Msk (0x1UL << USART_CR2_LINEN_Pos) /* 0x00004000 */
#define USART_CR2_LINEN USART_CR2_LINEN_Msk                /* LIN mode enable */
#define USART_CR2_SWAP_Pos (15U)
#define USART_CR2_SWAP_Msk (0x1UL << USART_CR2_SWAP_Pos) /* 0x00008000 */
#define USART_CR2_SWAP USART_CR2_SWAP_Msk                /* SWAP TX/RX pins */
#define USART_CR2_RXINV_Pos (16U)
#define USART_CR2_RXINV_Msk (0x1UL << USART_CR2_RXINV_Pos) /* 0x00010000 */
#define USART_CR2_RXINV USART_CR2_RXINV_Msk                /* RX pin active level inversion */
#define USART_CR2_TXINV_Pos (17U)
#define USART_CR2_TXINV_Msk (0x1UL << USART_CR2_TXINV_Pos) /* 0x00020000 */
#define USART_CR2_TXINV USART_CR2_TXINV_Msk                /* TX pin active level inversion */
#define USART_CR2_DATAINV_Pos (18U)
#define USART_CR2_DATAINV_Msk (0x1UL << USART_CR2_DATAINV_Pos) /* 0x00040000 */
#define USART_CR2_DATAINV USART_CR2_DATAINV_Msk                /* Binary data inversion */
#define USART_CR2_MSBFIRST_Pos (19U)
#define USART_CR2_MSBFIRST_Msk (0x1UL << USART_CR2_MSBFIRST_Pos) /* 0x00080000 */
#define USART_CR2_MSBFIRST USART_CR2_MSBFIRST_Msk                /* Most Significant Bit First */
#define USART_CR2_ABREN_Pos (20U)
#define USART_CR2_ABREN_Msk (0x1UL << USART_CR2_ABREN_Pos) /* 0x00100000 */
#define USART_CR2_ABREN USART_CR2_ABREN_Msk                /* Auto Baud-Rate Enable*/
#define USART_CR2_ABRMODE_Pos (21U)
#define USART_CR2_ABRMODE_Msk (0x3UL << USART_CR2_ABRMODE_Pos) /* 0x00600000 */
#define USART_CR2_ABRMODE USART_CR2_ABRMODE_Msk                /* ABRMOD[1:0] bits (Auto Baud-Rate Mode) */
#define USART_CR2_ABRMODE_0 (0x1UL << USART_CR2_ABRMODE_Pos)   /* 0x00200000 */
#define USART_CR2_ABRMODE_1 (0x2UL << USART_CR2_ABRMODE_Pos)   /* 0x00400000 */
#define USART_CR2_RTOEN_Pos (23U)
#define USART_CR2_RTOEN_Msk (0x1UL << USART_CR2_RTOEN_Pos) /* 0x00800000 */
#define USART_CR2_RTOEN USART_CR2_RTOEN_Msk                /* Receiver Time-Out enable */
#define USART_CR2_ADD_Pos (24U)
#define USART_CR2_ADD_Msk (0xFFUL << USART_CR2_ADD_Pos) /* 0xFF000000 */
#define USART_CR2_ADD USART_CR2_ADD_Msk                 /* Address of the USART node */

/******************  Bit definition for USART_CR3 register  *******************/
#define USART_CR3_EIE_Pos (0U)
#define USART_CR3_EIE_Msk (0x1UL << USART_CR3_EIE_Pos) /* 0x00000001 */
#define USART_CR3_EIE USART_CR3_EIE_Msk                /* Error Interrupt Enable */
#define USART_CR3_IREN_Pos (1U)
#define USART_CR3_IREN_Msk (0x1UL << USART_CR3_IREN_Pos) /* 0x00000002 */
#define USART_CR3_IREN USART_CR3_IREN_Msk                /* IrDA mode Enable */
#define USART_CR3_IRLP_Pos (2U)
#define USART_CR3_IRLP_Msk (0x1UL << USART_CR3_IRLP_Pos) /* 0x00000004 */
#define USART_CR3_IRLP USART_CR3_IRLP_Msk                /* IrDA Low-Power */
#define USART_CR3_HDSEL_Pos (3U)
#define USART_CR3_HDSEL_Msk (0x1UL << USART_CR3_HDSEL_Pos) /* 0x00000008 */
#define USART_CR3_HDSEL USART_CR3_HDSEL_Msk                /* Half-Duplex Selection */
#define USART_CR3_NACK_Pos (4U)
#define USART_CR3_NACK_Msk (0x1UL << USART_CR3_NACK_Pos) /* 0x00000010 */
#define USART_CR3_NACK USART_CR3_NACK_Msk                /* SmartCard NACK enable */
#define USART_CR3_SCEN_Pos (5U)
#define USART_CR3_SCEN_Msk (0x1UL << USART_CR3_SCEN_Pos) /* 0x00000020 */
#define USART_CR3_SCEN USART_CR3_SCEN_Msk                /* SmartCard mode enable */
#define USART_CR3_DMAR_Pos (6U)
#define USART_CR3_DMAR_Msk (0x1UL << USART_CR3_DMAR_Pos) /* 0x00000040 */
#define USART_CR3_DMAR USART_CR3_DMAR_Msk                /* DMA Enable Receiver */
#define USART_CR3_DMAT_Pos (7U)
#define USART_CR3_DMAT_Msk (0x1UL << USART_CR3_DMAT_Pos) /* 0x00000080 */
#define USART_CR3_DMAT USART_CR3_DMAT_Msk                /* DMA Enable Transmitter */
#define USART_CR3_RTSE_Pos (8U)
#define USART_CR3_RTSE_Msk (0x1UL << USART_CR3_RTSE_Pos) /* 0x00000100 */
#define USART_CR3_RTSE USART_CR3_RTSE_Msk                /* RTS Enable */
#define USART_CR3_CTSE_Pos (9U)
#define USART_CR3_CTSE_Msk (0x1UL << USART_CR3_CTSE_Pos) /* 0x00000200 */
#define USART_CR3_CTSE USART_CR3_CTSE_Msk                /* CTS Enable */
#define USART_CR3_CTSIE_Pos (10U)
#define USART_CR3_CTSIE_Msk (0x1UL << USART_CR3_CTSIE_Pos) /* 0x00000400 */
#define USART_CR3_CTSIE USART_CR3_CTSIE_Msk                /* CTS Interrupt Enable */
#define USART_CR3_ONEBIT_Pos (11U)
#define USART_CR3_ONEBIT_Msk (0x1UL << USART_CR3_ONEBIT_Pos) /* 0x00000800 */
#define USART_CR3_ONEBIT USART_CR3_ONEBIT_Msk                /* One sample bit method enable */
#define USART_CR3_OVRDIS_Pos (12U)
#define USART_CR3_OVRDIS_Msk (0x1UL << USART_CR3_OVRDIS_Pos) /* 0x00001000 */
#define USART_CR3_OVRDIS USART_CR3_OVRDIS_Msk                /* Overrun Disable */
#define USART_CR3_DDRE_Pos (13U)
#define USART_CR3_DDRE_Msk (0x1UL << USART_CR3_DDRE_Pos) /* 0x00002000 */
#define USART_CR3_DDRE USART_CR3_DDRE_Msk                /* DMA Disable on Reception Error */
#define USART_CR3_DEM_Pos (14U)
#define USART_CR3_DEM_Msk (0x1UL << USART_CR3_DEM_Pos) /* 0x00004000 */
#define USART_CR3_DEM USART_CR3_DEM_Msk                /* Driver Enable Mode */
#define USART_CR3_DEP_Pos (15U)
#define USART_CR3_DEP_Msk (0x1UL << USART_CR3_DEP_Pos) /* 0x00008000 */
#define USART_CR3_DEP USART_CR3_DEP_Msk                /* Driver Enable Polarity Selection */
#define USART_CR3_SCARCNT_Pos (17U)
#define USART_CR3_SCARCNT_Msk (0x7UL << USART_CR3_SCARCNT_Pos) /* 0x000E0000 */
#define USART_CR3_SCARCNT USART_CR3_SCARCNT_Msk                /* SCARCNT[2:0] bits (SmartCard Auto-Retry Count) */
#define USART_CR3_SCARCNT_0 (0x1UL << USART_CR3_SCARCNT_Pos)   /* 0x00020000 */
#define USART_CR3_SCARCNT_1 (0x2UL << USART_CR3_SCARCNT_Pos)   /* 0x00040000 */
#define USART_CR3_SCARCNT_2 (0x4UL << USART_CR3_SCARCNT_Pos)   /* 0x00080000 */
#define USART_CR3_WUS_Pos (20U)
#define USART_CR3_WUS_Msk (0x3UL << USART_CR3_WUS_Pos) /* 0x00300000 */
#define USART_CR3_WUS USART_CR3_WUS_Msk                /* WUS[1:0] bits (Wake UP Interrupt Flag Selection) */
#define USART_CR3_WUS_0 (0x1UL << USART_CR3_WUS_Pos)   /* 0x00100000 */
#define USART_CR3_WUS_1 (0x2UL << USART_CR3_WUS_Pos)   /* 0x00200000 */
#define USART_CR3_WUFIE_Pos (22U)
#define USART_CR3_WUFIE_Msk (0x1UL << USART_CR3_WUFIE_Pos) /* 0x00400000 */
#define USART_CR3_WUFIE USART_CR3_WUFIE_Msk                /* Wake Up Interrupt Enable */
#define USART_CR3_TXFTIE_Pos (23U)
#define USART_CR3_TXFTIE_Msk (0x1UL << USART_CR3_TXFTIE_Pos) /* 0x00800000 */
#define USART_CR3_TXFTIE USART_CR3_TXFTIE_Msk                /* TXFIFO threshold interrupt enable */
#define USART_CR3_TCBGTIE_Pos (24U)
#define USART_CR3_TCBGTIE_Msk (0x1UL << USART_CR3_TCBGTIE_Pos) /* 0x01000000 */
#define USART_CR3_TCBGTIE USART_CR3_TCBGTIE_Msk                /* Transmission Complete Before Guard Time Interrupt Enable */
#define USART_CR3_RXFTCFG_Pos (25U)
#define USART_CR3_RXFTCFG_Msk (0x7UL << USART_CR3_RXFTCFG_Pos) /* 0x0E000000 */
#define USART_CR3_RXFTCFG USART_CR3_RXFTCFG_Msk                /* RXFIFO FIFO threshold configuration */
#define USART_CR3_RXFTCFG_0 (0x1UL << USART_CR3_RXFTCFG_Pos)   /* 0x02000000 */
#define USART_CR3_RXFTCFG_1 (0x2UL << USART_CR3_RXFTCFG_Pos)   /* 0x04000000 */
#define USART_CR3_RXFTCFG_2 (0x4UL << USART_CR3_RXFTCFG_Pos)   /* 0x08000000 */
#define USART_CR3_RXFTIE_Pos (28U)
#define USART_CR3_RXFTIE_Msk (0x1UL << USART_CR3_RXFTIE_Pos) /* 0x10000000 */
#define USART_CR3_RXFTIE USART_CR3_RXFTIE_Msk                /* RXFIFO threshold interrupt enable */
#define USART_CR3_TXFTCFG_Pos (29U)
#define USART_CR3_TXFTCFG_Msk (0x7UL << USART_CR3_TXFTCFG_Pos) /* 0xE0000000 */
#define USART_CR3_TXFTCFG USART_CR3_TXFTCFG_Msk                /* TXFIFO threshold configuration */
#define USART_CR3_TXFTCFG_0 (0x1UL << USART_CR3_TXFTCFG_Pos)   /* 0x20000000 */
#define USART_CR3_TXFTCFG_1 (0x2UL << USART_CR3_TXFTCFG_Pos)   /* 0x40000000 */
#define USART_CR3_TXFTCFG_2 (0x4UL << USART_CR3_TXFTCFG_Pos)   /* 0x80000000 */

/******************  Bit definition for USART_BRR register  *******************/
#define USART_BRR_BRR ((uint16_t)0xFFFF) /* USART  Baud rate register [15:0] */

/******************  Bit definition for USART_GTPR register  ******************/
#define USART_GTPR_PSC_Pos (0U)
#define USART_GTPR_PSC_Msk (0xFFUL << USART_GTPR_PSC_Pos) /* 0x000000FF */
#define USART_GTPR_PSC USART_GTPR_PSC_Msk                 /* PSC[7:0] bits (Prescaler value) */
#define USART_GTPR_GT_Pos (8U)
#define USART_GTPR_GT_Msk (0xFFUL << USART_GTPR_GT_Pos) /* 0x0000FF00 */
#define USART_GTPR_GT USART_GTPR_GT_Msk                 /* GT[7:0] bits (Guard time value) */

/*******************  Bit definition for USART_RTOR register  *****************/
#define USART_RTOR_RTO_Pos (0U)
#define USART_RTOR_RTO_Msk (0xFFFFFFUL << USART_RTOR_RTO_Pos) /* 0x00FFFFFF */
#define USART_RTOR_RTO USART_RTOR_RTO_Msk                     /* Receiver Time Out Value */
#define USART_RTOR_BLEN_Pos (24U)
#define USART_RTOR_BLEN_Msk (0xFFUL << USART_RTOR_BLEN_Pos) /* 0xFF000000 */
#define USART_RTOR_BLEN USART_RTOR_BLEN_Msk                 /* Block Length */

/*******************  Bit definition for USART_RQR register  ******************/
#define USART_RQR_ABRRQ ((uint16_t)0x0001) /* Auto-Baud Rate Request */
#define USART_RQR_SBKRQ ((uint16_t)0x0002) /* Send Break Request */
#define USART_RQR_MMRQ ((uint16_t)0x0004)  /* Mute Mode Request */
#define USART_RQR_RXFRQ ((uint16_t)0x0008) /* Receive Data flush Request */
#define USART_RQR_TXFRQ ((uint16_t)0x0010) /* Transmit data flush Request */

/*******************  Bit definition for USART_ISR register  ******************/
#define USART_ISR_PE_Pos (0U)
#define USART_ISR_PE_Msk (0x1UL << USART_ISR_PE_Pos) /* 0x00000001 */
#define USART_ISR_PE USART_ISR_PE_Msk                /* Parity Error */
#define USART_ISR_FE_Pos (1U)
#define USART_ISR_FE_Msk (0x1UL << USART_ISR_FE_Pos) /* 0x00000002 */
#define USART_ISR_FE USART_ISR_FE_Msk                /* Framing Error */
#define USART_ISR_NE_Pos (2U)
#define USART_ISR_NE_Msk (0x1UL << USART_ISR_NE_Pos) /* 0x00000004 */
#define USART_ISR_NE USART_ISR_NE_Msk                /* Noise detected Flag */
#define USART_ISR_ORE_Pos (3U)
#define USART_ISR_ORE_Msk (0x1UL << USART_ISR_ORE_Pos) /* 0x00000008 */
#define USART_ISR_ORE USART_ISR_ORE_Msk                /* OverRun Error */
#define USART_ISR_IDLE_Pos (4U)
#define USART_ISR_IDLE_Msk (0x1UL << USART_ISR_IDLE_Pos) /* 0x00000010 */
#define USART_ISR_IDLE USART_ISR_IDLE_Msk                /* IDLE line detected */
#define USART_ISR_RXNE_RXFNE_Pos (5U)
#define USART_ISR_RXNE_RXFNE_Msk (0x1UL << USART_ISR_RXNE_RXFNE_Pos) /* 0x00000020 */
#define USART_ISR_RXNE_RXFNE USART_ISR_RXNE_RXFNE_Msk                /* Read Data Register Not Empty/RXFIFO Not Empty */
#define USART_ISR_TC_Pos (6U)
#define USART_ISR_TC_Msk (0x1UL << USART_ISR_TC_Pos) /* 0x00000040 */
#define USART_ISR_TC USART_ISR_TC_Msk                /* Transmission Complete */
#define USART_ISR_TXE_TXFNF_Pos (7U)
#define USART_ISR_TXE_TXFNF_Msk (0x1UL << USART_ISR_TXE_TXFNF_Pos) /* 0x00000080 */
#define USART_ISR_TXE_TXFNF USART_ISR_TXE_TXFNF_Msk                /* Transmit Data Register Empty/TXFIFO Not Full */
#define USART_ISR_LBDF_Pos (8U)
#define USART_ISR_LBDF_Msk (0x1UL << USART_ISR_LBDF_Pos) /* 0x00000100 */
#define USART_ISR_LBDF USART_ISR_LBDF_Msk                /* LIN Break Detection Flag */
#define USART_ISR_CTSIF_Pos (9U)
#define USART_ISR_CTSIF_Msk (0x1UL << USART_ISR_CTSIF_Pos) /* 0x00000200 */
#define USART_ISR_CTSIF USART_ISR_CTSIF_Msk                /* CTS interrupt flag */
#define USART_ISR_CTS_Pos (10U)
#define USART_ISR_CTS_Msk (0x1UL << USART_ISR_CTS_Pos) /* 0x00000400 */
#define USART_ISR_CTS USART_ISR_CTS_Msk                /* CTS flag */
#define USART_ISR_RTOF_Pos (11U)
#define USART_ISR_RTOF_Msk (0x1UL << USART_ISR_RTOF_Pos) /* 0x00000800 */
#define USART_ISR_RTOF USART_ISR_RTOF_Msk                /* Receiver Time Out */
#define USART_ISR_EOBF_Pos (12U)
#define USART_ISR_EOBF_Msk (0x1UL << USART_ISR_EOBF_Pos) /* 0x00001000 */
#define USART_ISR_EOBF USART_ISR_EOBF_Msk                /* End Of Block Flag */
#define USART_ISR_UDR_Pos (13U)
#define USART_ISR_UDR_Msk (0x1UL << USART_ISR_UDR_Pos) /* 0x00002000 */
#define USART_ISR_UDR USART_ISR_UDR_Msk                /* SPI Slave Underrun Error Flag */
#define USART_ISR_ABRE_Pos (14U)
#define USART_ISR_ABRE_Msk (0x1UL << USART_ISR_ABRE_Pos) /* 0x00004000 */
#define USART_ISR_ABRE USART_ISR_ABRE_Msk                /* Auto-Baud Rate Error */
#define USART_ISR_ABRF_Pos (15U)
#define USART_ISR_ABRF_Msk (0x1UL << USART_ISR_ABRF_Pos) /* 0x00008000 */
#define USART_ISR_ABRF USART_ISR_ABRF_Msk                /* Auto-Baud Rate Flag */
#define USART_ISR_BUSY_Pos (16U)
#define USART_ISR_BUSY_Msk (0x1UL << USART_ISR_BUSY_Pos) /* 0x00010000 */
#define USART_ISR_BUSY USART_ISR_BUSY_Msk                /* Busy Flag */
#define USART_ISR_CMF_Pos (17U)
#define USART_ISR_CMF_Msk (0x1UL << USART_ISR_CMF_Pos) /* 0x00020000 */
#define USART_ISR_CMF USART_ISR_CMF_Msk                /* Character Match Flag */
#define USART_ISR_SBKF_Pos (18U)
#define USART_ISR_SBKF_Msk (0x1UL << USART_ISR_SBKF_Pos) /* 0x00040000 */
#define USART_ISR_SBKF USART_ISR_SBKF_Msk                /* Send Break Flag */
#define USART_ISR_RWU_Pos (19U)
#define USART_ISR_RWU_Msk (0x1UL << USART_ISR_RWU_Pos) /* 0x00080000 */
#define USART_ISR_RWU USART_ISR_RWU_Msk                /* Receive Wake Up from mute mode Flag */
#define USART_ISR_WUF_Pos (20U)
#define USART_ISR_WUF_Msk (0x1UL << USART_ISR_WUF_Pos) /* 0x00100000 */
#define USART_ISR_WUF USART_ISR_WUF_Msk                /* Wake Up from stop mode Flag */
#define USART_ISR_TEACK_Pos (21U)
#define USART_ISR_TEACK_Msk (0x1UL << USART_ISR_TEACK_Pos) /* 0x00200000 */
#define USART_ISR_TEACK USART_ISR_TEACK_Msk                /* Transmit Enable Acknowledge Flag */
#define USART_ISR_REACK_Pos (22U)
#define USART_ISR_REACK_Msk (0x1UL << USART_ISR_REACK_Pos) /* 0x00400000 */
#define USART_ISR_REACK USART_ISR_REACK_Msk                /* Receive Enable Acknowledge Flag */
#define USART_ISR_TXFE_Pos (23U)
#define USART_ISR_TXFE_Msk (0x1UL << USART_ISR_TXFE_Pos) /* 0x00800000 */
#define USART_ISR_TXFE USART_ISR_TXFE_Msk                /* TXFIFO Empty Flag */
#define USART_ISR_RXFF_Pos (24U)
#define USART_ISR_RXFF_Msk (0x1UL << USART_ISR_RXFF_Pos) /* 0x01000000 */
#define USART_ISR_RXFF USART_ISR_RXFF_Msk                /* RXFIFO Full Flag */
#define USART_ISR_TCBGT_Pos (25U)
#define USART_ISR_TCBGT_Msk (0x1UL << USART_ISR_TCBGT_Pos) /* 0x02000000 */
#define USART_ISR_TCBGT USART_ISR_TCBGT_Msk                /* Transmission Complete Before Guard Time Completion Flag */
#define USART_ISR_RXFT_Pos (26U)
#define USART_ISR_RXFT_Msk (0x1UL << USART_ISR_RXFT_Pos) /* 0x04000000 */
#define USART_ISR_RXFT USART_ISR_RXFT_Msk                /* RXFIFO Threshold Flag */
#define USART_ISR_TXFT_Pos (27U)
#define USART_ISR_TXFT_Msk (0x1UL << USART_ISR_TXFT_Pos) /* 0x08000000 */
#define USART_ISR_TXFT USART_ISR_TXFT_Msk                /* TXFIFO Threshold Flag */

/*******************  Bit definition for USART_ICR register  ******************/
#define USART_ICR_PECF_Pos (0U)
#define USART_ICR_PECF_Msk (0x1UL << USART_ICR_PECF_Pos) /* 0x00000001 */
#define USART_ICR_PECF USART_ICR_PECF_Msk                /* Parity Error Clear Flag */
#define USART_ICR_FECF_Pos (1U)
#define USART_ICR_FECF_Msk (0x1UL << USART_ICR_FECF_Pos) /* 0x00000002 */
#define USART_ICR_FECF USART_ICR_FECF_Msk                /* Framing Error Clear Flag */
#define USART_ICR_NECF_Pos (2U)
#define USART_ICR_NECF_Msk (0x1UL << USART_ICR_NECF_Pos) /* 0x00000004 */
#define USART_ICR_NECF USART_ICR_NECF_Msk                /* Noise Error detected Clear Flag */
#define USART_ICR_ORECF_Pos (3U)
#define USART_ICR_ORECF_Msk (0x1UL << USART_ICR_ORECF_Pos) /* 0x00000008 */
#define USART_ICR_ORECF USART_ICR_ORECF_Msk                /* OverRun Error Clear Flag */
#define USART_ICR_IDLECF_Pos (4U)
#define USART_ICR_IDLECF_Msk (0x1UL << USART_ICR_IDLECF_Pos) /* 0x00000010 */
#define USART_ICR_IDLECF USART_ICR_IDLECF_Msk                /* IDLE line detected Clear Flag */
#define USART_ICR_TXFECF_Pos (5U)
#define USART_ICR_TXFECF_Msk (0x1UL << USART_ICR_TXFECF_Pos) /* 0x00000020 */
#define USART_ICR_TXFECF USART_ICR_TXFECF_Msk                /* TXFIFO Empty Clear Flag */
#define USART_ICR_TCCF_Pos (6U)
#define USART_ICR_TCCF_Msk (0x1UL << USART_ICR_TCCF_Pos) /* 0x00000040 */
#define USART_ICR_TCCF USART_ICR_TCCF_Msk                /* Transmission Complete Clear Flag */
#define USART_ICR_TCBGTCF_Pos (7U)
#define USART_ICR_TCBGTCF_Msk (0x1UL << USART_ICR_TCBGTCF_Pos) /* 0x00000080 */
#define USART_ICR_TCBGTCF USART_ICR_TCBGTCF_Msk                /* Transmission Complete Before Guard Time Clear Flag */
#define USART_ICR_LBDCF_Pos (8U)
#define USART_ICR_LBDCF_Msk (0x1UL << USART_ICR_LBDCF_Pos) /* 0x00000100 */
#define USART_ICR_LBDCF USART_ICR_LBDCF_Msk                /* LIN Break Detection Clear Flag */
#define USART_ICR_CTSCF_Pos (9U)
#define USART_ICR_CTSCF_Msk (0x1UL << USART_ICR_CTSCF_Pos) /* 0x00000200 */
#define USART_ICR_CTSCF USART_ICR_CTSCF_Msk                /* CTS Interrupt Clear Flag */
#define USART_ICR_RTOCF_Pos (11U)
#define USART_ICR_RTOCF_Msk (0x1UL << USART_ICR_RTOCF_Pos) /* 0x00000800 */
#define USART_ICR_RTOCF USART_ICR_RTOCF_Msk                /* Receiver Time Out Clear Flag */
#define USART_ICR_EOBCF_Pos (12U)
#define USART_ICR_EOBCF_Msk (0x1UL << USART_ICR_EOBCF_Pos) /* 0x00001000 */
#define USART_ICR_EOBCF USART_ICR_EOBCF_Msk                /* End Of Block Clear Flag */
#define USART_ICR_UDRCF_Pos (13U)
#define USART_ICR_UDRCF_Msk (0x1UL << USART_ICR_UDRCF_Pos) /* 0x00002000 */
#define USART_ICR_UDRCF USART_ICR_UDRCF_Msk                /* SPI Slave Underrun Clear Flag */
#define USART_ICR_CMCF_Pos (17U)
#define USART_ICR_CMCF_Msk (0x1UL << USART_ICR_CMCF_Pos) /* 0x00020000 */
#define USART_ICR_CMCF USART_ICR_CMCF_Msk                /* Character Match Clear Flag */
#define USART_ICR_WUCF_Pos (20U)
#define USART_ICR_WUCF_Msk (0x1UL << USART_ICR_WUCF_Pos) /* 0x00100000 */
#define USART_ICR_WUCF USART_ICR_WUCF_Msk                /* Wake Up from stop mode Clear Flag */

/*******************  Bit definition for USART_RDR register  ******************/
#define USART_RDR_RDR_Pos (0U)
#define USART_RDR_RDR_Msk (0x1FFUL << USART_RDR_RDR_Pos) /* 0x000001FF */
#define USART_RDR_RDR USART_RDR_RDR_Msk                  /* RDR[8:0] bits (Receive Data value) */

/*******************  Bit definition for USART_TDR register  ******************/
#define USART_TDR_TDR_Pos (0U)
#define USART_TDR_TDR_Msk (0x1FFUL << USART_TDR_TDR_Pos) /* 0x000001FF */
#define USART_TDR_TDR USART_TDR_TDR_Msk                  /* TDR[8:0] bits (Transmit Data value) */

/*******************  Bit definition for USART_PRESC register  ****************/
#define USART_PRESC_PRESCALER_Pos (0U)
#define USART_PRESC_PRESCALER_Msk (0xFUL << USART_PRESC_PRESCALER_Pos) /* 0x0000000F */
#define USART_PRESC_PRESCALER USART_PRESC_PRESCALER_Msk                /* PRESCALER[3:0] bits (Clock prescaler) */
#define USART_PRESC_PRESCALER_0 (0x1UL << USART_PRESC_PRESCALER_Pos)   /* 0x00000001 */
#define USART_PRESC_PRESCALER_1 (0x2UL << USART_PRESC_PRESCALER_Pos)   /* 0x00000002 */
#define USART_PRESC_PRESCALER_2 (0x4UL << USART_PRESC_PRESCALER_Pos)   /* 0x00000004 */
#define USART_PRESC_PRESCALER_3 (0x8UL << USART_PRESC_PRESCALER_Pos)   /* 0x00000008 */

/******************************************************************************
 *                   Inter-integrated Circuit Interface (I2C)                 *
 ******************************************************************************/
/*******************  Bit definition for I2C_CR1 register  *******************/
#define I2C_CR1_PE_Pos (0U)
#define I2C_CR1_PE_Msk (0x1UL << I2C_CR1_PE_Pos) /*!< 0x00000001 */
#define I2C_CR1_PE I2C_CR1_PE_Msk                /*!< Peripheral enable */
#define I2C_CR1_TXIE_Pos (1U)
#define I2C_CR1_TXIE_Msk (0x1UL << I2C_CR1_TXIE_Pos) /*!< 0x00000002 */
#define I2C_CR1_TXIE I2C_CR1_TXIE_Msk                /*!< TX interrupt enable */
#define I2C_CR1_RXIE_Pos (2U)
#define I2C_CR1_RXIE_Msk (0x1UL << I2C_CR1_RXIE_Pos) /*!< 0x00000004 */
#define I2C_CR1_RXIE I2C_CR1_RXIE_Msk                /*!< RX interrupt enable */
#define I2C_CR1_ADDRIE_Pos (3U)
#define I2C_CR1_ADDRIE_Msk (0x1UL << I2C_CR1_ADDRIE_Pos) /*!< 0x00000008 */
#define I2C_CR1_ADDRIE I2C_CR1_ADDRIE_Msk                /*!< Address match interrupt enable */
#define I2C_CR1_NACKIE_Pos (4U)
#define I2C_CR1_NACKIE_Msk (0x1UL << I2C_CR1_NACKIE_Pos) /*!< 0x00000010 */
#define I2C_CR1_NACKIE I2C_CR1_NACKIE_Msk                /*!< NACK received interrupt enable */
#define I2C_CR1_STOPIE_Pos (5U)
#define I2C_CR1_STOPIE_Msk (0x1UL << I2C_CR1_STOPIE_Pos) /*!< 0x00000020 */
#define I2C_CR1_STOPIE I2C_CR1_STOPIE_Msk                /*!< STOP detection interrupt enable */
#define I2C_CR1_TCIE_Pos (6U)
#define I2C_CR1_TCIE_Msk (0x1UL << I2C_CR1_TCIE_Pos) /*!< 0x00000040 */
#define I2C_CR1_TCIE I2C_CR1_TCIE_Msk                /*!< Transfer complete interrupt enable */
#define I2C_CR1_ERRIE_Pos (7U)
#define I2C_CR1_ERRIE_Msk (0x1UL << I2C_CR1_ERRIE_Pos) /*!< 0x00000080 */
#define I2C_CR1_ERRIE I2C_CR1_ERRIE_Msk                /*!< Errors interrupt enable */
#define I2C_CR1_DNF_Pos (8U)
#define I2C_CR1_DNF_Msk (0xFUL << I2C_CR1_DNF_Pos) /*!< 0x00000F00 */
#define I2C_CR1_DNF I2C_CR1_DNF_Msk                /*!< Digital noise filter */
#define I2C_CR1_ANFOFF_Pos (12U)
#define I2C_CR1_ANFOFF_Msk (0x1UL << I2C_CR1_ANFOFF_Pos) /*!< 0x00001000 */
#define I2C_CR1_ANFOFF I2C_CR1_ANFOFF_Msk                /*!< Analog noise filter OFF */
#define I2C_CR1_SWRST_Pos (13U)
#define I2C_CR1_SWRST_Msk (0x1UL << I2C_CR1_SWRST_Pos) /*!< 0x00002000 */
#define I2C_CR1_SWRST I2C_CR1_SWRST_Msk                /*!< Software reset */
#define I2C_CR1_TXDMAEN_Pos (14U)
#define I2C_CR1_TXDMAEN_Msk (0x1UL << I2C_CR1_TXDMAEN_Pos) /*!< 0x00004000 */
#define I2C_CR1_TXDMAEN I2C_CR1_TXDMAEN_Msk                /*!< DMA transmission requests enable */
#define I2C_CR1_RXDMAEN_Pos (15U)
#define I2C_CR1_RXDMAEN_Msk (0x1UL << I2C_CR1_RXDMAEN_Pos) /*!< 0x00008000 */
#define I2C_CR1_RXDMAEN I2C_CR1_RXDMAEN_Msk                /*!< DMA reception requests enable */
#define I2C_CR1_SBC_Pos (16U)
#define I2C_CR1_SBC_Msk (0x1UL << I2C_CR1_SBC_Pos) /*!< 0x00010000 */
#define I2C_CR1_SBC I2C_CR1_SBC_Msk                /*!< Slave byte control */
#define I2C_CR1_NOSTRETCH_Pos (17U)
#define I2C_CR1_NOSTRETCH_Msk (0x1UL << I2C_CR1_NOSTRETCH_Pos) /*!< 0x00020000 */
#define I2C_CR1_NOSTRETCH I2C_CR1_NOSTRETCH_Msk                /*!< Clock stretching disable */
#define I2C_CR1_WUPEN_Pos (18U)
#define I2C_CR1_WUPEN_Msk (0x1UL << I2C_CR1_WUPEN_Pos) /*!< 0x00040000 */
#define I2C_CR1_WUPEN I2C_CR1_WUPEN_Msk                /*!< Wakeup from STOP enable */
#define I2C_CR1_GCEN_Pos (19U)
#define I2C_CR1_GCEN_Msk (0x1UL << I2C_CR1_GCEN_Pos) /*!< 0x00080000 */
#define I2C_CR1_GCEN I2C_CR1_GCEN_Msk                /*!< General call enable */
#define I2C_CR1_SMBHEN_Pos (20U)
#define I2C_CR1_SMBHEN_Msk (0x1UL << I2C_CR1_SMBHEN_Pos) /*!< 0x00100000 */
#define I2C_CR1_SMBHEN I2C_CR1_SMBHEN_Msk                /*!< SMBus host address enable */
#define I2C_CR1_SMBDEN_Pos (21U)
#define I2C_CR1_SMBDEN_Msk (0x1UL << I2C_CR1_SMBDEN_Pos) /*!< 0x00200000 */
#define I2C_CR1_SMBDEN I2C_CR1_SMBDEN_Msk                /*!< SMBus device default address enable */
#define I2C_CR1_ALERTEN_Pos (22U)
#define I2C_CR1_ALERTEN_Msk (0x1UL << I2C_CR1_ALERTEN_Pos) /*!< 0x00400000 */
#define I2C_CR1_ALERTEN I2C_CR1_ALERTEN_Msk                /*!< SMBus alert enable */
#define I2C_CR1_PECEN_Pos (23U)
#define I2C_CR1_PECEN_Msk (0x1UL << I2C_CR1_PECEN_Pos) /*!< 0x00800000 */
#define I2C_CR1_PECEN I2C_CR1_PECEN_Msk                /*!< PEC enable */

/******************  Bit definition for I2C_CR2 register  ********************/
#define I2C_CR2_SADD_Pos (0U)
#define I2C_CR2_SADD_Msk (0x3FFUL << I2C_CR2_SADD_Pos) /*!< 0x000003FF */
#define I2C_CR2_SADD I2C_CR2_SADD_Msk                  /*!< Slave address (master mode) */
#define I2C_CR2_RD_WRN_Pos (10U)
#define I2C_CR2_RD_WRN_Msk (0x1UL << I2C_CR2_RD_WRN_Pos) /*!< 0x00000400 */
#define I2C_CR2_RD_WRN I2C_CR2_RD_WRN_Msk                /*!< Transfer direction (master mode) */
#define I2C_CR2_ADD10_Pos (11U)
#define I2C_CR2_ADD10_Msk (0x1UL << I2C_CR2_ADD10_Pos) /*!< 0x00000800 */
#define I2C_CR2_ADD10 I2C_CR2_ADD10_Msk                /*!< 10-bit addressing mode (master mode) */
#define I2C_CR2_HEAD10R_Pos (12U)
#define I2C_CR2_HEAD10R_Msk (0x1UL << I2C_CR2_HEAD10R_Pos) /*!< 0x00001000 */
#define I2C_CR2_HEAD10R I2C_CR2_HEAD10R_Msk                /*!< 10-bit address header only read direction (master mode) */
#define I2C_CR2_START_Pos (13U)
#define I2C_CR2_START_Msk (0x1UL << I2C_CR2_START_Pos) /*!< 0x00002000 */
#define I2C_CR2_START I2C_CR2_START_Msk                /*!< START generation */
#define I2C_CR2_STOP_Pos (14U)
#define I2C_CR2_STOP_Msk (0x1UL << I2C_CR2_STOP_Pos) /*!< 0x00004000 */
#define I2C_CR2_STOP I2C_CR2_STOP_Msk                /*!< STOP generation (master mode) */
#define I2C_CR2_NACK_Pos (15U)
#define I2C_CR2_NACK_Msk (0x1UL << I2C_CR2_NACK_Pos) /*!< 0x00008000 */
#define I2C_CR2_NACK I2C_CR2_NACK_Msk                /*!< NACK generation (slave mode) */
#define I2C_CR2_NBYTES_Pos (16U)
#define I2C_CR2_NBYTES_Msk (0xFFUL << I2C_CR2_NBYTES_Pos) /*!< 0x00FF0000 */
#define I2C_CR2_NBYTES I2C_CR2_NBYTES_Msk                 /*!< Number of bytes */
#define I2C_CR2_RELOAD_Pos (24U)
#define I2C_CR2_RELOAD_Msk (0x1UL << I2C_CR2_RELOAD_Pos) /*!< 0x01000000 */
#define I2C_CR2_RELOAD I2C_CR2_RELOAD_Msk                /*!< NBYTES reload mode */
#define I2C_CR2_AUTOEND_Pos (25U)
#define I2C_CR2_AUTOEND_Msk (0x1UL << I2C_CR2_AUTOEND_Pos) /*!< 0x02000000 */
#define I2C_CR2_AUTOEND I2C_CR2_AUTOEND_Msk                /*!< Automatic end mode (master mode) */
#define I2C_CR2_PECBYTE_Pos (26U)
#define I2C_CR2_PECBYTE_Msk (0x1UL << I2C_CR2_PECBYTE_Pos) /*!< 0x04000000 */
#define I2C_CR2_PECBYTE I2C_CR2_PECBYTE_Msk                /*!< Packet error checking byte */

/*******************  Bit definition for I2C_OAR1 register  ******************/
#define I2C_OAR1_OA1_Pos (0U)
#define I2C_OAR1_OA1_Msk (0x3FFUL << I2C_OAR1_OA1_Pos) /*!< 0x000003FF */
#define I2C_OAR1_OA1 I2C_OAR1_OA1_Msk                  /*!< Interface own address 1 */
#define I2C_OAR1_OA1MODE_Pos (10U)
#define I2C_OAR1_OA1MODE_Msk (0x1UL << I2C_OAR1_OA1MODE_Pos) /*!< 0x00000400 */
#define I2C_OAR1_OA1MODE I2C_OAR1_OA1MODE_Msk                /*!< Own address 1 10-bit mode */
#define I2C_OAR1_OA1EN_Pos (15U)
#define I2C_OAR1_OA1EN_Msk (0x1UL << I2C_OAR1_OA1EN_Pos) /*!< 0x00008000 */
#define I2C_OAR1_OA1EN I2C_OAR1_OA1EN_Msk                /*!< Own address 1 enable */

/*******************  Bit definition for I2C_OAR2 register  ******************/
#define I2C_OAR2_OA2_Pos (1U)
#define I2C_OAR2_OA2_Msk (0x7FUL << I2C_OAR2_OA2_Pos) /*!< 0x000000FE */
#define I2C_OAR2_OA2 I2C_OAR2_OA2_Msk                 /*!< Interface own address 2 */
#define I2C_OAR2_OA2MSK_Pos (8U)
#define I2C_OAR2_OA2MSK_Msk (0x7UL << I2C_OAR2_OA2MSK_Pos) /*!< 0x00000700 */
#define I2C_OAR2_OA2MSK I2C_OAR2_OA2MSK_Msk                /*!< Own address 2 masks */
#define I2C_OAR2_OA2NOMASK (0U)                            /*!< No mask                                        */
#define I2C_OAR2_OA2MASK01_Pos (8U)
#define I2C_OAR2_OA2MASK01_Msk (0x1UL << I2C_OAR2_OA2MASK01_Pos) /*!< 0x00000100 */
#define I2C_OAR2_OA2MASK01 I2C_OAR2_OA2MASK01_Msk                /*!< OA2[1] is masked, Only OA2[7:2] are compared   */
#define I2C_OAR2_OA2MASK02_Pos (9U)
#define I2C_OAR2_OA2MASK02_Msk (0x1UL << I2C_OAR2_OA2MASK02_Pos) /*!< 0x00000200 */
#define I2C_OAR2_OA2MASK02 I2C_OAR2_OA2MASK02_Msk                /*!< OA2[2:1] is masked, Only OA2[7:3] are compared */
#define I2C_OAR2_OA2MASK03_Pos (8U)
#define I2C_OAR2_OA2MASK03_Msk (0x3UL << I2C_OAR2_OA2MASK03_Pos) /*!< 0x00000300 */
#define I2C_OAR2_OA2MASK03 I2C_OAR2_OA2MASK03_Msk                /*!< OA2[3:1] is masked, Only OA2[7:4] are compared */
#define I2C_OAR2_OA2MASK04_Pos (10U)
#define I2C_OAR2_OA2MASK04_Msk (0x1UL << I2C_OAR2_OA2MASK04_Pos) /*!< 0x00000400 */
#define I2C_OAR2_OA2MASK04 I2C_OAR2_OA2MASK04_Msk                /*!< OA2[4:1] is masked, Only OA2[7:5] are compared */
#define I2C_OAR2_OA2MASK05_Pos (8U)
#define I2C_OAR2_OA2MASK05_Msk (0x5UL << I2C_OAR2_OA2MASK05_Pos) /*!< 0x00000500 */
#define I2C_OAR2_OA2MASK05 I2C_OAR2_OA2MASK05_Msk                /*!< OA2[5:1] is masked, Only OA2[7:6] are compared */
#define I2C_OAR2_OA2MASK06_Pos (9U)
#define I2C_OAR2_OA2MASK06_Msk (0x3UL << I2C_OAR2_OA2MASK06_Pos) /*!< 0x00000600 */
#define I2C_OAR2_OA2MASK06 I2C_OAR2_OA2MASK06_Msk                /*!< OA2[6:1] is masked, Only OA2[7] are compared   */
#define I2C_OAR2_OA2MASK07_Pos (8U)
#define I2C_OAR2_OA2MASK07_Msk (0x7UL << I2C_OAR2_OA2MASK07_Pos) /*!< 0x00000700 */
#define I2C_OAR2_OA2MASK07 I2C_OAR2_OA2MASK07_Msk                /*!< OA2[7:1] is masked, No comparison is done      */
#define I2C_OAR2_OA2EN_Pos (15U)
#define I2C_OAR2_OA2EN_Msk (0x1UL << I2C_OAR2_OA2EN_Pos) /*!< 0x00008000 */
#define I2C_OAR2_OA2EN I2C_OAR2_OA2EN_Msk                /*!< Own address 2 enable */

/*******************  Bit definition for I2C_TIMINGR register *******************/
#define I2C_TIMINGR_CLEAR_MASK (0xF0FFFFFFU) /* Make used to set TIMINGR clearing reserved bits */
#define I2C_TIMINGR_SCLL_Pos (0U)
#define I2C_TIMINGR_SCLL_Msk (0xFFUL << I2C_TIMINGR_SCLL_Pos) /*!< 0x000000FF */
#define I2C_TIMINGR_SCLL I2C_TIMINGR_SCLL_Msk                 /*!< SCL low period (master mode) */
#define I2C_TIMINGR_SCLH_Pos (8U)
#define I2C_TIMINGR_SCLH_Msk (0xFFUL << I2C_TIMINGR_SCLH_Pos) /*!< 0x0000FF00 */
#define I2C_TIMINGR_SCLH I2C_TIMINGR_SCLH_Msk                 /*!< SCL high period (master mode) */
#define I2C_TIMINGR_SDADEL_Pos (16U)
#define I2C_TIMINGR_SDADEL_Msk (0xFUL << I2C_TIMINGR_SDADEL_Pos) /*!< 0x000F0000 */
#define I2C_TIMINGR_SDADEL I2C_TIMINGR_SDADEL_Msk                /*!< Data hold time */
#define I2C_TIMINGR_SCLDEL_Pos (20U)
#define I2C_TIMINGR_SCLDEL_Msk (0xFUL << I2C_TIMINGR_SCLDEL_Pos) /*!< 0x00F00000 */
#define I2C_TIMINGR_SCLDEL I2C_TIMINGR_SCLDEL_Msk                /*!< Data setup time */
#define I2C_TIMINGR_PRESC_Pos (28U)
#define I2C_TIMINGR_PRESC_Msk (0xFUL << I2C_TIMINGR_PRESC_Pos) /*!< 0xF0000000 */
#define I2C_TIMINGR_PRESC I2C_TIMINGR_PRESC_Msk                /*!< Timings prescaler */

/******************* Bit definition for I2C_TIMEOUTR register *******************/
#define I2C_TIMEOUTR_TIMEOUTA_Pos (0U)
#define I2C_TIMEOUTR_TIMEOUTA_Msk (0xFFFUL << I2C_TIMEOUTR_TIMEOUTA_Pos) /*!< 0x00000FFF */
#define I2C_TIMEOUTR_TIMEOUTA I2C_TIMEOUTR_TIMEOUTA_Msk                  /*!< Bus timeout A */
#define I2C_TIMEOUTR_TIDLE_Pos (12U)
#define I2C_TIMEOUTR_TIDLE_Msk (0x1UL << I2C_TIMEOUTR_TIDLE_Pos) /*!< 0x00001000 */
#define I2C_TIMEOUTR_TIDLE I2C_TIMEOUTR_TIDLE_Msk                /*!< Idle clock timeout detection */
#define I2C_TIMEOUTR_TIMOUTEN_Pos (15U)
#define I2C_TIMEOUTR_TIMOUTEN_Msk (0x1UL << I2C_TIMEOUTR_TIMOUTEN_Pos) /*!< 0x00008000 */
#define I2C_TIMEOUTR_TIMOUTEN I2C_TIMEOUTR_TIMOUTEN_Msk                /*!< Clock timeout enable */
#define I2C_TIMEOUTR_TIMEOUTB_Pos (16U)
#define I2C_TIMEOUTR_TIMEOUTB_Msk (0xFFFUL << I2C_TIMEOUTR_TIMEOUTB_Pos) /*!< 0x0FFF0000 */
#define I2C_TIMEOUTR_TIMEOUTB I2C_TIMEOUTR_TIMEOUTB_Msk                  /*!< Bus timeout B*/
#define I2C_TIMEOUTR_TEXTEN_Pos (31U)
#define I2C_TIMEOUTR_TEXTEN_Msk (0x1UL << I2C_TIMEOUTR_TEXTEN_Pos) /*!< 0x80000000 */
#define I2C_TIMEOUTR_TEXTEN I2C_TIMEOUTR_TEXTEN_Msk                /*!< Extended clock timeout enable */

/******************  Bit definition for I2C_ISR register  *********************/
#define I2C_ISR_TXE_Pos (0U)
#define I2C_ISR_TXE_Msk (0x1UL << I2C_ISR_TXE_Pos) /*!< 0x00000001 */
#define I2C_ISR_TXE I2C_ISR_TXE_Msk                /*!< Transmit data register empty */
#define I2C_ISR_TXIS_Pos (1U)
#define I2C_ISR_TXIS_Msk (0x1UL << I2C_ISR_TXIS_Pos) /*!< 0x00000002 */
#define I2C_ISR_TXIS I2C_ISR_TXIS_Msk                /*!< Transmit interrupt status */
#define I2C_ISR_RXNE_Pos (2U)
#define I2C_ISR_RXNE_Msk (0x1UL << I2C_ISR_RXNE_Pos) /*!< 0x00000004 */
#define I2C_ISR_RXNE I2C_ISR_RXNE_Msk                /*!< Receive data register not empty */
#define I2C_ISR_ADDR_Pos (3U)
#define I2C_ISR_ADDR_Msk (0x1UL << I2C_ISR_ADDR_Pos) /*!< 0x00000008 */
#define I2C_ISR_ADDR I2C_ISR_ADDR_Msk                /*!< Address matched (slave mode)*/
#define I2C_ISR_NACKF_Pos (4U)
#define I2C_ISR_NACKF_Msk (0x1UL << I2C_ISR_NACKF_Pos) /*!< 0x00000010 */
#define I2C_ISR_NACKF I2C_ISR_NACKF_Msk                /*!< NACK received flag */
#define I2C_ISR_STOPF_Pos (5U)
#define I2C_ISR_STOPF_Msk (0x1UL << I2C_ISR_STOPF_Pos) /*!< 0x00000020 */
#define I2C_ISR_STOPF I2C_ISR_STOPF_Msk                /*!< STOP detection flag */
#define I2C_ISR_TC_Pos (6U)
#define I2C_ISR_TC_Msk (0x1UL << I2C_ISR_TC_Pos) /*!< 0x00000040 */
#define I2C_ISR_TC I2C_ISR_TC_Msk                /*!< Transfer complete (master mode) */
#define I2C_ISR_TCR_Pos (7U)
#define I2C_ISR_TCR_Msk (0x1UL << I2C_ISR_TCR_Pos) /*!< 0x00000080 */
#define I2C_ISR_TCR I2C_ISR_TCR_Msk                /*!< Transfer complete reload */
#define I2C_ISR_BERR_Pos (8U)
#define I2C_ISR_BERR_Msk (0x1UL << I2C_ISR_BERR_Pos) /*!< 0x00000100 */
#define I2C_ISR_BERR I2C_ISR_BERR_Msk                /*!< Bus error */
#define I2C_ISR_ARLO_Pos (9U)
#define I2C_ISR_ARLO_Msk (0x1UL << I2C_ISR_ARLO_Pos) /*!< 0x00000200 */
#define I2C_ISR_ARLO I2C_ISR_ARLO_Msk                /*!< Arbitration lost */
#define I2C_ISR_OVR_Pos (10U)
#define I2C_ISR_OVR_Msk (0x1UL << I2C_ISR_OVR_Pos) /*!< 0x00000400 */
#define I2C_ISR_OVR I2C_ISR_OVR_Msk                /*!< Overrun/Underrun */
#define I2C_ISR_PECERR_Pos (11U)
#define I2C_ISR_PECERR_Msk (0x1UL << I2C_ISR_PECERR_Pos) /*!< 0x00000800 */
#define I2C_ISR_PECERR I2C_ISR_PECERR_Msk                /*!< PEC error in reception */
#define I2C_ISR_TIMEOUT_Pos (12U)
#define I2C_ISR_TIMEOUT_Msk (0x1UL << I2C_ISR_TIMEOUT_Pos) /*!< 0x00001000 */
#define I2C_ISR_TIMEOUT I2C_ISR_TIMEOUT_Msk                /*!< Timeout or Tlow detection flag */
#define I2C_ISR_ALERT_Pos (13U)
#define I2C_ISR_ALERT_Msk (0x1UL << I2C_ISR_ALERT_Pos) /*!< 0x00002000 */
#define I2C_ISR_ALERT I2C_ISR_ALERT_Msk                /*!< SMBus alert */
#define I2C_ISR_BUSY_Pos (15U)
#define I2C_ISR_BUSY_Msk (0x1UL << I2C_ISR_BUSY_Pos) /*!< 0x00008000 */
#define I2C_ISR_BUSY I2C_ISR_BUSY_Msk                /*!< Bus busy */
#define I2C_ISR_DIR_Pos (16U)
#define I2C_ISR_DIR_Msk (0x1UL << I2C_ISR_DIR_Pos) /*!< 0x00010000 */
#define I2C_ISR_DIR I2C_ISR_DIR_Msk                /*!< Transfer direction (slave mode) */
#define I2C_ISR_ADDCODE_Pos (17U)
#define I2C_ISR_ADDCODE_Msk (0x7FUL << I2C_ISR_ADDCODE_Pos) /*!< 0x00FE0000 */
#define I2C_ISR_ADDCODE I2C_ISR_ADDCODE_Msk                 /*!< Address match code (slave mode) */

/******************  Bit definition for I2C_ICR register  *********************/
#define I2C_ICR_ADDRCF_Pos (3U)
#define I2C_ICR_ADDRCF_Msk (0x1UL << I2C_ICR_ADDRCF_Pos) /*!< 0x00000008 */
#define I2C_ICR_ADDRCF I2C_ICR_ADDRCF_Msk                /*!< Address matched clear flag */
#define I2C_ICR_NACKCF_Pos (4U)
#define I2C_ICR_NACKCF_Msk (0x1UL << I2C_ICR_NACKCF_Pos) /*!< 0x00000010 */
#define I2C_ICR_NACKCF I2C_ICR_NACKCF_Msk                /*!< NACK clear flag */
#define I2C_ICR_STOPCF_Pos (5U)
#define I2C_ICR_STOPCF_Msk (0x1UL << I2C_ICR_STOPCF_Pos) /*!< 0x00000020 */
#define I2C_ICR_STOPCF I2C_ICR_STOPCF_Msk                /*!< STOP detection clear flag */
#define I2C_ICR_BERRCF_Pos (8U)
#define I2C_ICR_BERRCF_Msk (0x1UL << I2C_ICR_BERRCF_Pos) /*!< 0x00000100 */
#define I2C_ICR_BERRCF I2C_ICR_BERRCF_Msk                /*!< Bus error clear flag */
#define I2C_ICR_ARLOCF_Pos (9U)
#define I2C_ICR_ARLOCF_Msk (0x1UL << I2C_ICR_ARLOCF_Pos) /*!< 0x00000200 */
#define I2C_ICR_ARLOCF I2C_ICR_ARLOCF_Msk                /*!< Arbitration lost clear flag */
#define I2C_ICR_OVRCF_Pos (10U)
#define I2C_ICR_OVRCF_Msk (0x1UL << I2C_ICR_OVRCF_Pos) /*!< 0x00000400 */
#define I2C_ICR_OVRCF I2C_ICR_OVRCF_Msk                /*!< Overrun/Underrun clear flag */
#define I2C_ICR_PECCF_Pos (11U)
#define I2C_ICR_PECCF_Msk (0x1UL << I2C_ICR_PECCF_Pos) /*!< 0x00000800 */
#define I2C_ICR_PECCF I2C_ICR_PECCF_Msk                /*!< PAC error clear flag */
#define I2C_ICR_TIMOUTCF_Pos (12U)
#define I2C_ICR_TIMOUTCF_Msk (0x1UL << I2C_ICR_TIMOUTCF_Pos) /*!< 0x00001000 */
#define I2C_ICR_TIMOUTCF I2C_ICR_TIMOUTCF_Msk                /*!< Timeout clear flag */
#define I2C_ICR_ALERTCF_Pos (13U)
#define I2C_ICR_ALERTCF_Msk (0x1UL << I2C_ICR_ALERTCF_Pos) /*!< 0x00002000 */
#define I2C_ICR_ALERTCF I2C_ICR_ALERTCF_Msk                /*!< Alert clear flag */

/******************  Bit definition for I2C_PECR register  *********************/
#define I2C_PECR_PEC_Pos (0U)
#define I2C_PECR_PEC_Msk (0xFFUL << I2C_PECR_PEC_Pos) /*!< 0x000000FF */
#define I2C_PECR_PEC I2C_PECR_PEC_Msk                 /*!< PEC register */

/******************  Bit definition for I2C_RXDR register  *********************/
#define I2C_RXDR_RXDATA_Pos (0U)
#define I2C_RXDR_RXDATA_Msk (0xFFUL << I2C_RXDR_RXDATA_Pos) /*!< 0x000000FF */
#define I2C_RXDR_RXDATA I2C_RXDR_RXDATA_Msk                 /*!< 8-bit receive data */

/******************  Bit definition for I2C_TXDR register  *********************/
#define I2C_TXDR_TXDATA_Pos (0U)
#define I2C_TXDR_TXDATA_Msk (0xFFUL << I2C_TXDR_TXDATA_Pos) /*!< 0x000000FF */
#define I2C_TXDR_TXDATA I2C_TXDR_TXDATA_Msk                 /*!< 8-bit transmit data */

#endif  // __REGISTER_BITS_H__