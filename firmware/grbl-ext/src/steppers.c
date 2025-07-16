
#include "memory_map.h"
#include "steppers.h"

void init_steppers()
{
    // Inti X stepper
    GPIOB->MODER &= ~(MODER_MSK << (STEP_X_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_X_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_X_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_X_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_X_EN_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_X_EN_POS * MODER_BIT_COUNT));
    GPIOB->ODR |= STEP_X_DIR; // Set initial direction to forward
    GPIOB->ODR |= STEP_X_EN;  // Disable stepper

    // GPIOB->ODR &= ~STEP_X_EN; // Enable stepper

    // Init Y stepper
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Y_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Y_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_EN_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Y_EN_POS * MODER_BIT_COUNT));
    GPIOB->ODR |= STEP_Y_DIR; // Set initial direction to forward
    GPIOB->ODR |= STEP_Y_EN;  // Disable stepper

    // Init Z1 stepper
    GPIOC->MODER &= ~(MODER_MSK << (STEP_Z1_DIR_POS * MODER_BIT_COUNT));
    GPIOC->MODER |= (MODER_OUT << (STEP_Z1_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z1_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z1_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z1_EN_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z1_EN_POS * MODER_BIT_COUNT));
    GPIOB->ODR |= STEP_Z1_DIR; // Set initial direction to forward
    GPIOB->ODR |= STEP_Z1_EN;  // Disable stepper

    // Int Z2 stepper
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z2_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z2_DIR_POS * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z2_STEP_POS * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z2_STEP_POS * MODER_BIT_COUNT));
    GPIOD->MODER &= ~(MODER_MSK << (STEP_Z2_EN_POS * MODER_BIT_COUNT));
    GPIOD->MODER |= (MODER_OUT << (STEP_Z2_EN_POS * MODER_BIT_COUNT));
    GPIOB->ODR |= STEP_Z2_DIR; // Set initial direction to forward
    GPIOB->ODR |= STEP_Z2_EN;  // Disable stepper
}