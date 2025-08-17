#include "board_hal.h"
#include "cdc_device.h"

// Member unassigned value
#define UNASSIGNED_VALUE 0xFFU

#define RCC_CRRCR_HSI48ON (1 << 0)
#define RCC_CRRCR_HSI48RDY (1 << 1)

#define USB_EP_STATUS_MASK(dir) (3U << (USB_CHEP_TX_STTX_Pos + ((dir) == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? 0 : 8)))
#define USB_EP_DATA_TOGGLE_MASK(dir) (1U << (USB_CHEP_DTOG_TX_Pos + ((dir) == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? 0 : 8)))

#define USB_EP_TX_BUFFER 0
#define USB_EP_RX_BUFFER 1

typedef struct {
  usb_transfer_packet_t packet;  // "Inherited" fields
  uint16_t max_packet_size;      // Endpoint max ep_transfer size
  uint8_t ep_idn;                // Endpoint identifier (is zero based so can be used as index to arrays)
} usb_ep_transfer_t;

typedef struct {
  uint8_t ep_idn;
  uint8_t ep_type;
  bool assigned[EP_IN_OUT_PAIR];
} ep_assignment_t;

// Transfer buffers for each end point, used to buffer sending and receiving data
// over endpoints. A transfer might be larger than the hardware buffers
// and having a buffered transfer reduce chances of overrun on hardware buffers.
static usb_ep_transfer_t ep_packet_buffer[USB_EP_MAX][EP_IN_OUT_PAIR];

// State of endpoint assignment
static ep_assignment_t ep_assignment[USB_EP_MAX];

// Next available USB PMA buffer pointer location
static uint16_t usb_pma_next_available;

