#include <stdbool.h>

#include "board_hal.h"
#include "clock.h"
#include "motion.h"

#define STEP_X_DIR_POS (BIT_12_POS)         // PB12
#define STEP_X_DIR (1 << STEP_X_DIR_POS)    //
#define STEP_X_STEP_POS (BIT_13_POS)        // PB13
#define STEP_X_STEP (1 << STEP_X_STEP_POS)  //
#define STEP_X_EN_POS (BIT_14_POS)          // PB14
#define STEP_X_EN (1 << STEP_X_EN_POS)      //

#define STEP_Y_DIR_POS (BIT_02_POS)         // PB2
#define STEP_Y_DIR (1 << STEP_Y_DIR_POS)    //
#define STEP_Y_STEP_POS (BIT_10_POS)        // PB10
#define STEP_Y_STEP (1 << STEP_Y_STEP_POS)  //
#define STEP_Y_EN_POS (BIT_11_POS)          // PB11
#define STEP_Y_EN (1 << STEP_Y_EN_POS)      //

#define STEP_Z_DIR_POS (BIT_05_POS)         // PC5
#define STEP_Z_DIR (1 << STEP_Z_DIR_POS)    //
#define STEP_Z_STEP_POS (BIT_00_POS)        // PB0
#define STEP_Z_STEP (1 << STEP_Z_STEP_POS)  //
#define STEP_Z_EN_POS (BIT_01_POS)          // PB1
#define STEP_Z_EN (1 << STEP_Z_EN_POS)      //

#define STEP_E_DIR_POS (BIT_04_POS)         // PB4
#define STEP_E_DIR (1 << STEP_E_DIR_POS)    //
#define STEP_E_STEP_POS (BIT_03_POS)        // PB3
#define STEP_E_STEP (1 << STEP_E_STEP_POS)  //
#define STEP_E_EN_POS (BIT_01_POS)          // PD1
#define STEP_E_EN (1 << STEP_E_EN_POS)      //

volatile MotionState motion;

static inline void step_low_X() {
  GPIOB->BSRR = STEP_X_STEP << 16;  // Reset pin (LOW)
}

static inline void step_high_X() {
  GPIOB->BSRR = STEP_X_STEP;  // Set pin (HIGH)
}

