/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#ifdef STM32F1xx
  #include "stm32f1xx_hal.h"
#elif defined(STM32F4xx)
  #include "stm32f4xx_hal.h"
#elif defined(STM32H7xx)
  #include "stm32h7xx_hal.h"
#else
  #error SPI Touch Screen is currently only supported on STM32F1, STM32F4 and STM32H7 hardware.
#endif

#include "../../../inc/MarlinConfig.h"

// Not using regular SPI interface by default to avoid SPI mode conflicts with other SPI devices

#if !PIN_EXISTS(TOUCH_MISO)
  #error "TOUCH_MISO_PIN is not defined."
#elif !PIN_EXISTS(TOUCH_MOSI)
  #error "TOUCH_MOSI_PIN is not defined."
#elif !PIN_EXISTS(TOUCH_SCK)
  #error "TOUCH_SCK_PIN is not defined."
#elif !PIN_EXISTS(TOUCH_CS)
  #error "TOUCH_CS_PIN is not defined."
#endif

#ifndef TOUCH_INT_PIN
  #define TOUCH_INT_PIN  -1
#endif

#if PIN_EXISTS(TOUCH_INT)
  #define XPT2046_DFR_MODE      0x00
#else
  #define XPT2046_DFR_MODE      0x01
#endif
#define XPT2046_SER_MODE        0x04
#define XPT2046_CONTROL         0x80

enum XPTCoordinate : uint8_t {
  XPT2046_X  = 0x10 | XPT2046_CONTROL | XPT2046_DFR_MODE,
  XPT2046_Y  = 0x50 | XPT2046_CONTROL | XPT2046_DFR_MODE,
  XPT2046_Z1 = 0x30 | XPT2046_CONTROL | XPT2046_DFR_MODE,
  XPT2046_Z2 = 0x40 | XPT2046_CONTROL | XPT2046_DFR_MODE,
};

#ifndef XPT2046_Z1_THRESHOLD
  #define XPT2046_Z1_THRESHOLD 10
#endif

class XPT2046 {
private:
  static SPI_HandleTypeDef SPIx;

  static bool isBusy() { return false; }

  static uint16_t getRawData(const XPTCoordinate coordinate);
  static bool isTouched();

  static void dataTransferBegin() { if (SPIx.Instance) { HAL_SPI_Init(&SPIx); } WRITE(TOUCH_CS_PIN, LOW); };
  static void dataTransferEnd() { WRITE(TOUCH_CS_PIN, HIGH); };
  static uint16_t hardwareIO(uint16_t data);
  static uint16_t softwareIO(uint16_t data);
  static uint16_t IO(uint16_t data = 0) { return SPIx.Instance ? hardwareIO(data) : softwareIO(data); }

public:
  static void init();
  static bool getRawPoint(int16_t * const x, int16_t * const y);
};
