#ifndef __HAL_H__
#define __HAL_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "irq.h"

#define RESET_BIT 0
#define FEED_HOLD_BIT 1
#define CYCLE_START_BIT 2
#define SAFETY_DOOR_BIT 1
#define CONTROL_MASK ((1 << RESET_BIT) | (1 << FEED_HOLD_BIT) | (1 << CYCLE_START_BIT) | (1 << SAFETY_DOOR_BIT))
#define CONTROL_INVERT_MASK CONTROL_MASK

#define PROBE_BIT 5
#define PROBE_MASK (1 << PROBE_BIT)

#define STEPPERS_DISABLE_BIT 0
#define STEPPERS_DISABLE_MASK (1 << STEPPERS_DISABLE_BIT)

#define X_STEP_BIT 2
#define Y_STEP_BIT 3
#define Z_STEP_BIT 4
#define STEP_MASK ((1 << X_STEP_BIT) | (1 << Y_STEP_BIT) | (1 << Z_STEP_BIT))

#define X_DIRECTION_BIT 5
#define Y_DIRECTION_BIT 6
#define Z_DIRECTION_BIT 7
#define DIRECTION_MASK ((1 << X_DIRECTION_BIT) | (1 << Y_DIRECTION_BIT) | (1 << Z_DIRECTION_BIT))

#define X_LIMIT_BIT 1
#define Y_LIMIT_BIT 2
#define Z_LIMIT_BIT 3
#define LIMIT_MASK ((1 << X_LIMIT_BIT) | (1 << Y_LIMIT_BIT) | (1 << Z_LIMIT_BIT))

void interrupts_enable();
void interrupts_disable();

void board_init_hal();

void system_init_hal();

void serial_init_hal();
void serial_tx_enable_hal();
void serial_tx_disable_hal();
void serial_data_received(uint8_t data);
uint8_t serial_data_can_send();

void stepper_interrupt();
void stepper_init_hal();
void steppers_enable_hal(bool invert);

void coolant_init_hal();
void coolant_stop_hal();
void coolant_set_state_hal(uint8_t mode);

void spindle_init_hal();
void spindle_stop_hal();
void spindle_set_state_hal(uint8_t state, float rpm);

void limits_triggered();
void limits_init_hal();
uint8_t limits_get_state_hal();

void probe_init_hal();
uint8_t probe_get_state_hal();

uint8_t eeprom_get_char_hal();
void eeprom_put_char_hal(uint32_t addr, uint8_t new_value);

#endif  // __HAL_H__