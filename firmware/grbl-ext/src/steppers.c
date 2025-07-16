
#include "memory_map.h"
#include "steppers.h"

void init_steppers()
{
    // Inti X stepper
    GPIOB->MODER &= ~(MODER_MSK << (STEP_X_DIR * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_X_DIR * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_X_STEP * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_X_STEP * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_X_EN * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_X_EN * MODER_BIT_COUNT));
    GPIOB->ODR |= (1 << STEP_X_DIR); // Set initial direction to forward
    GPIOB->ODR &= ~(1 << STEP_X_EN); // // Enable stepper by pulling EN low

    // Init Y stepper
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_DIR * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Y_DIR * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_STEP * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Y_STEP * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Y_EN * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Y_EN * MODER_BIT_COUNT));
    GPIOB->ODR |= STEP_Y_DIR; // Set initial direction to forward

    // Init Z1 stepper
    GPIOC->MODER &= ~(MODER_MSK << (STEP_Z1_DIR * MODER_BIT_COUNT));
    GPIOC->MODER |= (MODER_OUT << (STEP_Z1_DIR * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z1_STEP * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z1_STEP * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z1_EN * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z1_EN * MODER_BIT_COUNT));
    GPIOC->ODR |= STEP_Z1_DIR; // Set initial direction to forward

    // Int Z2 stepper
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z2_DIR * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z2_DIR * MODER_BIT_COUNT));
    GPIOB->MODER &= ~(MODER_MSK << (STEP_Z2_STEP * MODER_BIT_COUNT));
    GPIOB->MODER |= (MODER_OUT << (STEP_Z2_STEP * MODER_BIT_COUNT));
    GPIOD->MODER &= ~(MODER_MSK << (STEP_Z2_EN * MODER_BIT_COUNT));
    GPIOD->MODER |= (MODER_OUT << (STEP_Z2_EN * MODER_BIT_COUNT));
    GPIOB->ODR |= STEP_Z2_DIR; // Set initial direction to forward
}