__attribute__((always_inline)) static inline uint16_t usb_pma_get_count(uint32_t ep_idn, uint8_t buf_id) {
  uint16_t count;
  count = (USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

__attribute__((always_inline)) static inline uint32_t usb_pma_get_ep_addr(uint32_t ep_idn, uint8_t buf_id) {
  return USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr & 0x0000FFFFU;
}

__attribute__((always_inline)) static inline void usb_ep_reg_set(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  USB->chep[ep_idn].CHEPnR = value;

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void usb_ep_reg_set_preserve(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  // USB_EP_VTTX and USB_EP_VTRX are rc_w0 bits so setting them to 1 preserves the current register values
  // this will preserve  IN/OUT/SETUP transaction is successfully completed states
  USB->chep[ep_idn].CHEPnR = (value | USB_EP_VTTX | USB_EP_VTRX);

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

__attribute__((always_inline)) static inline void usb_ep_clear_correct_transfer(uint32_t ep_idn, usb_direction_index_t ep_idn_idx) {
  // Correct transfer interupt flags are:
  //  (VT == valid transation)
  //  TX -> USB_CHEP_VTTX
  //  RX -> USB_CHEP_VTRX

  // Get current register value
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;

  // Clear THREE_ERR_RX, THREE_ERR_TX, DTOGRX, STATRX, DTOGTX, STATTX
  ep_reg &= USB_CHEP_REG_MASK;  // ep_reg & 0x07FF8F8F

  // Clear USB_CHEP_VTTX or USB_CHEP_VTRX depending on ep_idn_idx
  ep_reg &= ~(1 << (ep_idn_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_VTTX_Pos : USB_CHEP_VTRX_Pos));

  // CHEPnR has many rc_w0 bits. This means we set a bit:
  //    0 = clear that bit when writing the the CHEPnR
  //    1 = no change to the bit when writing the the CHEPnR
  usb_ep_reg_set(ep_idn, ep_reg, false);
}

__attribute__((always_inline)) static inline void usb_ep_status(uint32_t *ep_reg, usb_direction_index_t dir, usb_ep_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in ep_reg
  *ep_reg ^= (state << (USB_CHEP_TX_STTX_Pos + (dir == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? 0 : 8)));
}

__attribute__((always_inline)) static inline uint32_t usb_pma_next_addr(uint32_t size) {
  // Get next available Packet Memory Area location
  uint32_t usb_pma_addr = usb_pma_next_available;

  // Update next available by adding size (size is assumed to be 32 bit aligned)
  usb_pma_next_available = (usb_pma_next_available + size);

  // Return the assigned address
  return usb_pma_addr;
}

__attribute__((always_inline)) static inline void usb_ep_data_toggle(uint32_t *ep_reg, usb_direction_index_t ep_dir_idx, usb_ep_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in ep_reg
  *ep_reg ^= (state << (USB_CHEP_DTOG_TX_Pos + (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? 0 : 8)));
}

__attribute__((always_inline)) static inline void usb_pma_set_count(uint32_t ep_idn, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[buf_id].count_addr = count_addr;
}

__attribute__((always_inline)) static inline void usb_pma_set_ep_addr(uint32_t ep_idn, uint8_t idn_dir_idx, uint16_t addr) {
  uint32_t count_addr = USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[idn_dir_idx].count_addr;
  count_addr = (count_addr & 0xFFFF0000U) | (addr & 0x0000FFFCU);
  USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[idn_dir_idx].count_addr = count_addr;
}

__attribute__((always_inline)) static inline void ep_reset_assigned_state(uint32_t ep_idn) {
  ep_assignment[ep_idn].ep_idn = UNASSIGNED_VALUE;                         // Endpoint identity unassigned
  ep_assignment[ep_idn].ep_type = UNASSIGNED_VALUE;                        // Endpoint type unassigned
  ep_assignment[ep_idn].assigned[USB_DIR_DEVICE_IN_HOST_OUT_IDX] = false;  // Out unassigned
  ep_assignment[ep_idn].assigned[USB_DIR_DEVICE_OUT_HOST_IN_IDX] = false;  // In unassigned
}

// Bit 31 BLSIZE: Block size
// This bit selects the size of memory block used to define the assigned buffer area.
//
// – If BLSIZE = 0, the memory block is 2-byte large, which is the minimum block
//   allowed in a half-word wide memory. With this block size the assigned buffer size
//   ranges from 2 to 62 bytes.
//
// – If BLSIZE = 1, the memory block is 32-byte large, which permits to reach the
//   maximum ep_transfer length defined by USB specifications. With this block size the
//   assigned buffer size theoretically ranges from 32 to 1024 bytes, which is the longest
//   ep_transfer size allowed by USB standard specifications. However, the applicable size is
//   limited by the available buffer memory
//
// Bits 30:26 NUM_BLOCK[4:0]: Number of blocks
// These bits define the number of memory blocks assigned to this ep_transfer buffer. The actual
// amount of assigned memory depends on the BLSIZE value as illustrated in RM0444 Table 239.
__attribute__((always_inline)) static inline uint32_t usb_ep_calc_rx_buffer_block_size(uint16_t buffer_size, uint32_t *blsize, uint32_t *num_block) {
  uint32_t block_size_log2;  // log2(block_size)

  if (buffer_size > 62) {
    block_size_log2 = 5;  // 32 bytes
    *blsize = 1;
  } else {
    block_size_log2 = 1;  // 2 bytes
    *blsize = 0;
  }

  // Same as:
  // block_count = (buffer_size + (32 - 1)) / 32 --> buffer_size  > 62
  // block_count = (buffer_size + ( 2 - 1)) /  2 --> buffer_size <= 62
  uint8_t block_count = (buffer_size + ((1 << block_size_log2) - 1)) >> block_size_log2;

  // if BLSIZE == 1 then we need to subtract 1 from num_block
  // See: RM0444 Table 239. Definition of assigned buffer memory
  // Easiest way is to just subtract BLSIZE from NUM_BLOCK
  *num_block = block_count - *blsize;

  // Same as:
  // block_count * 32 --> buffer_size  > 62
  // block_count *  2 --> buffer_size <= 62
  return block_count << block_size_log2;
}

static uint8_t usb_ep_assign(uint8_t ep_addr, uint8_t ep_type) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  for (uint8_t idn = 0; idn < USB_EP_MAX; idn++) {
    // Check if already assigned, and return existing identifier if so
    if (ep_assignment[idn].assigned[ep_dir_idx] &&
        ep_assignment[idn].ep_type == ep_type &&
        ep_assignment[idn].ep_idn == ep_idn) {
      return idn;
    }

    // Assign only if currently not assigned
    if (!ep_assignment[idn].assigned[ep_dir_idx]) {
      // Check if EP number is the same
      if (ep_assignment[idn].ep_idn == UNASSIGNED_VALUE || ep_assignment[idn].ep_idn == ep_idn) {
        // One EP pair has to be the same type
        if (ep_assignment[idn].ep_type == UNASSIGNED_VALUE || ep_assignment[idn].ep_type == ep_type) {
          ep_assignment[idn].ep_idn = ep_idn;
          ep_assignment[idn].ep_type = ep_type;
          ep_assignment[idn].assigned[ep_dir_idx] = true;

          return idn;
        }
      }
    }
  }

  // Assignment failed
  return UNASSIGNED_VALUE;
}

static void usb_ep_set_rx_buffer_block_size(uint32_t ep_idn, uint32_t size) {
  // Calculate BLSIZE and NUM_BLOCK from size
  uint32_t blsize, num_block;
  usb_ep_calc_rx_buffer_block_size(size, &blsize, &num_block);

  // Merge BLSIZE and NUM_BLOCK and shift to correct bit positions
  uint32_t memory_buffer_assignment = (blsize << BIT_31_POS) | (num_block << BIT_26_POS);

  // Get existing register value (we don't want to override ADDR_RX), note this clears COUNT_RX
  // which is valid because we are setting the buffer size and previous received data likely invalid
  uint32_t usb_chep_txrxbd_n = USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[USB_EP_RX_BUFFER].count_addr;

  // Merge BLSIZE, NUM_BLOCK and ADDR_RX
  usb_chep_txrxbd_n = memory_buffer_assignment | (usb_chep_txrxbd_n & 0x0000FFFFU);

  // Update register
  USB_BUFFER_DESC_TABLE->ep[ep_idn].buffer[USB_EP_RX_BUFFER].count_addr = usb_chep_txrxbd_n;
}

static void usb_ep_control_init() {
  usb_ep_assign(USB_DIR_DEVICE_IN_HOST_OUT, USB_EP_TYPE_CONTROL);
  usb_ep_assign(USB_DIR_DEVICE_OUT_HOST_IN, USB_EP_TYPE_CONTROL);

  ep_packet_buffer[EP0_IDN][USB_DIR_DEVICE_IN_HOST_OUT_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  ep_packet_buffer[EP0_IDN][USB_DIR_DEVICE_IN_HOST_OUT_IDX].ep_idn = EP0_IDN;

  ep_packet_buffer[EP0_IDN][USB_DIR_DEVICE_OUT_HOST_IN_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  ep_packet_buffer[EP0_IDN][USB_DIR_DEVICE_OUT_HOST_IN_IDX].ep_idn = EP0_IDN;

  uint16_t pma_rx_addr = usb_pma_next_addr(USB_EP0_BUFFER_SIZE);
  uint16_t pma_tx_addr = usb_pma_next_addr(USB_EP0_BUFFER_SIZE);

  usb_pma_set_ep_addr(EP0_IDN, USB_EP_RX_BUFFER, pma_rx_addr);
  usb_pma_set_ep_addr(EP0_IDN, USB_EP_TX_BUFFER, pma_tx_addr);

  uint32_t ep_reg = USB->chep[EP0_IDN].CHEPnR & ~USB_CHEP_REG_MASK;
  ep_reg |= USB_EP_CONTROL;
  usb_ep_status(&ep_reg, USB_DIR_DEVICE_OUT_HOST_IN_IDX, USB_EP_STATE_NAK);
  usb_ep_status(&ep_reg, USB_DIR_DEVICE_IN_HOST_OUT_IDX, USB_EP_STATE_NAK);

  usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
  usb_ep_reg_set(EP0_IDN, ep_reg, false);
}

/*
 * This method writes data from an unaligned buffer to the endpoint buffer
 */
__attribute__((always_inline)) static inline bool usb_write_unaligned_data(uint16_t dst, const void *__restrict src, uint16_t byte_count) {
  if (byte_count == 0) {
    // No count then nothing to write
    return true;
  }

  // We are writing 32 bit values from unaligned byte locations
  uint32_t write_count = byte_count / sizeof(uint32_t);

  // The PMA buffer we are writing to
  volatile uint32_t *pma_buf = (volatile uint32_t *)(USB_DRD_PMAADDR + dst);

  // The unaligned buffer we area reading from
  const uint8_t *src8 = src;

  // Read unaligned byte and write to PMA buffer
  while (write_count--) {
    *pma_buf = unaligned_read_32(src8);
    src8 += sizeof(uint32_t);
    pma_buf++;
  }

  // Write an remaining bytes (for odd byte_count)
  // ie:
  //    1   for 16-bit
  //    1-3 for 32-bit
  uint16_t odd = byte_count & (sizeof(uint32_t) - 1);
  if (odd) {
    uint32_t b = 0;
    for (uint16_t i = 0; i < odd; i++) {
      b |= *src8++ << (i * 8);
    }
    *pma_buf = b;
  }

  return true;
}

static void usb_tx_packet(usb_ep_transfer_t *ep_transfer) {
  uint32_t len = transfer_remaining_length(ep_transfer->packet.total_length, ep_transfer->packet.transferred_length, ep_transfer->max_packet_size);

  uint16_t addr_ptr = (uint16_t)usb_pma_get_ep_addr(ep_transfer->ep_idn, USB_EP_TX_BUFFER);

  usb_write_unaligned_data(addr_ptr, &(ep_transfer->packet.buffer[ep_transfer->packet.transferred_length]), len);
  ep_transfer->packet.transferred_length += len;

  usb_pma_set_count(ep_transfer->ep_idn, USB_EP_TX_BUFFER, len);

  uint32_t ep_reg = USB->chep[ep_transfer->ep_idn].CHEPnR;
  usb_ep_status(&ep_reg, USB_DIR_DEVICE_OUT_HOST_IN_IDX, USB_EP_STATE_VALID);

  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_DIR_DEVICE_OUT_HOST_IN_IDX);  // only change TX Status, reserve other toggle bits
  usb_ep_reg_set_preserve(ep_transfer->ep_idn, ep_reg, true);
}

static void usb_ep_transfer_complete(uint8_t ep_addr, uint32_t transferred_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);

  usb_device.ep_status[ep_idn][ep_dir_idx].busy = 0;
  usb_device.ep_status[ep_idn][ep_dir_idx].claimed = 0;

  if (ep_idn == 0) {
    usb_control_transfer(ep_addr, transferred_bytes);
  } else {
    usb_cdc_transfer(ep_addr, transferred_bytes);
  }
}

static bool usb_rx_packet(void *__restrict dst, uint16_t src, uint16_t byte_count) {
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

static void usb_ep_rx(uint32_t ep_idn) {
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
  usb_ep_transfer_t *packet = &ep_packet_buffer[ep_idn][USB_DIR_DEVICE_IN_HOST_OUT_IDX];
  const uint16_t rx_count = usb_pma_get_count(ep_idn, USB_EP_RX_BUFFER);
  uint16_t pma_addr = (uint16_t)usb_pma_get_ep_addr(ep_idn, USB_EP_RX_BUFFER);

  usb_rx_packet(packet->packet.buffer + packet->packet.transferred_length, pma_addr, rx_count);
  packet->packet.transferred_length += rx_count;

  if ((rx_count < packet->max_packet_size) || (packet->packet.transferred_length >= packet->packet.total_length)) {
    // All bytes now received

    usb_ep_set_rx_buffer_block_size(ep_idn, (uint32_t)packet->max_packet_size);

    usb_ep_transfer_complete(ep_idn, packet->packet.transferred_length);

    packet->packet.total_length = packet->packet.transferred_length = 0;
  } else {
    ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_DIR_DEVICE_IN_HOST_OUT_IDX);  // will change RX Status, reserved other toggle bits
    usb_ep_status(&ep_reg, USB_DIR_DEVICE_IN_HOST_OUT_IDX, USB_EP_STATE_VALID);
    usb_ep_reg_set_preserve(ep_idn, ep_reg, false);
  }
}

static void usb_ep_tx_queued_bytes(uint32_t ep_idn) {
  usb_ep_transfer_t *ep_transfer = &ep_packet_buffer[ep_idn][USB_DIR_DEVICE_OUT_HOST_IN_IDX];

  if (ep_transfer->packet.total_length != ep_transfer->packet.transferred_length) {
    usb_tx_packet(ep_transfer);
  } else {
    uint32_t ep_addr = USB->chep[ep_idn].CHEPnR & USB_CHEP_ADDR;
    usb_ep_transfer_complete(ep_addr | USB_DIR_DEVICE_OUT_HOST_IN, ep_transfer->packet.transferred_length);
  }
}

static void usb_ep_reset() {
  for (uint32_t idn = 0; idn < USB_EP_MAX; idn++) {
    ep_reset_assigned_state(idn);
  }

  // Reset PMA assignment (to end of EP buffer descriptor table)
  usb_pma_next_available = 8 * USB_EP_MAX;
}

static void setup_received(usb_control_request_t *setup_received) {
  // Setup recieved, therefore host has connected to device
  usb_device.connected = 1;

  // Reset state
  usb_device.ep_status[EP0_IDN][USB_DIR_DEVICE_IN_HOST_OUT_IDX].busy = 0;
  usb_device.ep_status[EP0_IDN][USB_DIR_DEVICE_IN_HOST_OUT_IDX].claimed = 0;
  usb_device.ep_status[EP0_IDN][USB_DIR_DEVICE_OUT_HOST_IN_IDX].busy = 0;
  usb_device.ep_status[EP0_IDN][USB_DIR_DEVICE_OUT_HOST_IN_IDX].claimed = 0;

  // Process control request
  if (!process_control_request(setup_received)) {
    // USB 2.0 Specification, Section 9.2.7, “Error Handling”
    // If a device detects a condition that prevents it from completing the request, it must indicate the error by returning a STALL handshake.
    // For control transfers, the device must respond with a STALL to any setup or data stage packet it cannot handle.
    usb_ep_stall_set(EP0_IDN | USB_DIR_DEVICE_OUT_HOST_IN);
    usb_ep_stall_set(EP0_IDN | USB_DIR_DEVICE_IN_HOST_OUT);
  }
}

static void usb_ep_setup(uint32_t ep_idn) {
  uint16_t rx_count = usb_pma_get_count(ep_idn, USB_EP_RX_BUFFER);
  uint16_t rx_addr = usb_pma_get_ep_addr(ep_idn, USB_EP_RX_BUFFER);

  __attribute__((aligned(4)))
  uint8_t setup_packet[8];

  usb_rx_packet(setup_packet, rx_addr, rx_count);

  // Clear correct transfer flag
  usb_ep_clear_correct_transfer(ep_idn, USB_DIR_DEVICE_IN_HOST_OUT_IDX);

  if (rx_count == sizeof(usb_control_request_t)) {
    setup_received((usb_control_request_t *)setup_packet);
  } else {
    // Something went wrong, reset the endpoint state (by resetting size, which clears count etc)
    usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
  }
}

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

void usb_init_hal() {
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

void usb_sof_set_enable(bool enable) {
  if (enable) {
    USB->CNTR |= USB_CNTR_SOFM;
  } else {
    USB->CNTR &= ~USB_CNTR_SOFM;
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
        usb_ep_clear_correct_transfer(ep_idn, USB_DIR_DEVICE_IN_HOST_OUT_IDX);

        // Receive data
        usb_ep_rx(ep_idn);
      }
    }

    if (ep_reg & USB_EP_VTTX) {
      // Clear USB_EP_VTTX
      usb_ep_clear_correct_transfer(ep_idn, USB_DIR_DEVICE_OUT_HOST_IN_IDX);

      // Transmit next batch of queued data (or complete transfer if none remianing in queue)
      usb_ep_tx_queued_bytes(ep_idn);
    }
  }

  if (int_status & USB_ISTR_PMAOVR) {
    USB->ISTR = ~USB_ISTR_PMAOVR;

    // TODO: overrun/underrun
  }
}

bool usb_ep_open(const usb_ep_descriptor_t *ep_descriptor) {
  const uint8_t ep_addr = ep_descriptor->bEndpointAddress;
  const uint8_t ep_dir_idx = USB_EP_DIR_IDX(ep_addr);
  const uint32_t packet_size = USB_EP_PACKET_SIZE(ep_descriptor->wMaxPacketSize);
  const uint8_t ep_idn = usb_ep_assign(ep_addr, ep_descriptor->bmAttributes.type);

  // Fail if unassigned
  if (ep_idn == UNASSIGNED_VALUE) {
    return false;
  }

  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR & ~USB_CHEP_REG_MASK;
  ep_reg |= USB_EP_IDN(ep_addr);

  // Supported endpoint types
  switch (ep_descriptor->bmAttributes.type) {
    case USB_EP_TYPE_BULK:
      ep_reg |= USB_EP_BULK;
      break;

    case USB_EP_TYPE_INTERRUPT:
      ep_reg |= USB_EP_INTERRUPT;
      break;

    default:
      // End type is not supported
      return false;
  }

  /* Create a packet memory buffer area. */
  uint16_t pma_addr = usb_pma_next_addr(packet_size);
  usb_pma_set_ep_addr(ep_idn, ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_EP_TX_BUFFER : USB_EP_RX_BUFFER, pma_addr);

  usb_ep_transfer_t *packet = &ep_packet_buffer[ep_idn][ep_dir_idx];
  packet->max_packet_size = packet_size;
  packet->ep_idn = ep_idn;

  usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_NAK);
  usb_ep_data_toggle(&ep_reg, ep_dir_idx, 0);

  // reserve other direction toggle bits
  if (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX) {
    ep_reg &= ~(USB_CH_RX_VALID | USB_EP_DTOG_RX);
  } else {
    ep_reg &= ~(USB_CHEP_TX_STTX_Msk | USB_EP_DTOG_TX);
  }

  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);

  return true;
}

