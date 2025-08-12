#include "board_hal.h"
#include "dcd.h"

// TODO: remove
#include "fsdev_type.h"

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

void usb_init_hal() {
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

__attribute__((always_inline)) static inline uint32_t usb_endpoint_read(uint32_t endpoint_id) {
  return USB->chep[endpoint_id].CHEPnR;
}

void USB_UCPD1_2_IRQHandler() {
  uint32_t int_status = USB->ISTR;

  if (int_status & USB_ISTR_SOF) {
    USB->ISTR = ~USB_ISTR_SOF;
    // Start of Frame
  }

  if (int_status & USB_ISTR_RESET) {
    // Clear reset flag
    USB->ISTR = ~USB_ISTR_RESET;

    // Call driver reset
    usb_reset();

    // Return after resetting USB as all should be clear
    return;
  }

  if (int_status & USB_ISTR_WKUP) {
    USB->CNTR &= ~USB_CNTR_SUSPRDY;
    USB->CNTR &= ~USB_CNTR_SUSPEN;

    USB->ISTR = ~USB_ISTR_WKUP;
  }

  if (int_status & USB_ISTR_SUSP) {
    /* Suspend is asserted for both suspend and unplug events. without Vbus monitoring,
     * these events cannot be differentiated, so we only trigger suspend. */

    /* Force low-power mode in the macrocell */
    USB->CNTR |= USB_CNTR_SUSPEN;
    USB->CNTR |= USB_CNTR_SUSPRDY;

    /* clear of the ISTR bit must be done after setting of CNTR_FSUSP */
    USB->ISTR = ~USB_ISTR_SUSP;
  }

  if (int_status & USB_ISTR_ESOF) {
    USB->ISTR = ~USB_ISTR_ESOF;
  }

  // loop to handle all pending CTR interrupts
  while (USB->ISTR & USB_ISTR_CTR) {
    uint32_t const endpoint_id = USB->ISTR & USB_ISTR_IDN;
    uint32_t const endpoint_reg = usb_endpoint_read(endpoint_id);

    if (endpoint_reg & USB_EP_VTRX) {
      if (endpoint_reg & USB_EP_SETUP) {
        handle_ctr_setup(endpoint_id);  // CTR will be clear after copied setup packet
      } else {
        usb_endpoint_write_clear_ctr(endpoint_id, USB_ENDPOINT_DIRECTION_OUT);
        handle_ctr_rx(endpoint_id);
      }
    }

    if (endpoint_reg & USB_EP_VTTX) {
      usb_endpoint_write_clear_ctr(endpoint_id, USB_ENDPOINT_DIRECTION_IN);
      handle_ctr_tx(endpoint_id);
    }
  }

  if (int_status & USB_ISTR_PMAOVR) {
    USB->ISTR = ~USB_ISTR_PMAOVR;

    // TODO: overrun/underrun
  }
}
