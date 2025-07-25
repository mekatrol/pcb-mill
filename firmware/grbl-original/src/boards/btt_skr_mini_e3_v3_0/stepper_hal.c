#include <stdbool.h>

#include "memory_map.h"
#include "motion.h"
#include "register_bits.h"

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

#define STEP_Z1_DIR_POS (BIT_05_POS)          // PC5
#define STEP_Z1_DIR (1 << STEP_Z1_DIR_POS)    //
#define STEP_Z1_STEP_POS (BIT_00_POS)         // PB0
#define STEP_Z1_STEP (1 << STEP_Z1_STEP_POS)  //
#define STEP_Z1_EN_POS (BIT_01_POS)           // PB1
#define STEP_Z1_EN (1 << STEP_Z1_EN_POS)      //

#define STEP_Z2_DIR_POS (BIT_04_POS)          // PB4
#define STEP_Z2_DIR (1 << STEP_Z2_DIR_POS)    //
#define STEP_Z2_STEP_POS (BIT_03_POS)         // PB3
#define STEP_Z2_STEP (1 << STEP_Z2_STEP_POS)  //
#define STEP_Z2_EN_POS (BIT_01_POS)           // PD1
#define STEP_Z2_EN (1 << STEP_Z2_EN_POS)      //

volatile MotionState motion;

static inline void step_low_X() {
  GPIOB->BSRR = (1 << 13) << 16;  // Reset pin (LOW)
}

static inline void step_high_X() {
  GPIOB->BSRR = (1 << 13);  // Set pin (HIGH)
}

static inline void set_dir_X(int32_t dir) {
  if (dir > 0) {
    GPIOB->BSRR = (1 << 12);        // Set pin (HIGH)
  } else {                          //
    GPIOB->BSRR = (1 << 12) << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_X(int32_t enable) {
  if (enable > 0) {
    GPIOB->BSRR = (1 << 14) << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOB->BSRR = (1 << 14);        // Set pin (HIGH)
  }
}

static inline void step_low_Y() {
  GPIOB->BSRR = (1 << 10) << 16;  // Reset pin (LOW)
}

static inline void step_high_Y() {
  GPIOB->BSRR = (1 << 10);  // Set pin (HIGH)
}

static inline void set_dir_Y(int32_t dir) {
  if (dir > 0) {
    GPIOB->BSRR = (1 << 2);        // Set pin (HIGH)
  } else {                         //
    GPIOB->BSRR = (1 << 2) << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_Y(int32_t enable) {
  if (enable > 0) {
    GPIOB->BSRR = (1 << 11) << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOB->BSRR = (1 << 11);        // Set pin (HIGH)
  }
}

static inline void step_low_Z() {
  GPIOB->BSRR = (1 << 0) << 16;  // Reset pin (LOW)
  GPIOB->BSRR = (1 << 3) << 16;  // Reset pin (LOW)
}

static inline void step_high_Z() {
  GPIOB->BSRR = (1 << 0) << 0;  // Set pin (HIGH)
  GPIOB->BSRR = (1 << 3) << 0;  // Set pin (HIGH)
}

static inline void set_dir_Z(int32_t dir) {
  if (dir > 0) {
    GPIOB->BSRR = (1 << 1);        // Set pin (HIGH)
    GPIOD->BSRR = (1 << 1);        // Set pin (HIGH)
  } else {                         //
    GPIOB->BSRR = (1 << 1) << 16;  // Reset pin (LOW)
    GPIOD->BSRR = (1 << 1) << 16;  // Reset pin (LOW)
  }
}

static inline void set_ena_Z(int32_t enable) {
  if (enable > 0) {
    GPIOB->BSRR = (1 << 14) << 16;  // Reset pin (LOW)
    GPIOB->BSRR = (1 << 14) << 16;  // Reset pin (LOW)
  } else {                          //
    GPIOB->BSRR = (1 << 14);        // Set pin (HIGH)
    GPIOB->BSRR = (1 << 14);        // Set pin (HIGH)
  }
}

void steppers_enable_hal(bool invert) {
  set_ena_X(1);
  set_ena_Y(1);
  set_ena_Z(1);
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

  // Init Z1 stepper
  GPIO_SET_MODE(GPIOB, STEP_Z1_DIR_POS, MODER_OUT);   // Set Z1 direction to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Z1_STEP_POS, MODER_OUT);  // Set Z1 step to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Z1_EN_POS, MODER_OUT);    // Set Z1 enable to output pin mode
  GPIOC->ODR |= STEP_Z1_DIR;                          // Set initial direction to forward
  GPIOB->ODR |= STEP_Z1_EN;                           // Disable stepper
  GPIOB->ODR &= ~STEP_Z1_STEP;                        // Set state low

  // Init Z2 stepper
  GPIO_SET_MODE(GPIOB, STEP_Z2_DIR_POS, MODER_OUT);   // Set Z2 direction to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Z2_STEP_POS, MODER_OUT);  // Set Z2 step to output pin mode
  GPIO_SET_MODE(GPIOB, STEP_Z2_EN_POS, MODER_OUT);    // Set Z2 enable to output pin mode
  GPIOB->ODR |= STEP_Z2_DIR;                          // Set initial direction to forward
  GPIOD->ODR |= STEP_Z2_EN;                           // Disable stepper
  GPIOB->ODR &= ~STEP_Z2_STEP;                        // Set state low

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
}
