
#include "steppers.h"

#include "memory_map.h"

void init_steppers() {
  // Inti X stepper
  GPIOB->MODER &= ~(MODER_MSK << (STEP_X_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_X_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_X_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_X_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_X_EN_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_X_EN_POS * MODER_BIT_COUNT));
  GPIOB->ODR |= STEP_X_DIR;    // Set initial direction to forward
  GPIOB->ODR |= STEP_X_EN;     // Disable stepper
  GPIOB->ODR &= ~STEP_X_STEP;  // Set state low

  // Init Y stepper
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Y_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Y_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_EN_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Y_EN_POS * MODER_BIT_COUNT));
  GPIOB->ODR |= STEP_Y_DIR;    // Set initial direction to forward
  GPIOB->ODR |= STEP_Y_EN;     // Disable stepper
  GPIOB->ODR &= ~STEP_Y_STEP;  // Set state low

  // Init Z1 stepper
  GPIOC->MODER &= ~(MODER_MSK << (STEP_Z1_DIR_POS * MODER_BIT_COUNT));
  GPIOC->MODER |= (MODER_OUT << (STEP_Z1_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Z1_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Z1_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Z1_EN_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Z1_EN_POS * MODER_BIT_COUNT));
  GPIOB->ODR |= STEP_Z1_DIR;  // Set initial direction to forward
  GPIOB->ODR |= STEP_Z1_EN;   // Disable stepper

  // Int Z2 stepper
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Z2_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Z2_DIR_POS * MODER_BIT_COUNT));
  GPIOB->MODER &= ~(MODER_MSK << (STEP_Z2_STEP_POS * MODER_BIT_COUNT));
  GPIOB->MODER |= (MODER_OUT << (STEP_Z2_STEP_POS * MODER_BIT_COUNT));
  GPIOD->MODER &= ~(MODER_MSK << (STEP_Z2_EN_POS * MODER_BIT_COUNT));
  GPIOD->MODER |= (MODER_OUT << (STEP_Z2_EN_POS * MODER_BIT_COUNT));
  GPIOB->ODR |= STEP_Z2_DIR;  // Set initial direction to forward
  GPIOB->ODR |= STEP_Z2_EN;   // Disable stepper
}

void stepper_enable_x() { GPIOB->ODR &= ~STEP_X_EN; }

void stepper_disable_x() { GPIOB->ODR |= STEP_X_EN; }

void stepper_x_set_dir(rotation_t rotation_direction) {
  if (rotation_direction == ROTATION_CW) {
    GPIOB->ODR |= STEP_X_DIR;
  } else {
    GPIOB->ODR &= ~STEP_X_DIR;
  }
}

void stepper_x_set_state(uint8_t state) {
  if (state == 1) {
    // Rising edge
    GPIOB->ODR |= STEP_X_STEP;
  } else {
    GPIOB->ODR &= ~STEP_X_STEP;  // Falling edge
  }
}

void stepper_enable_y() { GPIOB->ODR &= ~STEP_Y_EN; }

void stepper_disable_y() { GPIOB->ODR |= STEP_Y_EN; }

void stepper_y_set_dir(rotation_t rotation_direction) {
  if (rotation_direction == ROTATION_CW) {
    GPIOB->ODR |= STEP_Y_DIR;
  } else {
    GPIOB->ODR &= ~STEP_Y_DIR;
  }
}

void stepper_y_set_state(uint8_t state) {
  if (state == 1) {
    // Rising edge
    GPIOB->ODR |= STEP_Y_STEP;
  } else {
    // Falling edge
    GPIOB->ODR &= ~STEP_Y_STEP;
  }
}

void stepper_enable_z() {
  GPIOB->ODR |= STEP_Z1_EN;
  GPIOB->ODR |= STEP_Z2_EN;
}

void stepper_disable_z() {
  GPIOB->ODR |= STEP_Z1_EN;
  GPIOB->ODR |= STEP_Z2_EN;
}

void stepper_z_set_dir(rotation_t rotation_direction) {
  if (rotation_direction == ROTATION_CW) {
    GPIOB->ODR |= STEP_Z1_DIR;
    GPIOB->ODR |= STEP_Z2_DIR;
  } else {
    GPIOB->ODR &= ~STEP_Z1_DIR;
    GPIOB->ODR &= ~STEP_Z2_DIR;
  }
}

void stepper_z_set_state(uint8_t state) {
  if (state == 1) {
    // Rising edge
    GPIOB->ODR |= STEP_Z1_STEP;
    GPIOB->ODR |= STEP_Z2_STEP;
  } else {
    // Falling edge
    GPIOB->ODR &= ~STEP_Z1_STEP;
    GPIOB->ODR &= ~STEP_Z2_STEP;
  }
}
