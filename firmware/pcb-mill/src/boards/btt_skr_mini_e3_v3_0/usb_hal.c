#include "board_hal.h"
#include "feed_forward_buffer.h"
#include "aligned_memory.h"
#include "usb.h"

#include "stm32g0xx.h"

// Member unassigned value
#define UNASSIGNED_VALUE 0xFFU

// STM32G0B1 has 8 endpoints
#define USB_EP_MAX 8

// Endpoint TX and RX count_addr indexes
#define USB_EP_TX_COUNT_ADDR_IDX 0
#define USB_EP_RX_COUNT_ADDR_IDX 1

#define USB_EP_STATUS_MASK(ep_dir_idx) (3U << ((ep_dir_idx) == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_TX_STTX_Pos : USB_CHEP_RX_STRX_Pos))
#define USB_EP_DATA_TOGGLE_MASK(ep_dir_idx) (1U << ((ep_dir_idx) == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_DTOG_TX_Pos : USB_CHEP_DTOG_RX_Pos))

// See Table 237. Transmission status encoding in RM0444
typedef enum {
  USB_EP_STATE_DISABLED = 0b00,
  USB_EP_STATE_STALL = 0b01,
  USB_EP_STATE_NAK = 0b10,
  USB_EP_STATE_VALID = 0b11
} usb_ep_state_t;

// This is the same as USB_DRD_PMABuffDescTypeDef, except we can reference the
// TXBD and RXBD using a direction index for the count_addr:
//     buffer[0].count_addr => TXBD
//     buffer[1].count_addr => RXBD
typedef struct
{
  union {
    struct {
      volatile uint32_t count_addr;
    } buffer[EP_IN_OUT_PAIR];

    volatile uint32_t TXBD;
    volatile uint32_t RXBD;
  };
} usb_buffer_description_table_t;

_Static_assert(sizeof(usb_buffer_description_table_t) == 8, "sizeof(usb_buffer_description_table_t) must be 8");

// Buffer Tables are located in Packet Memory Area (PMA)
typedef struct {
  usb_buffer_description_table_t ep[USB_EP_MAX];
} usb_buffer_description_tables_t;

_Static_assert(sizeof(usb_buffer_description_tables_t) == 64, "sizeof(usb_control_request_t) must be 64");

typedef struct {
  feed_forward_buffer_t feed;  // "Inherited" fields
  uint16_t max_packet_size;    // Endpoint max ep_transfer size
  uint8_t ep_idn;              // Endpoint identifier (is zero based so can be used as index to arrays)
} usb_ep_transfer_t;

typedef struct {
  uint8_t ep_idn;
  uint8_t ep_type;
  bool assigned[EP_IN_OUT_PAIR];
} ep_assignment_t;

// Transfer TX/RX buffer for each endpoint, used to buffer sending and receiving data
// over endpoints. A transfer might be larger than the hardware buffers
// and having a buffered transfer reduce chances of overrun on hardware buffers.
static usb_ep_transfer_t ep_transfer_set[USB_EP_MAX][EP_IN_OUT_PAIR];

// State of endpoint assignment
static ep_assignment_t ep_assignment[USB_EP_MAX];

// Buffer descriptor tables. Located at USB_DRD_PMAADDR.
#define USB_BUFF_DESC ((volatile usb_buffer_description_tables_t *)(USB_DRD_PMAADDR))

// Next available USB PMA buffer pointer location
static uint16_t usb_pma_next_available;

// The countdown is there to ensure the remote wakeup signal is held for the correct duration (1–15 ms),
// measured in 1 ms ESOF ticks, and then released at the right time by clearing USB_CNTR_RESUME.
static volatile uint8_t remote_wake_counter;

// This counter is reset in SOF interrupt, and incremented by systick or similar (by calling usb_systick_hal)
static volatile uint8_t usb_offline_counter;

/****************************************************************************************************************************************
 * HAL internal inline methods
 ****************************************************************************************************************************************/

ALWAYS_INLINE static void
ep_reset_assigned_state(uint32_t ep_idn) {
  ep_assignment[ep_idn].ep_idn = UNASSIGNED_VALUE;                         // Endpoint identity unassigned
  ep_assignment[ep_idn].ep_type = UNASSIGNED_VALUE;                        // Endpoint type unassigned
  ep_assignment[ep_idn].assigned[USB_DIR_DEVICE_IN_HOST_OUT_IDX] = false;  // Out unassigned
  ep_assignment[ep_idn].assigned[USB_DIR_DEVICE_OUT_HOST_IN_IDX] = false;  // In unassigned
}

