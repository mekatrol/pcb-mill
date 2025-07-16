#ifndef __STEPPER_H__
#define __STEPPER_H__

#include "register_bits.h"

#define STEP_X_DIR (BIT_12)  // PB12
#define STEP_X_STEP (BIT_13) // PB13
#define STEP_X_EN (BIT_14)   // PB14

#define STEP_Y_DIR (BIT_02)  // PB2
#define STEP_Y_STEP (BIT_10) // PB10
#define STEP_Y_EN (BIT_11)   // PB11

#define STEP_Z1_DIR (BIT_05)  // PC5
#define STEP_Z1_STEP (BIT_00) // PB0
#define STEP_Z1_EN (BIT_01)   // PB1

#define STEP_Z2_DIR (BIT_04)  // PB4
#define STEP_Z2_STEP (BIT_03) // PB3
#define STEP_Z2_EN (BIT_01)   // PD1

void init_steppers();

#endif // __STEPPER_H__