#include "board_hal.h"

/****************************************************************************************************************************************
 * HAL public methods
 ****************************************************************************************************************************************/

void usb_init_board_hal() {
  // Set PA11 and PA12 to Alternate Function mode
  GPIO_SET_MODE(GPIOA, BIT_11_POS, MODER_ALT);
  GPIO_SET_MODE(GPIOA, BIT_12_POS, MODER_ALT);

  // Set AF0 (USB) for PA11 and PA12 (AFR = 0)
  GPIOA->AFR[1] &= ~((GPIO_AF_MSK << ((BIT_11_POS - 8) * GPIO_AF_BIT_COUNT)) | (GPIO_AF_MSK << ((BIT_12_POS - 8) * GPIO_AF_BIT_COUNT)));

  // No pull-up/pull-down
  GPIOA->PUPDR &= ~((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));

  // Push-pull output (default)
  GPIOA->OTYPER &= ~(BIT_11 | BIT_12);

  // Very high speed (11b)
  GPIOA->OSPEEDR &= ~((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));
  GPIOA->OSPEEDR |= ((0x11 << (BIT_11_POS * 2)) | (0x11 << (BIT_12_POS * 2)));

  // Set up 48MHz clock for USB
  RCC->CR |= BIT_22;                     // Enable HSI48
  while ((RCC->CR & BIT_23) == 0);       // Wait for HSI48 ready
  RCC->CCIPR2 &= ~(0b11 << BIT_12_POS);  // Select HSI48 as USB clock source

  // Enable USB peripheral clock
  RCC->APBENR1 |= RCC_APBENR1_USBEN;

  // Enable USB IRQ
  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

void USB_UCPD1_2_IRQHandler() {
}