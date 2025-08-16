#include "board_hal.h"
#include "usb_hal.h"

__attribute__((always_inline)) static inline void usb_pma_set_count(uint32_t ep_idn, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  USB_BUFFER_DESC_TABLE->endpoint[ep_idn].buffer[buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline void usb_endpoint_reg_set_clear_ctr(uint32_t ep_idn, usb_endpoint_direction_index_t dir) {
  uint32_t endpoint_reg = USB->chep[ep_idn].CHEPnR;
  endpoint_reg |= USB_EP_VTTX | USB_EP_VTRX;
  endpoint_reg &= USB_CHEP_REG_MASK;
  endpoint_reg &= ~(1 << (USB_CHEP_VTTX_Pos + (dir == USB_EP_DIRECTION_IN_IDX ? 0 : 8)));
  usb_endpoint_reg_set(ep_idn, endpoint_reg, false);
}

__attribute__((always_inline)) static inline uint32_t unaligned_read32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

__attribute__((always_inline)) static inline void unaligned_write32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)(value);
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

static void setup_received(usb_control_request_t *setup_received) {
  // Mark as connected after receiving 1st setup packet.
  // But it is easier to set it every time instead of wasting time to check then set
  usb_device.connected = 1;

  // mark both in & out control as free
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_OUT_IDX].busy = 0;
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_OUT_IDX].claimed = 0;
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_IN_IDX].busy = 0;
  usb_device.ep_status[USB_EP0_ADDR][USB_EP_DIRECTION_IN_IDX].claimed = 0;

  // Process control request
  if (!process_control_request(setup_received)) {
    // Failed -> stall both control endpoint IN and OUT
    usb_endpoint_stall_set(USB_EP0_ADDR | USB_DIR_IN);
    usb_endpoint_stall_set(USB_EP0_ADDR | USB_DIR_OUT);
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

void handle_ctr_setup(uint32_t ep_idn) {
  uint16_t rx_count = usb_pma_get_count(ep_idn, USB_EP_RX_BUFFER);
  uint16_t rx_addr = usb_pma_get_endpoint_addr(ep_idn, USB_EP_RX_BUFFER);
  uint8_t setup_packet[8] __attribute__((aligned(4)));

  usb_read_packet_data(setup_packet, rx_addr, rx_count);

  // Clear CTR RX if another setup packet arrived before this, it will be discarded
  usb_endpoint_reg_set_clear_ctr(ep_idn, USB_EP_DIRECTION_OUT_IDX);

  // Setup packet should always be 8 bytes. If not, we probably missed the packet
  if (rx_count == 8) {
    setup_received((usb_control_request_t *)setup_packet);
    // Hardware should reset EP0 RX/TX to NAK and both toggle to 1
  } else {
    usb_endpoint_set_rx_buffer_block_size(0, sizeof(usb_control_request_t));
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
    uint32_t const ep_idn = USB->ISTR & USB_ISTR_IDN;
    uint32_t const endpoint_reg = usb_endpoint_reg_get(ep_idn);

    if (endpoint_reg & USB_EP_VTRX) {
      if (endpoint_reg & USB_EP_SETUP) {
        handle_ctr_setup(ep_idn);  // CTR will be clear after copied setup packet
      } else {
        usb_endpoint_reg_set_clear_ctr(ep_idn, USB_EP_DIRECTION_OUT_IDX);
        usb_endpoint_ctr_rx(ep_idn);
      }
    }

    if (endpoint_reg & USB_EP_VTTX) {
      usb_endpoint_reg_set_clear_ctr(ep_idn, USB_EP_DIRECTION_IN_IDX);
      usb_endpoint_ctr_tx(ep_idn);
    }
  }

  if (int_status & USB_ISTR_PMAOVR) {
    USB->ISTR = ~USB_ISTR_PMAOVR;

    // TODO: overrun/underrun
  }
}

bool usb_read_packet_data(void *__restrict dst, uint16_t src, uint16_t byte_count) {
  if (byte_count == 0) {
    // Not count then nothing to read
    return true;
  }

  // We are readng 32 bit values from unaligned byte locations
  uint32_t read_count = byte_count / sizeof(uint32_t);

  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + src);
  uint8_t *dst8 = (uint8_t *)dst;

  while (read_count--) {
    unaligned_write32(dst8, (uint32_t)(*pma_buf));
    dst8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = *pma_buf;
    while (odd--) {
      *dst8++ = (uint8_t)(temp & 0xfful);
      temp >>= 8;
    }
  }

  return true;
}

static bool usb_write_packet_data(uint16_t dst, const void *__restrict src, uint16_t byte_count) {
  if (byte_count == 0) {
    // Not count then nothing to write
    return true;
  }

  // We are writing 32 bit values from unaligned byte locations
  uint32_t write_count = byte_count / sizeof(uint32_t);

  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + dst);
  const uint8_t *src8 = src;

  while (write_count--) {
    *pma_buf = unaligned_read32(src8);
    src8 += sizeof(uint32_t);
    pma_buf++;
  }

  // odd bytes e.g 1 for 16-bit or 1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t temp = 0;
    for (uint16_t i = 0; i < odd; i++) {
      temp |= *src8++ << (i * 8);
    }
    *pma_buf = temp;
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
    usb_endpoint_reg_set(i, 0, false);
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
  usb_endpoint_reset();

  // EP0 must exist
  usb_endpoint_control_init();

  // Enable USB
  USB->DADDR = USB_DADDR_EF;
}

void usb_transmit_packet(endpoint_packet_t *control_transfer, uint16_t ep_idn) {
  uint32_t len = min_u16(control_transfer->total_len - control_transfer->queued_len, control_transfer->max_packet_size);

  uint16_t addr_ptr = (uint16_t)usb_pma_get_endpoint_addr(ep_idn, USB_EP_TX_BUFFER);

  usb_write_packet_data(addr_ptr, &(control_transfer->buffer[control_transfer->queued_len]), len);
  control_transfer->queued_len += len;

  usb_pma_set_count(ep_idn, USB_EP_TX_BUFFER, len);

  uint32_t endpoint_reg = usb_endpoint_reg_get(ep_idn);
  usb_endpoint_status(&endpoint_reg, USB_EP_DIRECTION_IN_IDX, USB_EP_STATE_VALID);

  endpoint_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_EP_DIRECTION_IN_IDX);  // only change TX Status, reserve other toggle bits
  usb_endpoint_reg_set_preserve(ep_idn, endpoint_reg, true);
}
