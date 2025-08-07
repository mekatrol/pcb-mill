#ifndef TUSB_FSDEV_TYPE_H
#define TUSB_FSDEV_TYPE_H

#include "stdint.h"

#define FSDEV_BTABLE_BASE 0U

// FSDEV_PMA_SIZE is PMA buffer size in bytes.
// - 2048-byte devices, access with 32-bit address

//--------------------------------------------------------------------+
// BTable Typedef
//--------------------------------------------------------------------+
enum {
  BTABLE_BUF_TX = 0,
  BTABLE_BUF_RX = 1
};

// Buffer Table is located in Packet Memory Area (PMA)
typedef struct {
  struct {
    volatile uint32_t count_addr;
  } ep32[CFG_TUD_ENDPPOINT_MAX][2];
} fsdev_btable_t;

#define FSDEV_BTABLE ((volatile fsdev_btable_t*)(FSDEV_PMA_BASE + FSDEV_BTABLE_BASE))

typedef struct {
  volatile uint32_t value;
} fsdev_pma_buf_t;

#define PMA_BUF_AT(_addr) ((fsdev_pma_buf_t*)(FSDEV_PMA_BASE + _addr))

#define USB_EPTX_STAT 0x0030U
#define USB_EPTX_STAT_Pos 4u
#define USB_EP_DTOG_TX_Pos 6u
#define USB_EP_CTR_TX_Pos 7u

typedef enum {
  EP_STAT_DISABLED = 0,
  EP_STAT_STALL = 1,
  EP_STAT_NAK = 2,
  EP_STAT_VALID = 3
} ep_stat_t;

#define EP_STAT_MASK(_dir) (3u << (USB_EPTX_STAT_Pos + ((_dir) == TUSB_DIR_IN ? 0 : 8)))
#define EP_DTOG_MASK(_dir) (1u << (USB_EP_DTOG_TX_Pos + ((_dir) == TUSB_DIR_IN ? 0 : 8)))

//--------------------------------------------------------------------+
// Endpoint Helper
// - CTR is write 0 to clear
// - DTOG and STAT are write 1 to toggle
//--------------------------------------------------------------------+

__attribute__((always_inline)) static inline uint32_t ep_read(uint32_t ep_id) {
  return USB->chep[ep_id].CHEPnR;
}

__attribute__((always_inline)) static inline void ep_write(uint32_t ep_id, uint32_t value, bool need_exclusive) {
  if (need_exclusive) {
    dcd_int_disable(0);
  }

  USB->chep[ep_id].CHEPnR = value;

  if (need_exclusive) {
    dcd_int_enable(0);
  }
}

__attribute__((always_inline)) static inline void ep_write_clear_ctr(uint32_t ep_id, tusb_dir_t dir) {
  uint32_t reg = USB->chep[ep_id].CHEPnR;
  reg |= USB_EP_CTR_TX | USB_EP_CTR_RX;
  reg &= USB_EPREG_MASK;
  reg &= ~(1 << (USB_EP_CTR_TX_Pos + (dir == TUSB_DIR_IN ? 0 : 8)));
  ep_write(ep_id, reg, false);
}

__attribute__((always_inline)) static inline void ep_change_status(uint32_t* reg, tusb_dir_t dir, ep_stat_t state) {
  *reg ^= (state << (USB_EPTX_STAT_Pos + (dir == TUSB_DIR_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline void ep_change_dtog(uint32_t* reg, tusb_dir_t dir, uint8_t state) {
  *reg ^= (state << (USB_EP_DTOG_TX_Pos + (dir == TUSB_DIR_IN ? 0 : 8)));
}

__attribute__((always_inline)) static inline bool ep_is_iso(uint32_t reg) {
  return (reg & USB_EP_TYPE_MASK) == USB_EP_ISOCHRONOUS;
}

//--------------------------------------------------------------------+
// BTable Helper
//--------------------------------------------------------------------+

__attribute__((always_inline)) static inline uint32_t btable_get_addr(uint32_t ep_id, uint8_t buf_id) {
  return FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr & 0x0000FFFFu;
}

__attribute__((always_inline)) static inline void btable_set_addr(uint32_t ep_id, uint8_t buf_id, uint16_t addr) {
  uint32_t count_addr = FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr;
  count_addr = (count_addr & 0xFFFF0000u) | (addr & 0x0000FFFCu);
  FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline uint16_t btable_get_count(uint32_t ep_id, uint8_t buf_id) {
  uint16_t count;
  count = (FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

__attribute__((always_inline)) static inline void btable_set_count(uint32_t ep_id, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr = count_addr;
}

/* Aligned buffer size according to hardware */
__attribute__((always_inline)) static inline uint16_t pma_align_buffer_size(uint16_t size, uint8_t* blsize, uint8_t* num_block) {
  /* The STM32 full speed USB peripheral supports only a limited set of
   * buffer sizes given by the RX buffer entry format in the USB_BTABLE. */
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

__attribute__((always_inline)) static inline void btable_set_rx_bufsize(uint32_t ep_id, uint8_t buf_id, uint16_t wCount) {
  uint8_t blsize, num_block;
  (void)pma_align_buffer_size(wCount, &blsize, &num_block);

  /* Encode into register. When BLSIZE==1, we need to subtract 1 block count */
  uint16_t bl_nb = (blsize << 15) | ((num_block - blsize) << 10);
  if (bl_nb == 0) {
    // zlp but 0 is invalid value, set blsize to 1 (32 bytes)
    // Note: lower value can cause PMAOVR on setup with ch32v203
    bl_nb = 1 << 15;
  }

  uint32_t count_addr = FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr;
  count_addr = (bl_nb << 16) | (count_addr & 0x0000FFFFu);
  FSDEV_BTABLE->ep32[ep_id][buf_id].count_addr = count_addr;
}

#endif