ALWAYS_INLINE static void usb_ep_reset() {
  for (uint32_t idn = 0; idn < USB_EP_MAX; idn++) {
    ep_reset_assigned_state(idn);
  }

  // Reset PMA assignment (to end of EP buffer descriptor table)
  usb_pma_next_available = 8 * USB_EP_MAX;
}

/*
 * Set endpoint register with option of disabling interrupts while setting the value.
 */
ALWAYS_INLINE static void usb_ep_reg_set(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
  if (disable_usb_irq) {
    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }

  USB->chep[ep_idn].CHEPnR = value;

  if (disable_usb_irq) {
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

ALWAYS_INLINE static void usb_ep_reg_set_preserve(uint32_t ep_idn, uint32_t value, bool disable_usb_irq) {
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

/*
 * Get endpoint status value.
 */
ALWAYS_INLINE static uint32_t usb_ep_status_get(uint32_t ep_reg, usb_request_direction_index_t ep_dir_idx) {
  // Calculate status bits
  uint32_t pos = (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_TX_STTX_Pos : USB_CHEP_RX_STRX_Pos);

  // Mask status bits
  uint32_t status = ep_reg & (3U << pos);

  // Shift status to zero bit
  return status >> pos;
}

/*
 * Set endpoint status value.
 */
ALWAYS_INLINE static void usb_ep_status(uint32_t *ep_reg, usb_request_direction_index_t ep_dir_idx, usb_ep_state_t status_flags) {
  // Any bits set to 1 in status_flags will be toggle the same bit in ep_reg
  *ep_reg ^= (status_flags << (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_TX_STTX_Pos : USB_CHEP_RX_STRX_Pos));
}

/*
 * Toggle endpoint register value.
 */
ALWAYS_INLINE static void usb_ep_data_toggle(uint32_t *ep_reg, usb_request_direction_index_t ep_dir_idx, usb_ep_state_t state) {
  // Any bits set to 1 in state will be toggle the same bit in ep_reg
  *ep_reg ^= (state << (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_DTOG_TX_Pos : USB_CHEP_DTOG_RX_Pos));
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
ALWAYS_INLINE static uint32_t usb_ep_calc_rx_buffer_block_size(uint16_t buffer_size, uint32_t *blsize, uint32_t *num_block) {
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

ALWAYS_INLINE static void usb_ep_clear_correct_transfer(uint32_t ep_idn, usb_request_direction_index_t ep_dir_idx) {
  // Correct transfer interupt flags are:
  //  (VT == valid transation)
  //  TX -> USB_CHEP_VTTX
  //  RX -> USB_CHEP_VTRX

  // Get current register value
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;

  // Clear THREE_ERR_RX, THREE_ERR_TX, DTOGRX, STATRX, DTOGTX, STATTX
  ep_reg &= USB_CHEP_REG_MASK;  // ep_reg & 0x07FF8F8F

  // Clear USB_CHEP_VTTX or USB_CHEP_VTRX depending on ep_dir_idx
  ep_reg &= ~(1 << (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_CHEP_VTTX_Pos : USB_CHEP_VTRX_Pos));

  // CHEPnR has many rc_w0 bits. This means we set a bit:
  //    0 = clear that bit when writing the the CHEPnR
  //    1 = no change to the bit when writing the the CHEPnR
  usb_ep_reg_set(ep_idn, ep_reg, false);
}

ALWAYS_INLINE static uint16_t usb_pma_count_get(uint32_t ep_idn, uint8_t buf_id) {
  uint16_t count;
  count = (USB_BUFF_DESC->ep[ep_idn].buffer[buf_id].count_addr >> 16);
  return count & 0x3FFU;
}

ALWAYS_INLINE static void usb_pma_count_set(uint32_t ep_idn, uint8_t buf_id, uint16_t byte_count) {
  uint32_t count_addr = USB_BUFF_DESC->ep[ep_idn].buffer[buf_id].count_addr;
  count_addr = (count_addr & ~0x03FF0000u) | ((byte_count & 0x3FFu) << 16);
  USB_BUFF_DESC->ep[ep_idn].buffer[buf_id].count_addr = count_addr;
}

ALWAYS_INLINE static uint32_t usb_pma_next_addr(uint32_t size) {
  // Get next available Packet Memory Area location
  uint32_t usb_pma_addr = usb_pma_next_available;

  // Update next available by adding size (size is assumed to be 32 bit aligned)
  usb_pma_next_available = (usb_pma_next_available + size);

  // Return the assigned address
  return usb_pma_addr;
}

/*
 * Get the PMA for an endpoint
 */
ALWAYS_INLINE static uint32_t usb_pma_ep_addr_get(uint32_t ep_idn, uint8_t buf_id) {
  return USB_BUFF_DESC->ep[ep_idn].buffer[buf_id].count_addr & 0x0000FFFFU;
}

/*
 * Set the PMA for an endpoint
 */
ALWAYS_INLINE static void usb_pma_ep_addr_set(
    uint32_t ep_idn,      // The ep identifier (0 = EP0 ... 7 = EP7)
    uint8_t idn_dir_idx,  // The ep direction
    uint16_t addr) {      // The allocated memory address
  uint32_t count_addr = USB_BUFF_DESC->ep[ep_idn].buffer[idn_dir_idx].count_addr;
  count_addr = (count_addr & 0xFFFF0000U) | (addr & 0x0000FFFCU);
  USB_BUFF_DESC->ep[ep_idn].buffer[idn_dir_idx].count_addr = count_addr;
}

/****************************************************************************************************************************************
 * HAL internal methods
 ****************************************************************************************************************************************/

/*
 * Set the enpoints block size (BLSIZE and NUM_BLOCK) from buffer_size
 */
static void usb_ep_set_rx_buffer_block_size(uint32_t ep_idn, uint32_t buffer_size) {
  // Calculate BLSIZE and NUM_BLOCK from size
  uint32_t blsize, num_block;
  usb_ep_calc_rx_buffer_block_size(buffer_size, &blsize, &num_block);

  // Merge BLSIZE and NUM_BLOCK and shift to correct bit positions
  uint32_t memory_buffer_assignment = (blsize << BIT_31_POS) | (num_block << BIT_26_POS);

  // Get existing register value (we don't want to override ADDR_RX), note this clears COUNT_RX
  // which is valid because we are setting the buffer size and previous received data likely invalid
  uint32_t usb_chep_txrxbd_n = USB_BUFF_DESC->ep[ep_idn].buffer[USB_EP_RX_COUNT_ADDR_IDX].count_addr;

  // Merge BLSIZE, NUM_BLOCK and ADDR_RX
  usb_chep_txrxbd_n = memory_buffer_assignment | (usb_chep_txrxbd_n & 0x0000FFFFU);

  // Update register
  USB_BUFF_DESC->ep[ep_idn].buffer[USB_EP_RX_COUNT_ADDR_IDX].count_addr = usb_chep_txrxbd_n;
}

/*
 * Set next available endpoint assignment for the specified ep IDn and type
 */
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

/*
 * Initialise the control endpoint (must be EP0)
 */
static void usb_ep_control_init() {
  usb_ep_assign(USB_DIR_DEVICE_IN_HOST_OUT, USB_EP_TYPE_CONTROL);
  usb_ep_assign(USB_DIR_DEVICE_OUT_HOST_IN, USB_EP_TYPE_CONTROL);

  ep_transfer_set[EP0_IDN][USB_DIR_DEVICE_IN_HOST_OUT_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  ep_transfer_set[EP0_IDN][USB_DIR_DEVICE_IN_HOST_OUT_IDX].ep_idn = EP0_IDN;

  ep_transfer_set[EP0_IDN][USB_DIR_DEVICE_OUT_HOST_IN_IDX].max_packet_size = USB_EP0_BUFFER_SIZE;
  ep_transfer_set[EP0_IDN][USB_DIR_DEVICE_OUT_HOST_IN_IDX].ep_idn = EP0_IDN;

  uint16_t pma_tx_addr = usb_pma_next_addr(USB_EP0_BUFFER_SIZE);
  uint16_t pma_rx_addr = usb_pma_next_addr(USB_EP0_BUFFER_SIZE);

  // Set EP0 TX/RX buffer addresses
  usb_pma_ep_addr_set(EP0_IDN, USB_EP_TX_COUNT_ADDR_IDX, pma_tx_addr);
  usb_pma_ep_addr_set(EP0_IDN, USB_EP_RX_COUNT_ADDR_IDX, pma_rx_addr);

  // Get CHEP0R value
  uint32_t ep_reg = USB->chep[EP0_IDN].CHEPnR & ~USB_CHEP_REG_MASK;

  // EP0 is the control end point
  ep_reg |= USB_EP_CONTROL;

  usb_ep_status(&ep_reg, USB_DIR_DEVICE_OUT_HOST_IN_IDX, USB_EP_STATE_NAK);
  usb_ep_status(&ep_reg, USB_DIR_DEVICE_IN_HOST_OUT_IDX, USB_EP_STATE_NAK);

  usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));

  // Update the EP0 register with configured value
  usb_ep_reg_set(EP0_IDN, ep_reg, false);
}

/*
 *
 */
static bool usb_rx_pma_read(void *__restrict dst, uint16_t src, uint16_t byte_count) {
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

/*
 * Gets device to a stalled state
 */
bool usb_ep_stall_get_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);

  // Get status bits
  uint32_t status = usb_ep_status_get(ep_reg, ep_dir_idx);

  // Return true if stalled
  return status == USB_EP_STATE_STALL;
}

/*
 * Sets device to a stalled state
 */
void usb_ep_stall_set_hal(uint8_t ep_idn, uint8_t ep_dir_idx) {
  uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx);
  usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_STALL);
  usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
}

/*
 * Clears device to a stalled state
 */
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
 * Process a control request and stall if failed.
 */
static void process_control_request_hal(usb_control_request_t *control_request) {
  // Process control request
  if (!usb_process_control_request(control_request)) {
    // USB 2.0 Specification, Section 9.2.7, "Error Handling"
    // If a device detects a condition that prevents it from completing the request, it must indicate the error by returning a STALL handshake.
    // For control transfers, the device must respond with a STALL to any setup or data stage packet it cannot handle.
    usb_ep_stall_set_hal(EP0_IDN, USB_DIR_DEVICE_OUT_HOST_IN);
    usb_ep_stall_set_hal(EP0_IDN, USB_DIR_DEVICE_IN_HOST_OUT);
  }
}

/*
 * Set up an endpoint
 */
static void usb_ep_setup(uint32_t ep_idn) {
  uint8_t control_request[8];

  // Get received buffer PMA location
  uint16_t rx_addr = usb_pma_ep_addr_get(ep_idn, USB_EP_RX_COUNT_ADDR_IDX);

  // Get number of bytes received
  uint16_t rx_count = usb_pma_count_get(ep_idn, USB_EP_RX_COUNT_ADDR_IDX);

  // Receive the request data
  usb_rx_pma_read(control_request, rx_addr, rx_count);

  // Clear correct transfer flag
  usb_ep_clear_correct_transfer(ep_idn, USB_DIR_DEVICE_IN_HOST_OUT_IDX);

  // is the received data the correct length?
  if (rx_count == sizeof(usb_control_request_t)) {
    // Process setup request
    process_control_request_hal((usb_control_request_t *)control_request);
  } else {
    // Something went wrong, reset the control endpoint (by resetting size, which clears count etc)
    usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
  }
}

/*
 * This method writes data from an unaligned buffer to the endpoint buffer
 */
ALWAYS_INLINE static bool usb_write_unaligned_data(uint16_t dst, const void *__restrict src, uint16_t byte_count) {
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

static void usb_ep_transfer_complete(uint8_t ep_addr, uint32_t transferred_bytes) {
  const uint8_t ep_idn = USB_EP_IDN(ep_addr);

  if (ep_idn == 0) {
    // EP0 (control endpoint) uses a control transfer
    usb_control_transfer(ep_addr, transferred_bytes);
  } else {
    // Transfer data between EP and function buffers
    usb_ep_buffer_transfer(ep_addr, transferred_bytes);
  }
}

static void usb_tx_packet(usb_ep_transfer_t *ep_transfer) {
  // Calculate the remaining length of data to write
  uint32_t len = feed_forward_remaining_count(&ep_transfer->feed, ep_transfer->max_packet_size);

  // Get pointer to the endpoing PMA
  uint16_t addr_ptr = (uint16_t)usb_pma_ep_addr_get(ep_transfer->ep_idn, USB_EP_TX_COUNT_ADDR_IDX);

  // Transfer next batch of data from the feed buffer to the PMA
  usb_write_unaligned_data(addr_ptr, &(ep_transfer->feed.buffer[ep_transfer->feed.fed_count]), len);

  // Update the transferred length
  ep_transfer->feed.fed_count += len;

  // Set the length of data in PMA
  usb_pma_count_set(ep_transfer->ep_idn, USB_EP_TX_COUNT_ADDR_IDX, len);

  // Indicate endpoint is in a valid state
  uint32_t ep_reg = USB->chep[ep_transfer->ep_idn].CHEPnR;

  usb_ep_status(&ep_reg, USB_DIR_DEVICE_OUT_HOST_IN_IDX, USB_EP_STATE_VALID);

  // Update TX Status
  ep_reg &= USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(USB_DIR_DEVICE_OUT_HOST_IN_IDX);
  usb_ep_reg_set_preserve(ep_transfer->ep_idn, ep_reg, true);
}

static void usb_rx_packet(uint32_t ep_idn) {
  // Get the transfer buffer
  usb_ep_transfer_t *ep_transfer = &ep_transfer_set[ep_idn][USB_DIR_DEVICE_IN_HOST_OUT_IDX];

  // Get the number of bytes in PMA
  const uint16_t rx_count = usb_pma_count_get(ep_idn, USB_EP_RX_COUNT_ADDR_IDX);

  // Get PMA address
  uint16_t pma_addr = (uint16_t)usb_pma_ep_addr_get(ep_idn, USB_EP_RX_COUNT_ADDR_IDX);

  // Append data PMA to the transfer feed
  usb_rx_pma_read(((void *)ep_transfer->feed.buffer) + ep_transfer->feed.fed_count, pma_addr, rx_count);

  // Increase the amount fed to the buffer
  ep_transfer->feed.fed_count += rx_count;

  // Reset PMA RX block to max packet size (will reset count)
  usb_ep_set_rx_buffer_block_size(ep_idn, (uint32_t)ep_transfer->max_packet_size);

  // Callback to receive complete
  usb_ep_transfer_complete(ep_idn | USB_DIR_DEVICE_IN_HOST_OUT_IDX, ep_transfer->feed.fed_count);

  // Clear the feed counts (receive data was completed)
  ep_transfer->feed.total_count = ep_transfer->feed.fed_count = 0;
}

/****************************************************************************************************************************************
 * HAL public methods
 ****************************************************************************************************************************************/

/*
 * Initialise board HAL for USB function, and enable USB interrupts
 */
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

/*
 * Initialise board HAL for USB function, and enable USB interrupts
 */
void usb_init_enable_hal() {
  // Disable USB while initialising USB
  USB->DADDR = 0U;

  // Reset endpoint assignments
  usb_ep_reset();

  // Control ep (EP0) must exist
  usb_ep_control_init();

  // USB Device address (USB_DADDR)
  // Bit 7 EF: Enable function (USB_DADDR_EF)
  // This bit is set by the software to enable the USB Device. The address of this device is
  // contained in the following ADD[6:0] bits. If this bit is at 0 no transactions are handled,
  // irrespective of the settings of USB_CHEPnR registers.
  USB->DADDR = USB_DADDR_EF;  // Enable USB and clear USB device address
}

/*
 * Initialise HAL for USB function, signals host to enumerate device
 */
void usb_device_start_hal() {
  // Perform USB peripheral reset and enter power down mode
  USB->CNTR = USB_CNTR_USBRST | USB_CNTR_PDWN;

  // Power USB back up
  USB->CNTR &= ~USB_CNTR_PDWN;

  // When PDWN is set, the PHY clock is stopped and the USB transceiver is powered down.
  // After clearing PDWN, a startup time is required before enabling the peripheral.
  // Wait for at least 1 µs (PHY startup)
  for (volatile int i = 0; i < 100; i++) {
    __NOP();
  }

  // Enable USB (clear USB_CNTR_USBRST)
  USB->CNTR = 0;

  // Clear pending interrupts
  USB->ISTR = 0;

  // Reset endpoints to disabled
  for (uint32_t i = 0; i < USB_EP_MAX; i++) {
    // This doesn't clear all bits since some bits are "toggle", but does set the type to DISABLED.
    usb_ep_reg_set(i, 0, false);
  }

  // Enable interrupt flags
  USB->CNTR |= USB_CNTR_SOFM |     // SOF (start of frame)
               USB_CNTR_ESOFM |    // Expected SOF (start of frame) - used for disconnection detection and suspension resumption
               USB_CNTR_RESETM |   // USB reset interrupt interrupt enabled
               USB_CNTR_SUSPM |    // Suspend mode interrupt enabled
               USB_CNTR_WKUPM |    // Wake-up interrupt enabled
               USB_CNTR_PMAOVRM |  // Packet memory area over / underrun interrupt enabled
               USB_CNTR_CTRM;      // Correct transfer interrupt enabled

  // Initialise USB HAL function and control endpoint
  usb_init_enable_hal();

  // This bit is set by software to enable the embedded pull-up on DP line.
  // Clearing it to 0 can be used to signal disconnect to the host when
  // needed by the user software.
  USB->BCDR |= USB_BCDR_DPPU;

  // Enable interrupts
  NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

void USB_UCPD1_2_IRQHandler() {
  // Get copy of ISTR to reduce volatile reads
  uint32_t istr = USB->ISTR;

  // Reset interrupt
  if (istr & USB_ISTR_RESET) {
    USB->ISTR = ~USB_ISTR_RESET;

    // Reset USB
    usb_reset();

    // We have reset the USB hardware, return from interrupt
    // as there is no value in processing further status flags
    // until next interrupt is raised
    return;
  }

  // USB_ISTR_CTR can have multiple setup events queued, so we need to iterate until
  // all queued are cleared. If not we would return from interrupt only to interrupt again
  // which is not efficient.
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
        usb_rx_packet(ep_idn);
      }
    }

    if (ep_reg & USB_EP_VTTX) {
      // Clear USB_EP_VTTX
      usb_ep_clear_correct_transfer(ep_idn, USB_DIR_DEVICE_OUT_HOST_IN_IDX);

      usb_ep_transfer_t *ep_transfer = &ep_transfer_set[ep_idn][USB_DIR_DEVICE_OUT_HOST_IN_IDX];

      // Data remaining to send?
      if (ep_transfer->feed.total_count != ep_transfer->feed.fed_count) {
        // Transmit next packet
        usb_tx_packet(ep_transfer);
      } else {
        // Tranmit transfer complete
        usb_ep_transfer_complete(ep_idn | USB_DIR_DEVICE_OUT_HOST_IN, ep_transfer->feed.fed_count);
      }
    }
  }

  if (istr & USB_ISTR_SOF) {
    USB->ISTR = ~USB_ISTR_SOF;

    // Reset offline counter
    usb_offline_counter = 0;
  }

  // Expected SOF missing
  //  Triggered when the expected SOF is missing (i.e., the host didn'’'t send an SOF at the right time).
  //  Used in suspend/resume and reset detection logic:
  //     - If several ESOF events occur without receiving real SOFs → the device can infer the bus is in suspend.
  //     - Also used for remote wakeup timing, since USB spec requires precise SOF-based delays when resuming the bus.
  // During suspend, the host stops sending SOFs. So you cannot rely on SOF interrupts.
  // Instead, the peripheral generates ESOF interrupts internally every 1 ms to keep time during suspend.
  // That’s why remote_wake_counter is tied to ESOF.
  if (istr & USB_ISTR_ESOF) {
    USB->ISTR = ~USB_ISTR_ESOF;

    if (remote_wake_counter == 1) {
      // Stop driving resume
      USB->CNTR &= ~USB_CNTR_L2RES;
    }
    if (remote_wake_counter > 0) {
      remote_wake_counter--;
    }
  }

  // Wakeup
  if (istr & USB_ISTR_WKUP) {
    // On wake-up you should:
    //  - Clear the wakeup interrupt flag
    //  - Clear suspend control bits in CNTR (SUSPEN, SUSPRDY) so the PHY exits low-power.
    USB->ISTR = ~USB_ISTR_WKUP;
    USB->CNTR &= ~(USB_CNTR_SUSPRDY | USB_CNTR_SUSPEN);
  }

  // Suspend (and disconnect)
  if (istr & USB_ISTR_SUSP) {
    USB->ISTR = ~USB_ISTR_SUSP;

    // A brief description of a typical suspend procedure is provided below, focused on the USBrelated aspects of the application software routine responding to the SUSP notification of
    // the USB peripheral:
    //    1. Set the SUSPEN bit in the USB_CNTR register to 1. This action activates the suspend
    //       mode within the USB peripheral. As soon as the suspend mode is activated, the check
    //       on SOF reception is disabled to avoid any further SUSP interrupts being issued while
    //       the USB is suspended.
    //    2. Remove or reduce any static power consumption in blocks different from the USB
    //       peripheral.
    //    3. Set SUSPRDY bit in USB_CNTR register to 1 to remove static power consumption in
    //       the analog USB transceivers but keeping them able to detect resume activity.
    //    4. Optionally turn off external oscillator and device PLL to stop any activity inside the
    //       device.
    // These need to be set of two oerations, do not set both flags in a single operation (on some STM32 devices)
    USB->CNTR |= USB_CNTR_SUSPEN;
    USB->CNTR |= USB_CNTR_SUSPRDY;

    // Call device layer suspen
    usb_device_suspended();
  }
}

