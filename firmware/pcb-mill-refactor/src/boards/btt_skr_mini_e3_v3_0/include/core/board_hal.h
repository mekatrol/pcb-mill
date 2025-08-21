#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include <stdint.h>

// IMPORTANT: header include order matters
#include "stm32g0xx.h"
#include "stm32g0b1xx.h"
#include "core_cm0plus.h"

#include "diagnostics.h"

#define BIT_00_POS 0
#define BIT_00 (1U << BIT_00_POS)
#define BIT_01_POS 1
#define BIT_01 (1U << BIT_01_POS)
#define BIT_02_POS 2
#define BIT_02 (1U << BIT_02_POS)
#define BIT_03_POS 3
#define BIT_03 (1U << BIT_03_POS)
#define BIT_04_POS 4
#define BIT_04 (1U << BIT_04_POS)
#define BIT_05_POS 5
#define BIT_05 (1U << BIT_05_POS)
#define BIT_06_POS 6
#define BIT_06 (1U << BIT_06_POS)
#define BIT_07_POS 7
#define BIT_07 (1U << BIT_07_POS)
#define BIT_08_POS 8
#define BIT_08 (1U << BIT_08_POS)
#define BIT_09_POS 9
#define BIT_09 (1U << BIT_09_POS)
#define BIT_10_POS 10
#define BIT_10 (1U << BIT_10_POS)
#define BIT_11_POS 11
#define BIT_11 (1U << BIT_11_POS)
#define BIT_12_POS 12
#define BIT_12 (1U << BIT_12_POS)
#define BIT_13_POS 13
#define BIT_13 (1U << BIT_13_POS)
#define BIT_14_POS 14
#define BIT_14 (1U << BIT_14_POS)
#define BIT_15_POS 15
#define BIT_15 (1U << BIT_15_POS)
#define BIT_16_POS 16
#define BIT_16 (1U << BIT_16_POS)
#define BIT_17_POS 17
#define BIT_17 (1U << BIT_17_POS)
#define BIT_18_POS 18
#define BIT_18 (1U << BIT_18_POS)
#define BIT_19_POS 19
#define BIT_19 (1U << BIT_19_POS)
#define BIT_20_POS 20
#define BIT_20 (1U << BIT_20_POS)
#define BIT_21_POS 21
#define BIT_21 (1U << BIT_21_POS)
#define BIT_22_POS 22
#define BIT_22 (1U << BIT_22_POS)
#define BIT_23_POS 23
#define BIT_23 (1U << BIT_23_POS)
#define BIT_24_POS 24
#define BIT_24 (1U << BIT_24_POS)
#define BIT_25_POS 25
#define BIT_25 (1U << BIT_25_POS)
#define BIT_26_POS 26
#define BIT_26 (1U << BIT_26_POS)
#define BIT_27_POS 27
#define BIT_27 (1U << BIT_27_POS)
#define BIT_28_POS 28
#define BIT_28 (1U << BIT_28_POS)
#define BIT_29_POS 29
#define BIT_29 (1U << BIT_29_POS)
#define BIT_30_POS 30
#define BIT_30 (1U << BIT_30_POS)
#define BIT_31_POS 31
#define BIT_31 (1U << BIT_31_POS)

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

#define USART_BRR(PCLK, BAUD) (((PCLK) + ((BAUD) / 2)) / (BAUD))

static inline __attribute__((always_inline)) void interrupts_enable() {
  __enable_irq();
}

static inline __attribute__((always_inline)) void interrupts_disable() {
  __disable_irq();
}

#endif  // __MEMORY_MAP_H__