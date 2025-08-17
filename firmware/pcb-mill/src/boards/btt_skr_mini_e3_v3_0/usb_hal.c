#include "board_hal.h"
#include "usb_hal.h"

__attribute__((always_inline)) static inline void usb_ep_clear_correct_transfer(uint32_t ep_idn, usb_ep_direction_index_t ep_idn_idx) {
  // Correct transfer interupt flags are:
  //  (VT == valid transation)
  //  TX -> USB_CHEP_VTTX
  //  RX -> USB_CHEP_VTRX

  // Get current register value
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;

  // Clear THREE_ERR_RX, THREE_ERR_TX, DTOGRX, STATRX, DTOGTX, STATTX (ep_reg & 0x07FF8F8F)
  ep_reg &= USB_CHEP_REG_MASK;

  // Clear USB_CHEP_VTTX or USB_CHEP_VTRX depending on ep_idn_idx
  ep_reg &= ~(1 << (ep_idn_idx == USB_EP_DIRECTION_IN_IDX ? USB_CHEP_VTTX_Pos : USB_CHEP_VTRX_Pos));

  // CHEPnR has many rc_w0 bits. This means we set a bit:
  //    0 = clear that bit when writing the the CHEPnR
  //    1 = no change to the bit when writing the the CHEPnR
  usb_ep_reg_set(ep_idn, ep_reg, false);
}

static void setup_received(usb_control_request_t *setup_received) {
  // Setup recieved, therefore host has connected to device
  usb_device.connected = 1;

  // Reset state
  usb_device.ep_status[EP0_IDN][USB_EP_DIRECTION_OUT_IDX].busy = 0;
  usb_device.ep_status[EP0_IDN][USB_EP_DIRECTION_OUT_IDX].claimed = 0;
  usb_device.ep_status[EP0_IDN][USB_EP_DIRECTION_IN_IDX].busy = 0;
  usb_device.ep_status[EP0_IDN][USB_EP_DIRECTION_IN_IDX].claimed = 0;

  // Process control request
  if (!process_control_request(setup_received)) {
    // USB 2.0 Specification, Section 9.2.7, “Error Handling”
    // If a device detects a condition that prevents it from completing the request, it must indicate the error by returning a STALL handshake.
    // For control transfers, the device must respond with a STALL to any setup or data stage packet it cannot handle.
    usb_ep_stall_set(EP0_IDN | USB_DIR_IN);
    usb_ep_stall_set(EP0_IDN | USB_DIR_OUT);
  }
}

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

void usb_ep_setup(uint32_t ep_idn) {
  uint16_t rx_count = usb_pma_get_count(ep_idn, USB_EP_RX_BUFFER);
  uint16_t rx_addr = usb_pma_get_ep_addr(ep_idn, USB_EP_RX_BUFFER);

  __attribute__((aligned(4)))
  uint8_t setup_packet[8];

  usb_rx_packet(setup_packet, rx_addr, rx_count);

  // Clear correct transfer flag
  usb_ep_clear_correct_transfer(ep_idn, USB_EP_DIRECTION_OUT_IDX);

  if (rx_count == sizeof(usb_control_request_t)) {
    setup_received((usb_control_request_t *)setup_packet);
  } else {
    // Something went wrong, reset the endpoint state (by resetting size, which clears count etc)
    usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
  }
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
    // These bits are written by the hardware according to the host channel or device endpoint number
    const uint32_t ep_idn = USB->ISTR & USB_ISTR_IDN;
    const uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;

    if (ep_reg & USB_EP_VTRX) {
      // Was a setup transaction received?
      if (ep_reg & USB_EP_SETUP) {
        // Setup processing clears the USB_EP_VTRX flag after receiving data
        usb_ep_setup(ep_idn);
      } else {
        // Clear USB_EP_VTRX
        usb_ep_clear_correct_transfer(ep_idn, USB_EP_DIRECTION_OUT_IDX);

        // Receive data
        usb_ep_rx(ep_idn);
      }
    }

    if (ep_reg & USB_EP_VTTX) {
      // Clear USB_EP_VTTX
      usb_ep_clear_correct_transfer(ep_idn, USB_EP_DIRECTION_IN_IDX);

      // Transmit next batch of queued data (or complete transfer if none remianing in queue)
      usb_ep_tx_queued_bytes(ep_idn);
    }
  }

  if (int_status & USB_ISTR_PMAOVR) {
    USB->ISTR = ~USB_ISTR_PMAOVR;

    // TODO: overrun/underrun
  }
}

bool usb_rx_packet(void *__restrict dst, uint16_t src, uint16_t byte_count) {
  if (byte_count == 0) {
    // No count then nothing to read
    return true;
  }

  // We are readng 32 bit values from unaligned byte locations
  uint32_t read_count = byte_count / sizeof(uint32_t);

  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + src);
  uint8_t *dst8 = (uint8_t *)dst;

  while (read_count--) {
    unaligned_write_32(dst8, (uint32_t)(*pma_buf));
    dst8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = *pma_buf;
    while (odd--) {
      *dst8++ = (uint8_t)(temp & 0xffUL);
      temp >>= 8;
    }
  }

  return true;
}

void usb_device_init() {
  // Perform USB peripheral reset
  USB->CNTR = USB_CNTR_USBRST | USB_CNTR_PDWN;
  USB->CNTR &= ~USB_CNTR_PDWN;
  USB->CNTR = 0;  // Enable USB
  USB->ISTR = 0;  // Clear pending interrupts

  // Reset endpoints to disabled
  for (uint32_t i = 0; i < USB_EP_MAX; i++) {
    // This doesn't clear all bits since some bits are "toggle", but does set the type to DISABLED.
    usb_ep_reg_set(i, 0, false);
  }

  USB->CNTR |= USB_CNTR_RESETM | USB_CNTR_ESOFM | USB_CNTR_CTRM |
               USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_PMAOVRM;

  usb_hal_reset();

  // Enable pull up to tell host it can enumerate device
  USB->BCDR |= USB_BCDR_DPPU;
}

void usb_sof_set_enable(bool enable) {
  if (enable) {
    USB->CNTR |= USB_CNTR_SOFM;
  } else {
    USB->CNTR &= ~USB_CNTR_SOFM;
  }
}

void usb_hal_reset() {
  // Disable USB
  USB->DADDR = 0U;

  // Reset endpoints
  usb_ep_reset();

  // EP0 must exist
  usb_ep_control_init();

  // Enable USB
  USB->DADDR = USB_DADDR_EF;
}