/*
 * Prepare HAL for sending / receiving data from host
 */
bool usb_ep_queue_transfer_hal(uint8_t ep_idn, uint8_t ep_dir_idx, uint8_t *buffer, uint16_t total_bytes) {
  // Get the endpoint specific transfer buffer
  usb_ep_transfer_t *ep_transfer = &ep_transfer_set[ep_idn][ep_dir_idx];

  // Initialise transfer feed
  ep_transfer->feed.buffer = buffer;            // Use callers buffer
  ep_transfer->feed.total_count = total_bytes;  // We are going to transfer total bytes
  ep_transfer->feed.fed_count = 0;              // Nothing has been transferred yet

  if (ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX) {
    // Transmit from device is USB_DIR_DEVICE_OUT_HOST_IN_IDX to host
    usb_tx_packet(ep_transfer);
  } else {
    // Receive to device is USB_DIR_DEVICE_IN_HOST_OUT_IDX from host
    uint32_t ep_reg = USB->chep[ep_idn].CHEPnR;
    ep_reg &= (USB_CHEP_REG_MASK | USB_EP_STATUS_MASK(ep_dir_idx));

    usb_ep_set_rx_buffer_block_size(ep_idn, (uint32_t)ep_transfer->feed.total_count);
    usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_VALID);
    usb_ep_reg_set_preserve(ep_idn, ep_reg, true);
  }

  // STM32G0B1 does not detect failures in this method so always return true (assume success)
  return true;
}

