#ifndef __STEPPER_H__
#define __STEPPER_H__

#include <stdint.h>

#include "../../grbl/grbl.h"
#include "register_bits.h"

#define STEP_X_DIR_POS (BIT_12)             // PB12
#define STEP_X_DIR (1 << STEP_X_DIR_POS)    //
#define STEP_X_STEP_POS (BIT_13)            // PB13
#define STEP_X_STEP (1 << STEP_X_STEP_POS)  //
#define STEP_X_EN_POS (BIT_14)              // PB14
#define STEP_X_EN (1 << STEP_X_EN_POS)      //

#define STEP_Y_DIR_POS (BIT_02)             // PB2
#define STEP_Y_DIR (1 << STEP_Y_DIR_POS)    //
#define STEP_Y_STEP_POS (BIT_10)            // PB10
#define STEP_Y_STEP (1 << STEP_Y_STEP_POS)  //
#define STEP_Y_EN_POS (BIT_11)              // PB11
#define STEP_Y_EN (1 << STEP_Y_EN_POS)      //

#define STEP_Z1_DIR_POS (BIT_05)              // PC5
#define STEP_Z1_DIR (1 << STEP_Z1_DIR_POS)    //
#define STEP_Z1_STEP_POS (BIT_00)             // PB0
#define STEP_Z1_STEP (1 << STEP_Z1_STEP_POS)  //
#define STEP_Z1_EN_POS (BIT_01)               // PB1
#define STEP_Z1_EN (1 << STEP_Z1_EN_POS)      //

#define STEP_Z2_DIR_POS (BIT_04)              // PB4
#define STEP_Z2_DIR (1 << STEP_Z2_DIR_POS)    //
#define STEP_Z2_STEP_POS (BIT_03)             // PB3
#define STEP_Z2_STEP (1 << STEP_Z2_STEP_POS)  //
#define STEP_Z2_EN_POS (BIT_01)               // PD1
#define STEP_Z2_EN (1 << STEP_Z2_EN_POS)      //

void init_steppers();

void stepper_enable_x();
void stepper_disable_x();
void stepper_x_set_dir(rotation_t rotation_direction);

void stepper_enable_y();
void stepper_disable_y();
void stepper_y_set_dir(rotation_t rotation_direction);

void stepper_enable_z();
void stepper_disable_z();
void stepper_z_set_dir(rotation_t rotation_direction);

#endif  // __STEPPER_H__