static inline void set_dir_X(int32_t dir) {
  if (dir > 0) {
    GPIOB->BSRR = STEP_X_DIR;        // Set pin (HIGH)
  } else {                           //
    GPIOB->BSRR = STEP_X_DIR << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_X(bool enable) {
  if (enable) {
    GPIOB->BSRR = STEP_X_EN << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOB->BSRR = STEP_X_EN;        // Set pin (HIGH)
  }
}

static inline void step_low_Y() {
  GPIOB->BSRR = STEP_Y_STEP << 16;  // Reset pin (LOW)
}

static inline void step_high_Y() {
  GPIOB->BSRR = STEP_Y_STEP;  // Set pin (HIGH)
}

static inline void set_dir_Y(int32_t dir) {
  if (dir > 0) {
    GPIOB->BSRR = STEP_Y_DIR;        // Set pin (HIGH)
  } else {                           //
    GPIOB->BSRR = STEP_Y_DIR << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_Y(bool enable) {
  if (enable) {
    GPIOB->BSRR = STEP_Y_EN << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOB->BSRR = STEP_Y_EN;        // Set pin (HIGH)
  }
}

static inline void step_low_Z() {
  GPIOB->BSRR = STEP_Z_STEP << 16;  // Reset pin (LOW)
}

static inline void step_high_Z() {
  GPIOB->BSRR = STEP_Z_STEP;  // Set pin (HIGH)
}

static inline void set_dir_Z(int32_t dir) {
  if (dir > 0) {
    GPIOC->BSRR = STEP_Z_DIR;        // Set pin (HIGH)
  } else {                           //
    GPIOC->BSRR = STEP_Z_DIR << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_Z(bool enable) {
  if (enable) {
    GPIOB->BSRR = STEP_Z_EN << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOB->BSRR = STEP_Z_EN;        // Set pin (HIGH)
  }
}

static inline void step_low_E() {
  GPIOB->BSRR = STEP_E_STEP << 16;  // Reset pin (LOW)
}

static inline void step_high_E() {
  GPIOB->BSRR = STEP_E_STEP;  // Set pin (HIGH)
}

static inline void set_dir_E(int32_t dir) {
  if (dir > 0) {
    GPIOB->BSRR = STEP_E_DIR;        // Set pin (HIGH)
  } else {                           //
    GPIOB->BSRR = STEP_E_DIR << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_E(bool enable) {
  if (enable) {
    GPIOD->BSRR = STEP_E_EN << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOD->BSRR = STEP_E_EN;        // Set pin (HIGH)
  }
}

extern int32_t stepper_cooling_fan_run_on_start;

void steppers_enable_hal(bool enable) {
  set_ena_X(enable);
  set_ena_Y(enable);
  set_ena_Z(enable);
  set_ena_E(enable);

  // Stepper cooling fan
  if (enable) {
    GPIOC->BSRR = BIT_06;  // Turn Fan 0 on
  } else {
    // TODO: this should be a helper method to set fan run on, not direct manipulation
    stepper_cooling_fan_run_on_start = get_elapsed_seconds();
  }
}

void stepper_init_hal() {
  // Init X stepper
  GPIO_SET_MODE(GPIOB, STEP_X_DIR_POS, MODER_OUT);   // Set X direction to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_X_STEP_POS, MODER_OUT);  // Set X step to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_X_EN_POS, MODER_OUT);    // Set X enable to output pin mode
  GPIOB->ODR |= STEP_X_DIR;                          // Set initial direction to forward
  GPIOB->ODR |= STEP_X_EN;                           // Disable stepper
  GPIOB->ODR &= ~STEP_X_STEP;                        // Set state low

  // Init Y stepper
  GPIO_SET_MODE(GPIOB, STEP_Y_DIR_POS, MODER_OUT);   // Set Y direction to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Y_STEP_POS, MODER_OUT);  // Set Y step to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Y_EN_POS, MODER_OUT);    // Set Y enable to output pin modes
  GPIOB->ODR |= STEP_Y_DIR;                          // Set initial direction to forward
  GPIOB->ODR |= STEP_Y_EN;                           // Disable stepper
  GPIOB->ODR &= ~STEP_Y_STEP;                        // Set state low

  // Init Z stepper
  GPIO_SET_MODE(GPIOC, STEP_Z_DIR_POS, MODER_OUT);   // Set Z direction to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Z_STEP_POS, MODER_OUT);  // Set Z step to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Z_EN_POS, MODER_OUT);    // Set Z enable to output pin mode
  GPIOC->ODR |= STEP_Z_DIR;                          // Set initial direction to forward
  GPIOB->ODR |= STEP_Z_EN;                           // Disable stepper
  GPIOB->ODR &= ~STEP_Z_STEP;                        // Set state low

  // Init E stepper
  GPIO_SET_MODE(GPIOB, STEP_E_DIR_POS, MODER_OUT);   // Set E direction to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_E_STEP_POS, MODER_OUT);  // Set E step to output pin mode
  GPIO_SET_MODE(GPIOD, STEP_E_EN_POS, MODER_OUT);    // Set E enable to output pin mode
  GPIOB->ODR |= STEP_E_DIR;                          // Set initial direction to forward
  GPIOD->ODR |= STEP_E_EN;                           // Disable stepper
  GPIOB->ODR &= ~STEP_E_STEP;                        // Set state low

  motion.x.step_low = step_low_X;
  motion.x.step_high = step_high_X;
  motion.x.set_dir = set_dir_X;
  motion.x.set_ena = set_ena_X;

  motion.y.step_low = step_low_Y;
  motion.y.step_high = step_high_Y;
  motion.y.set_dir = set_dir_Y;
  motion.y.set_ena = set_ena_Y;

  motion.z.step_low = step_low_Z;
  motion.z.step_high = step_high_Z;
  motion.z.set_dir = set_dir_Z;
  motion.z.set_ena = set_ena_Z;

  motion.e.step_low = step_low_E;
  motion.e.step_high = step_high_E;
  motion.e.set_dir = set_dir_E;
  motion.e.set_ena = set_ena_E;
}
