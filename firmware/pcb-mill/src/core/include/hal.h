#ifndef __HAL_H__
#define __HAL_H__

#include <stdint.h>
#include <stdbool.h>

#include "build_config.h"
#include "board_hal.h"
#include "machine_config.h"

// A board must define this method to allow initialisation of board specific HAL
void board_init_hal();

// When there is a critical failure then the board goes into halt mode
void device_halt();

void delay_ms(uint32_t ms);

void steppers_enable_hal(bool enable);
void stepper_interrupt();

// Initialise limit detection
void limits_init_hal();

#endif  // __HAL_H__