void usb_ep_close_all() {
  NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

  for (uint32_t i = 1; i < USB_EP_MAX; i++) {
    usb_ep_reg_set(i, 0, false);
    ep_reset_assigned_state(i);
  }

  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

  // Reset PMA assignment
  usb_pma_next_available = 8 * USB_EP_MAX + 2 * USB_EP0_BUFFER_SIZE;
}

void usb_ep_control_status_complete(const usb_control_request_t *request) {
  const usb_request_type_t request_type = usb_request_type(request->bmRequestType);
  const usb_request_recipient_t request_recipient = usb_request_recipient(request->bmRequestType);

  if (request_recipient == USB_REQUEST_RECIPIENT_DEVICE &&
      request_type == USB_REQUEST_TYPE_STANDARD &&
      request->bRequest == USB_STD_SET_ADDRESS) {
    const uint8_t dev_addr = (uint8_t)request->wValue;
    USB->DADDR = (USB_DADDR_EF | dev_addr);
  }

  usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
}

void usb_ep_stall_set_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);
  usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_STALL);

  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
}

void usb_ep_stall_clear_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  // Get current value of CHEPnR
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;

  // Clear state
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx) | USB_EP_DATA_TOGGLE_MASK(ep_dir_idx);

  // Reset to DATA0
  usb_ep_data_toggle(&ep_reg, ep_dir_idx, 0);

  // Set value of CHEPnR
  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
}

/*
 * Prepare HAL for sending / receiving data from host
 */
bool usb_ep_transfer_queue_hal(uint8_t ep_idn, uint8_t ep_dir_idx, uint8_t *buffer, uint16_t total_bytes) {
  usb_ep_transfer_t *packet = &ep_packet_buffer[ep_idn][ep_dir_idx];

  // Initialise packet
  packet->packet.buffer = buffer;             // Use callers buffer
  packet->packet.total_length = total_bytes;  // We are going to transfer total bytes
  packet->packet.transferred_length = 0;      // Nothing has been transferred yet

  if (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX) {
    // Transmit from device is USB_DIR_DEVICE_OUT_HOST_IN_IDX to host
    usb_tx_packet(packet);
  } else {
    // Receive to device is USB_DIR_DEVICE_IN_HOST_OUT_IDX from host
    uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
    ep_reg &= (USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx));

    usb_ep_set_rx_buffer_block_size(ep_idn, (uint32_t)packet->packet.total_length);
    usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_VALID);
    usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
  }

  // STM32G0B1 does not detect failures in this method so always return true (assume success)
  return true;
}