/*
 * Set the device address in HAL
 */
void usb_device_set_addr_hal(const uint8_t device_addr) {
  // Set the device address and keep enabled
  USB->DADDR = (USB_DADDR_EF | device_addr);

  // Address is set upon ACK status response so we clear RX count
  usb_ep_set_rx_buffer_block_size(EP0_IDN, sizeof(usb_control_request_t));
}

bool usb_ep_open_hal(const usb_ep_descriptor_t *ep_descriptor) {
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
  usb_pma_ep_addr_set(ep_idn, ep_dir_idx == USB_DIR_DEVICE_OUT_HOST_IN_IDX ? USB_EP_TX_COUNT_ADDR_IDX : USB_EP_RX_COUNT_ADDR_IDX, pma_addr);

  usb_ep_transfer_t *ep_transfer = &ep_transfer_set[ep_idn][ep_dir_idx];
  ep_transfer->max_packet_size = packet_size;
  ep_transfer->ep_idn = ep_idn;

  usb_ep_status(&ep_reg, ep_dir_idx, USB_EP_STATE_NAK);
  usb_ep_data_toggle(&ep_reg, ep_dir_idx, 0);

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

  // Reset PMA assignment (except EP0)
  usb_pma_next_available = 8 * USB_EP_MAX + 2 * USB_EP0_BUFFER_SIZE;
}

/*
 * Wake up device from suspend mode
 */
bool usb_remote_wakeup_start_hal() {
  if (remote_wake_counter > 1) {
    // Already in waking up
    return false;
  }

  // Start driving resume (K-state) on the bus
  USB->CNTR |= USB_CNTR_L2RES;

  // Hold resume for 10 ms (USB spec: must be 1–15 ms)
  remote_wake_counter = 10;

  return true;
}

/*
 * Should be called every 1ms approximately
 */
void usb_systick_hal() {
  // Only timeout once
  if (usb_offline_counter >= 100) {
    return;
  }

  usb_offline_counter++;

  if (usb_offline_counter >= 100) {
    usb_device_suspended_sof_timeout();
  }
}