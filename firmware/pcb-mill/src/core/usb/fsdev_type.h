#ifndef TUSB_FSDEV_TYPE_H
#define TUSB_FSDEV_TYPE_H

#include "usb.h"

#include "tusb_types.h"

typedef struct {
  volatile uint32_t value;
} usb_pma_buf_t;

#define USB_PMA_BUF_AT(addr) ((usb_pma_buf_t*)(USB_DRD_PMAADDR + addr))

typedef enum {
  EP_STAT_DISABLED = 0,
  EP_STAT_STALL = 1,
  EP_STAT_NAK = 2,
  EP_STAT_VALID = 3
} ep_stat_t;

#define EP_STAT_MASK(dir) (3u << (USB_CHEP_TX_STTX_Pos + ((dir) == TUSB_DIR_IN ? 0 : 8)))
#define EP_DTOG_MASK(dir) (1u << (USB_CHEP_DTOG_TX_Pos + ((dir) == TUSB_DIR_IN ? 0 : 8)))

__attribute__((always_inline)) static inline uint32_t ep_read(uint32_t ep_id) {
  return USB->chep[ep_id].CHEPnR;
}

__attribute__((always_inline)) static inline void ep_write(uint32_t ep_id, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  USB->chep[ep_id].CHEPnR = value;

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void ep_write_clear_ctr(uint32_t ep_id, tusb_dir_t dir) {
  uint32_t reg = USB->chep[ep_id].CHEPnR;
  reg |= USB_EP_VTTX | USB_EP_VTRX;
  reg &= USB_CHEP_REG_MASK;
  reg &= ~(1 << (USB_CHEP_VTTX_Pos + (dir == TUSB_DIR_IN ? 0 : 8)));
  ep_write(ep_id, reg, false);
}

__attribute__((always_inline)) static inline void ep_change_status(uint32_t* reg, tusb_dir_t dir, ep_stat_t state) {
  *reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == TUSB_DIR_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline void ep_change_dtog(uint32_t* reg, tusb_dir_t dir, uint8_t state) {
  *reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (dir == TUSB_DIR_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline bool ep_is_iso(uint32_t reg) {
  return (reg & USB_EP_TYPE_MASK) == USB_EP_ISOCHRONOUS;
}

__attribute__((always_inline)) static inline uint32_t usb_pma_get_addr(uint32_t ep_id, uint8_t buf_id) {
  return USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr & 0x0000FFFFu;
}

__attribute__((always_inline)) static inline void usb_pma_set_addr(uint32_t ep_id, uint8_t buf_id, uint16_t addr) {
  uint32_t count_addr = USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr;
  count_addr = (count_addr & 0xFFFF0000u) | (addr & 0x0000FFFCu);
  USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline uint16_t usb_pma_get_count(uint32_t ep_id, uint8_t buf_id) {
  uint16_t count;
  count = (USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

__attribute__((always_inline)) static inline void usb_pma_set_count(uint32_t ep_id, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr = count_addr;
}

/* Aligned buffer size according to hardware */
__attribute__((always_inline)) static inline uint16_t pma_align_buffer_size(uint16_t size, uint8_t* blsize, uint8_t* num_block) {
  /* The STM32 full speed USB peripheral supports only a limited set of
   * buffer sizes given by the RX buffer entry format in the USBRAM_REGSITER. */
  uint16_t block_in_bytes;
  if (size > 62) {
    block_in_bytes = 32;
    *blsize = 1;
    *num_block = DIV_CEIL(size, 32);
  } else {
    block_in_bytes = 2;
    *blsize = 0;
    *num_block = DIV_CEIL(size, 2);
  }

  return (*num_block) * block_in_bytes;
}

__attribute__((always_inline)) static inline void usb_pma_set_rx_bufsize(uint32_t ep_id, uint8_t buf_id, uint16_t wCount) {
  uint8_t blsize, num_block;
  (void)pma_align_buffer_size(wCount, &blsize, &num_block);

  /* Encode into register. When BLSIZE==1, we need to subtract 1 block count */
  uint16_t bl_nb = (blsize << 15) | ((num_block - blsize) << 10);
  if (bl_nb == 0) {
    // zlp but 0 is invalid value, set blsize to 1 (32 bytes)
    // Note: lower value can cause PMAOVR on setup with ch32v203
    bl_nb = 1 << 15;
  }

  uint32_t count_addr = USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr;
  count_addr = (bl_nb << 16) | (count_addr & 0x0000FFFFu);
  USBRAM_REGSITER->endpoint[ep_id].buffer[buf_id].count_addr = count_addr;
}

#endif
