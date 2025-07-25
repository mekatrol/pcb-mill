#include "irq.h"
#include "memory_map.h"
#include "register_bits.h"

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

#define RCC_CCIPR_CLK48SEL_MASK (3 << 28)
#define RCC_CCIPR_CLK48SEL_HSI48 (0 << 28)

void usb_clock_init(void) {
  // Enable HSI48
  RCC->CRRCR |= RCC_CRRCR_HSI48ON;
  while ((RCC->CRRCR & RCC_CRRCR_HSI48RDY) == 0);  // Wait for HSI48 ready

  // Select HSI48 as USB clock source
  RCC->CCIPR &= ~RCC_CCIPR_CLK48SEL_MASK;
  RCC->CCIPR |= RCC_CCIPR_CLK48SEL_HSI48;

  // Enable USB peripheral clock
  RCC->APBENR1 |= RCC_APBENR1_USBEN;

  // Enable USB IRQ
  ENABLE_IRQ(USB_UCPD1_2_IRQn);
}

void USB_UCPD1_2_IRQHandler() {
}
